#include "FFT.h"
#include "Graphics/ShaderCompiler.h"
#include "../../Renderer.h"
#include <complex>
#include <numbers>

namespace DSM {
    FFT::FFT(bool enabledOutput) : 
        m_EnabledDebugOutput(enabledOutput), 
        m_FFTRootSig{enabledOutput ? 4u : 3u, 0} {}

    void FFT::Initialize(uint32_t width, uint32_t height)
    {
        m_FFTOutputUAV = g_Renderer.m_TextureHeap.Allocate();
        m_FFTOutputSRV = g_Renderer.m_TextureHeap.Allocate();
        m_IFFTOutputUAV = g_Renderer.m_TextureHeap.Allocate();
        m_IFFTOutputSRV = g_Renderer.m_TextureHeap.Allocate();
        m_ConvolutionKernelUAV = g_Renderer.m_TextureHeap.Allocate();
        m_SequenceUAV = g_Renderer.m_TextureHeap.Allocate();
        m_ConvolutionKernelSRV = g_Renderer.m_TextureHeap.Allocate();
        m_SequenceSRV = g_Renderer.m_TextureHeap.Allocate();
        m_HorizReverseIndicesSRV = g_Renderer.m_TextureHeap.Allocate();
        m_VerticReverseIndicesSRV = g_Renderer.m_TextureHeap.Allocate();

        if(m_EnabledDebugOutput){
            m_FFTDebugSRV = g_Renderer.m_TextureHeap.Allocate();
            m_FFTDebugUAV = g_Renderer.m_TextureHeap.Allocate();
        }

        m_FFTRootSig[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1);
        m_FFTRootSig[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1);
        m_FFTRootSig[2].InitAsConstants(0, sizeof(FFTConstants) / sizeof(uint32_t));
        if(m_EnabledDebugOutput){    
            m_FFTRootSig[3].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1);
        }
        m_FFTRootSig.Finalize(L"FFT Root Signature");

        auto createPSO = [this](
            auto& pso,
            const std::string& fileName,
            const std::string& entryPoint,
            std::vector<std::pair<std::string, std::string>> defines = {}) {
            ShaderDesc shaderDesc{
                .m_Type = ShaderType::Compute,
                .m_Mode = ShaderMode::SM_6_6,
                .m_FileName = fileName,
                .m_EnterPoint = entryPoint,
            };
            for(const auto& define : defines){
                shaderDesc.m_Defines.AddDefine(define.first, define.second);
            }
            ShaderByteCode shader{shaderDesc};
            pso.SetRootSignature(m_FFTRootSig);
            pso.SetComputeShader(shader);
            pso.Finalize();
        };

        // 基2 FFT
        std::string fftRadix2FileName = "Shaders/DigitalImageProcessing/FFTRadix2.hlsl";
        std::vector<std::pair<std::string, std::string>> defines{};
        createPSO(m_IFFTScalePSO, fftRadix2FileName, "IFFTScale");
        if(m_EnabledDebugOutput){
            defines.emplace_back("ENABLE_DEBUG_OUTPUT", "1");
        }

        createPSO(m_FFTHorizBitReversedPSO, fftRadix2FileName, "BitReverseCS", defines);
        createPSO(m_FFTHorizPSO, fftRadix2FileName, "FFTRadix2CS", defines);
        createPSO(m_FFTHorizGroupMemPSO, fftRadix2FileName, "FFTRadix2WithGroupMemCS", defines);

        defines.emplace_back("IS_VERTICAL", "1");
        createPSO(m_FFTVerticBitReversedPSO, fftRadix2FileName, "BitReverseCS", defines);
        createPSO(m_FFTVerticPSO, fftRadix2FileName, "FFTRadix2CS", defines);
        createPSO(m_FFTVerticGroupMemPSO, fftRadix2FileName, "FFTRadix2WithGroupMemCS", defines);

        // Bluestein FFT
        std::string fftBluesteinFileName = "Shaders/DigitalImageProcessing/FFTBluestein.hlsl";
        createPSO(m_CalcuSequencePSO, fftBluesteinFileName, "CalcuSequenceCS");
        createPSO(m_FrequencyMultiplicationPSO, fftBluesteinFileName, "FrequencyMultiplicationCS");
        createPSO(m_PhaseFactorPSO, fftBluesteinFileName, "PhaseFactorCS");


