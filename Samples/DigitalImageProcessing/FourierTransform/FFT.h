#pragma once
#ifndef __FFT_H__
#define __FFT_H__

#include "Graphics/CommandList/ComputeCommandList.h"
#include "Graphics/RootSignature.h"
#include "Graphics/PipelineState.h"
#include "Graphics/ShaderCompiler.h"

namespace DSM{
    class FFT
    {
    public:
        FFT(bool enabledOutput = true) : 
            m_EnabledDebugOutput(enabledOutput), 
            m_FFTRootSig{enabledOutput ? 5u : 4u, 0} {}

        void Initialize(uint32_t width, uint32_t height)
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

            m_FFTRootSig[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1);
            m_FFTRootSig[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1);
            m_FFTRootSig[2].InitAsBufferSRV(1);
            m_FFTRootSig[3].InitAsConstants(0, sizeof(FFTConstants) / sizeof(uint32_t));
            if(m_EnabledDebugOutput){    
                m_FFTRootSig[4].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1);
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
            createPSO(m_FFTConvertDataPSO, "ConvertData");
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

        void ExecuteFFT(ComputeCommandList& cmdList, Texture& inputTex, DescriptorHandle inputSRV)
        {
            auto func = [this, &cmdList, inputSRV](auto& outputTex, auto& outputUAV) {
                cmdList.SetPipelineState(m_FFTConvertDataPSO);
                cmdList.SetDescriptorTable(0, inputSRV);
                cmdList.SetDescriptorTable(1, outputUAV);
                cmdList.Dispatch2D(outputTex.GetWidth(), outputTex.GetHeight(), 32, 32);
                cmdList.InsertUAVBarrier(outputTex, true);
            };
            Execute(cmdList, inputTex, false, func);
        }

        void ExecuteIFFT(ComputeCommandList& cmdList, Texture& inputTex, DescriptorHandle inputSRV)
        {
            auto func = [&cmdList, &inputTex](auto& outputTex, auto& outputUAV) {
                auto rect = RECT{0, 0, static_cast<LONG>(inputTex.GetWidth()), static_cast<LONG>(inputTex.GetHeight())};
                cmdList.CopyTextureRegion(outputTex, 0, 0, 0, inputTex, rect);
            };
            Execute(cmdList, inputTex, true, func);
        }

        void Resize(uint32_t width, uint32_t height)
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

        Texture& GetFFTOutputTex() noexcept { return m_FFTOutputTex; }
        const Texture& GetFFTOutputTex() const noexcept { return m_FFTOutputTex; }
        DescriptorHandle GetFFTOutputSRV() const noexcept { return m_FFTOutputSRV; }
        Texture& GetFFTDebugTex() noexcept { return m_FFTDebugTex; }
        const Texture& GetFFTDebugTex() const noexcept { return m_FFTDebugTex; }
        DescriptorHandle GetFFTDebugSRV() const noexcept { return m_FFTDebugSRV; }
        DescriptorHandle GetIFFTOutputSRV() const noexcept { return m_IFFTOutputSRV; }
        Texture& GetIFFTOutputTex() noexcept { return m_IFFTOutputTex; }
        const Texture& GetIFFTOutputTex() const noexcept { return m_IFFTOutputTex; }

    private:
        void CreateReverseIndicesBuffer(uint32_t width, uint32_t height)
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

        template<typename Func>
        void Execute(
            ComputeCommandList& cmdList, 
            Texture& inputTex, 
            bool inverse,
            Func&& initFunc)
        {
            auto& outputTex = inverse ? m_IFFTOutputTex : m_FFTOutputTex;
            auto& outputUAV = inverse ? m_IFFTOutputUAV : m_FFTOutputUAV;
            auto& outputSRV = inverse ? m_IFFTOutputSRV : m_FFTOutputSRV;

            if(inputTex.GetWidth() != outputTex.GetWidth() ||
               inputTex.GetHeight() != outputTex.GetHeight()){
                Resize(inputTex.GetWidth(), inputTex.GetHeight());
            }
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
                cmdList.SetDescriptorTable(1, outputUAV);
                cmdList.SetShaderResource(2, indicesBuffer);
                cmdList.Dispatch2D(width, height, sm_ThreadSize, 1);
                cmdList.InsertUAVBarrier(outputTex, true);

                cmdList.SetPipelineState(groupMemPSO);
                if(m_EnabledDebugOutput){
                    cmdList.SetDescriptorTable(4, m_FFTDebugUAV);
                }
                cmdList.SetDescriptorTable(1, outputUAV);
                constants.numStages = numStages;
                constants.stage = 0;
                cmdList.SetConstantArray(3, sizeof(FFTConstants) / sizeof(uint32_t), &constants);
                cmdList.Dispatch2D(width, height, sm_ThreadSize, 1);
                cmdList.InsertUAVBarrier(outputTex, true);

                cmdList.SetPipelineState(fftPSO);
                if(m_EnabledDebugOutput){
                    cmdList.SetDescriptorTable(4, m_FFTDebugUAV);
                }
                constants.numStages = 1;
                for(uint32_t stage = numStages; stage < axisStages; ++stage){
                    cmdList.SetDescriptorTable(1, outputUAV);
                    constants.stage = stage;
                    cmdList.SetConstantArray(3, sizeof(FFTConstants) / sizeof(uint32_t), &constants);
                    cmdList.Dispatch2D(width, height, sm_ThreadSize, 1);
                    cmdList.InsertUAVBarrier(outputTex, true);
                }
            };

            cmdList.SetRootSignature(m_FFTRootSig);

            initFunc(outputTex, outputUAV);

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
                cmdList.SetDescriptorTable(1, outputUAV);
                cmdList.Dispatch2D(width, height, 32, 32);
            }

            cmdList.ExecuteCommandList(true);
        }


    private:
        struct FFTConstants
        {
            uint32_t numStages;
            uint32_t stage;
            float sign;
        };

        static constexpr uint32_t sm_ThreadSize = 256;

        bool m_EnabledDebugOutput = true;

        // 位反转后的索引
        GpuBuffer m_HorizReverseIndicesBuffer{};
        GpuBuffer m_VerticReverseIndicesBuffer{};

        Texture m_FFTOutputTex;
        DescriptorHandle m_FFTOutputUAV{};
        DescriptorHandle m_FFTOutputSRV{};

        Texture m_IFFTOutputTex;
        DescriptorHandle m_IFFTOutputUAV{};
        DescriptorHandle m_IFFTOutputSRV{};

        Texture m_FFTDebugTex;
        DescriptorHandle m_FFTDebugSRV{};
        DescriptorHandle m_FFTDebugUAV{};

        RootSignature m_FFTRootSig;

        ComputePSO m_FFTHorizGroupMemPSO;
        ComputePSO m_FFTVerticGroupMemPSO;
        ComputePSO m_FFTHorizPSO;
        ComputePSO m_FFTVerticPSO;
        
        ComputePSO m_FFTHorizBitReversedPSO;
        ComputePSO m_FFTVerticBitReversedPSO;

        ComputePSO m_FFTConvertDataPSO;

        ComputePSO m_IFFTHorizPSO;
        ComputePSO m_IFFTVerticPSO;

        ComputePSO m_IFFTScalePSO;
    };
}


#endif  // __FFT_H__