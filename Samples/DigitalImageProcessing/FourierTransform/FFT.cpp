#include "FFT.h"
#include "Graphics/ShaderCompiler.h"
#include "../Renderer.h"

namespace DSM {
    void FFT::Initialize(uint32_t width, uint32_t height)
    {
        m_FFTOutputUAV = g_Renderer.m_TextureHeap.Allocate();
        m_FFTOutputSRV = g_Renderer.m_TextureHeap.Allocate();
        m_IFFTOutputUAV = g_Renderer.m_TextureHeap.Allocate();
        m_IFFTOutputSRV = g_Renderer.m_TextureHeap.Allocate();

        if(m_EnabledDebugOutput){
            m_FFTDebugSRV = g_Renderer.m_TextureHeap.Allocate();
            m_FFTDebugUAV = g_Renderer.m_TextureHeap.Allocate();
        }

        Resize(width, height);

        m_FFTRootSig[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1);
        m_FFTRootSig[1].InitAsBufferSRV(0);
        m_FFTRootSig[2].InitAsConstants(0, sizeof(FFTConstants) / sizeof(uint32_t));
        if(m_EnabledDebugOutput){    
            m_FFTRootSig[3].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1);
        }
        m_FFTRootSig.Finalize(L"FFT Root Signature");

        auto createPSO = [this](
            auto& pso, 
            const std::string& entryPoint, 
            std::vector<std::pair<std::string, std::string>> defines = {}) {
            ShaderDesc shaderDesc{
                .m_Type = ShaderType::Compute,
                .m_Mode = ShaderMode::SM_6_6,
                .m_FileName = "Shaders/FourierTransform/FFT.hlsl",
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

        std::vector<std::pair<std::string, std::string>> defines{};
        createPSO(m_IFFTScalePSO, "IFFTScale");
        if(m_EnabledDebugOutput){
            defines.emplace_back("ENABLE_DEBUG_OUTPUT", "1");
        }

        createPSO(m_FFTHorizBitReversedPSO, "BitReverseCS", defines);
        createPSO(m_FFTHorizPSO, "LuminanceFFTCS", defines);
        createPSO(m_FFTHorizGroupMemPSO, "LuminanceFFTCSWithGroupMem", defines);

        defines.emplace_back("IS_VERTICAL", "1");
        createPSO(m_FFTVerticBitReversedPSO, "BitReverseCS", defines);
        createPSO(m_FFTVerticPSO, "LuminanceFFTCS", defines);
        createPSO(m_FFTVerticGroupMemPSO, "LuminanceFFTCSWithGroupMem", defines);
    }
    
    void FFT::ExecuteFFT(ComputeCommandList &cmdList, Texture &inputTex, DescriptorHandle inputSRV)
    {
        // 输入的纹理需要为复数格式
        assert(inputTex.GetFormat() == DXGI_FORMAT_R32G32_FLOAT);

        if(inputTex.GetWidth() != m_FFTOutputTex.GetWidth() ||
            inputTex.GetHeight() != m_FFTOutputTex.GetHeight()){
            Resize(inputTex.GetWidth(), inputTex.GetHeight());
        }

        auto rect = RECT{0, 0, static_cast<LONG>(inputTex.GetWidth()), static_cast<LONG>(inputTex.GetHeight())};
        cmdList.CopyTextureRegion(m_FFTOutputTex, 0, 0, 0, inputTex, rect);
        Execute(cmdList, m_FFTOutputTex, m_FFTOutputUAV, false);
    }
    
    void FFT::ExecuteIFFT(ComputeCommandList &cmdList, Texture &inputTex, DescriptorHandle inputSRV)
    {
        // 输入的纹理需要为复数格式
        assert(inputTex.GetFormat() == DXGI_FORMAT_R32G32_FLOAT);

        if(inputTex.GetWidth() != m_IFFTOutputTex.GetWidth() ||
            inputTex.GetHeight() != m_IFFTOutputTex.GetHeight()){
            Resize(inputTex.GetWidth(), inputTex.GetHeight());
        }

        auto rect = RECT{0, 0, static_cast<LONG>(inputTex.GetWidth()), static_cast<LONG>(inputTex.GetHeight())};
        cmdList.CopyTextureRegion(m_IFFTOutputTex, 0, 0, 0, inputTex, rect);
        Execute(cmdList, m_IFFTOutputTex, m_IFFTOutputUAV, true);
    }
    
    void FFT::Resize(uint32_t width, uint32_t height)
    {
        CreateReverseIndicesBuffer(width, height);

        TextureDesc fftOutputDesc{};
        fftOutputDesc.m_Width = Math::NextPowerOf2(width);
        fftOutputDesc.m_Height = Math::NextPowerOf2(height);
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
    }
    
    void FFT::Execute(ComputeCommandList &cmdList, Texture& outputTex, DescriptorHandle outputUAV, bool inverse)
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
            GpuBuffer& indicesBuffer,
            uint32_t numStages,
            uint32_t axisStages,
            uint32_t width, 
            uint32_t height){
            cmdList.SetPipelineState(bitReversePSO);
            cmdList.SetDescriptorTable(0, outputUAV);
            cmdList.SetShaderResource(1, indicesBuffer);
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
            m_HorizReverseIndicesBuffer,
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
            m_VerticReverseIndicesBuffer,
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
}