        Resize(width, height);
    }
    
    void FFT::ExecuteFFT(ComputeCommandList &cmdList, Texture &inputTex, DescriptorHandle inputSRV)
    {
        Execute(cmdList, inputTex, inputSRV, m_FFTOutputTex, m_FFTOutputUAV, false);
    }
    
    void FFT::ExecuteIFFT(ComputeCommandList &cmdList, Texture &inputTex, DescriptorHandle inputSRV)
    {
        Execute(cmdList, inputTex, inputSRV, m_IFFTOutputTex, m_IFFTOutputUAV, true);
    }
    
    void FFT::Resize(uint32_t width, uint32_t height)
    {
        uint32_t indicesWidth = width;
        uint32_t indicesHeight = height;

        // 如果宽度或高度不是2的幂，则使用 Bluestein FFT
        if(!Math::IsPowerOf2(width) || !Math::IsPowerOf2(height)){
            indicesWidth = Math::NextPowerOf2(width * 2 - 1);
            indicesHeight = Math::NextPowerOf2(height * 2 - 1);

            // 预计算卷积核
            std::vector<std::complex<float>> convolutionKernel;
            convolutionKernel.resize(indicesWidth * indicesHeight);
            for(uint32_t i = 0; i < indicesHeight; ++i){
                for(uint32_t j = 0; j < indicesWidth; ++j){
                    // 计算映射到原始尺寸的索引
                    uint32_t mapped_i = (i < height) ? i : indicesHeight - i;
                    uint32_t mapped_j = (j < width) ? j : indicesWidth - j;

                    // 确保索引在有效范围内
                    mapped_i = (mapped_i < height) ? mapped_i : 0;
                    mapped_j = (mapped_j < width) ? mapped_j : 0;

                    float theta = std::numbers::pi_v<float> *
                            (float(mapped_i * mapped_i) / height +
                            float(mapped_j * mapped_j) / width);

                    convolutionKernel[i * indicesWidth + j] = std::exp(std::complex<float>(0.0f, theta));
                }
            }
            TextureDesc tmpTexDesc{};
            tmpTexDesc.m_Width = indicesWidth;
            tmpTexDesc.m_Height = indicesHeight;
            tmpTexDesc.m_Format = DXGI_FORMAT_R32G32_FLOAT;
            tmpTexDesc.m_Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            tmpTexDesc.m_Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
            D3D12_SUBRESOURCE_DATA convolutionKernelData{};
            convolutionKernelData.pData = convolutionKernel.data();
            convolutionKernelData.RowPitch = sizeof(std::complex<float>) * indicesWidth;
            convolutionKernelData.SlicePitch = convolutionKernelData.RowPitch * indicesHeight;
            m_ConvolutionKernelTex.Create(L"Convolution Kernel Texture", tmpTexDesc, {&convolutionKernelData, 1});
            m_ConvolutionKernelTex.CreateUnorderedAccessView(m_ConvolutionKernelUAV);
            m_ConvolutionKernelTex.CreateShaderResourceView(m_ConvolutionKernelSRV);
            m_SequenceTex.Create(L"Sequence Texture", tmpTexDesc);
            m_SequenceTex.CreateUnorderedAccessView(m_SequenceUAV);
            m_SequenceTex.CreateShaderResourceView(m_SequenceSRV);

            // 预计算卷积核的 FFT
            ComputeCommandList cmdList{L"FFT Precompute Convolution Kernel"};
            cmdList.SetDescriptorHeap(g_Renderer.m_TextureHeap.GetHeap());
            ExecuteRadix2(cmdList, m_ConvolutionKernelTex, m_ConvolutionKernelUAV, false);
        }
        else{   // 无需使用，节省显存
            m_ConvolutionKernelTex.Destroy();
            m_SequenceTex.Destroy();
        }

        // 创建位反转索引缓冲区
        CreateReverseIndicesBuffer(indicesWidth, indicesHeight);

        TextureDesc fftOutputDesc{};
        fftOutputDesc.m_Width = width;
        fftOutputDesc.m_Height = height;
        fftOutputDesc.m_Format = DXGI_FORMAT_R32G32_FLOAT;
        fftOutputDesc.m_Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        fftOutputDesc.m_Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        m_FFTOutputTex.Create(L"FFT Output Texture", fftOutputDesc);
        m_FFTOutputTex.CreateUnorderedAccessView(m_FFTOutputUAV);
        m_FFTOutputTex.CreateShaderResourceView(m_FFTOutputSRV);
        TextureDesc ifftOutputDesc = fftOutputDesc;
        m_IFFTOutputTex.Create(L"IFFT Output Texture", ifftOutputDesc);
        m_IFFTOutputTex.CreateUnorderedAccessView(m_IFFTOutputUAV);
        m_IFFTOutputTex.CreateShaderResourceView(m_IFFTOutputSRV);

        if(m_EnabledDebugOutput){
            TextureDesc fftDebugDesc = fftOutputDesc;
            fftDebugDesc.m_Width = width;
            fftDebugDesc.m_Height = height;
            fftDebugDesc.m_Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            m_FFTDebugTex.Create(L"FFT Debug Texture", fftDebugDesc);
            m_FFTDebugTex.CreateShaderResourceView(m_FFTDebugSRV);
            m_FFTDebugTex.CreateUnorderedAccessView(m_FFTDebugUAV);
        }
    }
    
    void FFT::CreateReverseIndicesBuffer(uint32_t width, uint32_t height)
    {
        width = Math::NextPowerOf2(width);
        std::vector<uint32_t> reverseIndices(width);
        uint32_t numBits = static_cast<uint32_t>(std::log2(width));
        for (uint32_t i = 0; i < width; ++i) {
            reverseIndices[i] = Math::ReverseBits(i, numBits);
        }
        m_HorizReverseIndicesBuffer.Create(L"FFT Horizontal Reverse Indices Buffer",
            GpuBufferDesc{
                .m_Size = sizeof(uint32_t) * width,
                .m_Stride = sizeof(uint32_t)
            },
            reverseIndices.data());
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format = DXGI_FORMAT_R32_UINT;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Buffer.FirstElement = 0;
        srvDesc.Buffer.NumElements = width;
        g_RenderContext.GetDevice()->CreateShaderResourceView(
            m_HorizReverseIndicesBuffer.GetResource(), &srvDesc, m_HorizReverseIndicesSRV);

        height = Math::NextPowerOf2(height);
        reverseIndices.resize(height);
        numBits = static_cast<uint32_t>(std::log2(height));
        for (size_t i = 0; i < height; i++) {
            reverseIndices[i] = Math::ReverseBits(i, numBits);
        }
        m_VerticReverseIndicesBuffer.Create(L"FFT Vertical Reverse Indices Buffer",
            GpuBufferDesc{
                .m_Size = sizeof(uint32_t) * height,
                .m_Stride = sizeof(uint32_t)
            },
            reverseIndices.data());
        srvDesc.Buffer.NumElements = height;
        g_RenderContext.GetDevice()->CreateShaderResourceView(
            m_VerticReverseIndicesBuffer.GetResource(), &srvDesc, m_VerticReverseIndicesSRV);
    }
    
    void FFT::ExecuteRadix2(ComputeCommandList &cmdList, Texture& outputTex, DescriptorHandle outputUAV, bool inverse)
    {
        uint32_t width = outputTex.GetWidth();
        uint32_t height = outputTex.GetHeight();
        uint32_t horizStages = std::log2(Math::NextPowerOf2(width));
        uint32_t vertStages = std::log2(Math::NextPowerOf2(height));
        FFTConstants constants{
            .numStages = 1,
            .stage = 0,
            .sign = -1.0f
        };
        if(inverse){
            constants.sign = 1;
        }

        auto computeFFT = [&](auto& bitReversePSO,
            auto& groupMemPSO,
            auto& fftPSO,
            DescriptorHandle indicesBufferSRV,
            uint32_t numStages,
            uint32_t axisStages,
            uint32_t width, 
            uint32_t height){
            cmdList.SetPipelineState(bitReversePSO);
            cmdList.SetDescriptorTable(0, outputUAV);
            cmdList.SetDescriptorTable(1, indicesBufferSRV);
            cmdList.Dispatch2D(width, height, sm_ThreadSize, 1);
            cmdList.InsertUAVBarrier(outputTex, true);

            cmdList.SetPipelineState(groupMemPSO);
            if(m_EnabledDebugOutput){
                cmdList.SetDescriptorTable(3, m_FFTDebugUAV);
            }
            cmdList.SetDescriptorTable(0, outputUAV);
            constants.numStages = numStages;
            constants.stage = 0;
            cmdList.SetConstantArray(2, sizeof(FFTConstants) / sizeof(uint32_t), &constants);
            cmdList.Dispatch2D(width, height, sm_ThreadSize, 1);
            cmdList.InsertUAVBarrier(outputTex, true);

            cmdList.SetPipelineState(fftPSO);
            if(m_EnabledDebugOutput){
                cmdList.SetDescriptorTable(3, m_FFTDebugUAV);
            }
            constants.numStages = 1;
            for(uint32_t stage = numStages; stage < axisStages; ++stage){
                cmdList.SetDescriptorTable(0, outputUAV);
                constants.stage = stage;
                cmdList.SetConstantArray(2, sizeof(FFTConstants) / sizeof(uint32_t), &constants);
                cmdList.Dispatch2D(width, height, sm_ThreadSize, 1);
                cmdList.InsertUAVBarrier(outputTex, true);
            }
        };

        cmdList.SetRootSignature(m_FFTRootSig);

        // 水平FFT
        uint32_t numStages = (std::min)(uint32_t(std::log2(sm_ThreadSize)), horizStages);
        computeFFT(
            m_FFTHorizBitReversedPSO,
            m_FFTHorizGroupMemPSO,
            m_FFTHorizPSO,
            m_HorizReverseIndicesSRV,
            numStages,
            horizStages,
            width,
            height);

        // 垂直FFT
        numStages = (std::min)(uint32_t(std::log2(sm_ThreadSize)), vertStages);
        computeFFT(
            m_FFTVerticBitReversedPSO,
            m_FFTVerticGroupMemPSO,
            m_FFTVerticPSO,
            m_VerticReverseIndicesSRV,
            numStages,
            vertStages,
            height,
            width);

        if(inverse){
            cmdList.SetPipelineState(m_IFFTScalePSO);
            cmdList.SetDescriptorTable(0, outputUAV);
            cmdList.Dispatch2D(width, height, 32, 32);
        }

        cmdList.ExecuteCommandList();
    }
    
    void FFT::Execute(
        ComputeCommandList &cmdList, 
        Texture &inputTex, DescriptorHandle inputSRV, 
        Texture &outputTex, DescriptorHandle outputUAV, 
        bool inverse)
    {
        // 输入的纹理需要为复数格式
        assert(inputTex.GetFormat() == DXGI_FORMAT_R32G32_FLOAT);

        auto width = inputTex.GetWidth();
        auto height = inputTex.GetHeight();
        if(width != outputTex.GetWidth() ||
            height != outputTex.GetHeight()){
            Resize(width, height);
        }

        bool isPowerOf2 = Math::IsPowerOf2(width) && Math::IsPowerOf2(height);

        if(isPowerOf2){
            auto rect = RECT{0, 0, static_cast<LONG>(inputTex.GetWidth()), static_cast<LONG>(inputTex.GetHeight())};
            cmdList.CopyTextureRegion(outputTex, 0, 0, 0, inputTex, rect);
            ExecuteRadix2(cmdList, outputTex, outputUAV, inverse);
        }
        else{
            std::array<float, 3> constants = { 0.0f, 0.0f, 0.0f };
            constants[0] = inverse ? 1.0f : -1.0f;

            uint32_t indicesWidth = Math::NextPowerOf2(width * 2 - 1);
            uint32_t indicesHeight = Math::NextPowerOf2(height * 2 - 1);
            cmdList.SetRootSignature(m_FFTRootSig);

            // 计算序列并转换到频域
            cmdList.SetPipelineState(m_CalcuSequencePSO);
            cmdList.SetDescriptorTable(0, m_SequenceUAV);
            cmdList.SetDescriptorTable(1, inputSRV);
            cmdList.SetConstantArray(2, constants.size(), constants.data());
            cmdList.Dispatch2D(indicesWidth, indicesHeight, 16, 16);
            cmdList.InsertUAVBarrier(m_SequenceTex, true);

            ExecuteRadix2(cmdList, m_SequenceTex, m_SequenceUAV, false);

            // 将频域空间的序列与卷积核相乘
            cmdList.SetPipelineState(m_FrequencyMultiplicationPSO);
            cmdList.SetDescriptorTable(0, m_SequenceUAV);
            cmdList.SetDescriptorTable(1, m_ConvolutionKernelSRV);
            cmdList.SetConstantArray(2, constants.size(), constants.data());
            cmdList.Dispatch2D(indicesWidth, indicesHeight, 16, 16);
            cmdList.InsertUAVBarrier(m_ConvolutionKernelTex, true);

            // 进行逆变换
            ExecuteRadix2(cmdList, m_SequenceTex, m_SequenceUAV, true);

            // 乘上相位因子
            cmdList.SetPipelineState(m_PhaseFactorPSO);
            cmdList.SetDescriptorTable(0, outputUAV);
            cmdList.SetDescriptorTable(1, m_SequenceSRV);
            cmdList.SetConstantArray(2, constants.size(), constants.data());
            if(m_EnabledDebugOutput){
                cmdList.SetDescriptorTable(3, m_FFTDebugUAV);
            }
            cmdList.Dispatch2D(width, height, 16, 16);

            cmdList.ExecuteCommandList();
        }
    }
}