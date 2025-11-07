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
            m_FFTTmpUAV = g_Renderer.m_TextureHeap.Allocate();
            m_FFTTmpSRV = g_Renderer.m_TextureHeap.Allocate();

            if(m_EnabledDebugOutput){
                m_FFTDebugSRV = g_Renderer.m_TextureHeap.Allocate();
                m_FFTDebugUAV = g_Renderer.m_TextureHeap.Allocate();
            }

            Resize(width, height);

            m_FFTRootSig[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1);
            m_FFTRootSig[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1);
            m_FFTRootSig[2].InitAsBufferSRV(1);
            m_FFTRootSig[3].InitAsConstants(0, 1);
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

            createPSO(m_FFTHorizBitReversedPSO, "BitReverseCS");
            createPSO(m_FFTHorizPSO, "HorizFFTCS");
            std::vector<std::pair<std::string, std::string>> verticDefines = {{"IS_VERTIC_FFT", "1"}};
            createPSO(m_FFTVerticBitReversedPSO, "BitReverseCS", verticDefines);
            if(m_EnabledDebugOutput){
                verticDefines.emplace_back("ENABLE_DEBUG_OUTPUT", "1");
            }
            // m_RootSig[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1);
            // m_RootSig[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1);
            // if(m_EnabledDebugOutput){
            //     m_RootSig[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1);
            // }
            // m_RootSig.Finalize(L"DFT Root Signature");
            // m_FFTVerticPSO.SetRootSignature(m_RootSig);
            // ShaderDesc verticCSDesc{};
            // verticCSDesc.m_Type = ShaderType::Compute;
            // verticCSDesc.m_Mode = ShaderMode::SM_6_6;
            // verticCSDesc.m_FileName = "Shaders/FourierTransform/DFT.hlsl";
            // verticCSDesc.m_EnterPoint = "LuminanceDFTCS";
            // verticCSDesc.m_Defines.AddDefine("IS_VERTIC_DFT", "1");
            // if(m_EnabledDebugOutput){
            //     verticCSDesc.m_Defines.AddDefine("ENABLE_DEBUG_OUTPUT", "1");
            // }
            // ShaderByteCode verticDFTCS{verticCSDesc};
            // m_FFTVerticPSO.SetComputeShader(verticDFTCS);
            // m_FFTVerticPSO.Finalize();
            createPSO(m_FFTVerticPSO, "VerticFFTCS", verticDefines);
            createPSO(m_FFTConvertDataPSO, "ConvertData");
        }

        void ExecuteFFT(ComputeCommandList& cmdList, Texture& inputTex, DescriptorHandle inputSRV)
        {
            if(inputTex.GetWidth() != m_FFTOutputTex.GetWidth() ||
               inputTex.GetHeight() != m_FFTOutputTex.GetHeight()){
                Resize(inputTex.GetWidth(), inputTex.GetHeight());
            }
            uint32_t width = m_FFTOutputTex.GetWidth();
            uint32_t height = m_FFTOutputTex.GetHeight();

            cmdList.SetRootSignature(m_FFTRootSig);

            cmdList.SetPipelineState(m_FFTConvertDataPSO);
            cmdList.SetDescriptorTable(0, inputSRV);
            cmdList.SetDescriptorTable(1, m_FFTTmpUAV);
            cmdList.Dispatch2D(width, height, 32, 32);
            cmdList.InsertUAVBarrier(m_FFTTmpTex, true);

            // 水平FFT
            cmdList.SetPipelineState(m_FFTHorizBitReversedPSO);
            cmdList.SetDescriptorTable(0, m_FFTTmpSRV);
            cmdList.SetDescriptorTable(1, m_FFTOutputUAV);
            cmdList.SetShaderResource(2, m_HorizReverseIndicesBuffer);
            cmdList.Dispatch2D(width, height, 1, 1);
            cmdList.InsertUAVBarrier(m_FFTOutputTex, true);

            cmdList.SetPipelineState(m_FFTHorizPSO);
            cmdList.SetShaderResource(2, m_HorizReverseIndicesBuffer);
            std::array<DescriptorHandle, 2> inputSRVs = { m_FFTOutputSRV, m_FFTTmpSRV };
            std::array<DescriptorHandle, 2> outputUAVs = { m_FFTTmpUAV, m_FFTOutputUAV };
            uint32_t states = std::log2(Math::NextPowerOf2(m_FFTOutputTex.GetWidth()));
            for(uint32_t stage = 0; stage < states; ++stage){
                cmdList.SetDescriptorTable(0, inputSRVs[stage % 2]);
                cmdList.SetDescriptorTable(1, outputUAVs[stage % 2]);
                cmdList.SetConstant(3, 0, stage);
                cmdList.Dispatch2D(width, height, 1, 1);
                cmdList.InsertUAVBarrier(outputUAVs[stage % 2] == m_FFTOutputUAV ? 
                    m_FFTOutputTex : m_FFTTmpTex, true);
            }


            if((states % 2) == 0){
                std::swap(inputSRVs[0], inputSRVs[1]);
            }
            if((states % 2) == 0){
                std::swap(outputUAVs[0], outputUAVs[1]);
            }
            // cmdList.SetRootSignature(m_RootSig);
            // cmdList.SetPipelineState(m_FFTVerticPSO);

            // cmdList.SetDescriptorTable(0, inputSRVs[1]);
            // cmdList.SetDescriptorTable(1, outputUAVs[1]);
            // if (m_EnabledDebugOutput) {
            //     cmdList.SetDescriptorTable(2, m_FFTDebugUAV);
            // }

            // cmdList.Dispatch2D(height, width, sm_ThreadSize, 1);
            // // 垂直FFT
            // cmdList.SetPipelineState(m_FFTVerticBitReversedPSO);
            // cmdList.SetDescriptorTable(0, inputSRVs[1]);
            // cmdList.SetDescriptorTable(1, outputUAVs[1]);
            // cmdList.SetShaderResource(2, m_VerticReverseIndicesBuffer);
            // cmdList.Dispatch2D(width, height, 1, 1);
            // cmdList.InsertUAVBarrier(outputUAVs[1] == m_FFTOutputUAV ? 
            //     m_FFTOutputTex : m_FFTTmpTex, true);

            // cmdList.SetPipelineState(m_FFTVerticPSO);
            // cmdList.SetShaderResource(2, m_VerticReverseIndicesBuffer);
            // states = std::log2(Math::NextPowerOf2(m_FFTOutputTex.GetHeight()));
            // for(uint32_t stage = 0; stage < states; ++stage){
            //     cmdList.SetDescriptorTable(0, inputSRVs[stage % 2]);
            //     cmdList.SetDescriptorTable(1, outputUAVs[stage % 2]);
            //     cmdList.SetConstant(3, 0, stage);
            //     if(m_EnabledDebugOutput){
            //         cmdList.SetDescriptorTable(4, m_FFTDebugUAV);
            //     }
            //     cmdList.Dispatch2D(width, height, 1, 1);
            //     cmdList.InsertUAVBarrier(outputUAVs[stage % 2] == m_FFTOutputUAV ?
            //         m_FFTOutputTex : m_FFTTmpTex, true);
            // }

            cmdList.ExecuteCommandList(true);
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

            TextureDesc fftTmpDesc = fftOutputDesc;
            m_FFTTmpTex.Create(L"FFT Temporary Texture", fftTmpDesc);
            m_FFTTmpTex.CreateUnorderedAccessView(m_FFTTmpUAV);
            m_FFTTmpTex.CreateShaderResourceView(m_FFTTmpSRV);

            if(m_EnabledDebugOutput){
                TextureDesc fftDebugDesc = fftOutputDesc;
                fftDebugDesc.m_Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                m_FFTDebugTex.Create(L"FFT Debug Texture", fftDebugDesc);
                m_FFTDebugTex.CreateShaderResourceView(m_FFTDebugSRV);
                m_FFTDebugTex.CreateUnorderedAccessView(m_FFTDebugUAV);
            }
        }

        DescriptorHandle GetFFTOutputSRV() const noexcept { return m_FFTOutputSRV; }
        DescriptorHandle GetFFTDebugSRV() const noexcept { return m_FFTDebugSRV; }

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


    private:
        static constexpr uint32_t sm_ThreadSize = 256;

        bool m_EnabledDebugOutput = true;

        // 位反转后的索引
        GpuBuffer m_HorizReverseIndicesBuffer{};
        GpuBuffer m_VerticReverseIndicesBuffer{};

        Texture m_FFTOutputTex;
        DescriptorHandle m_FFTOutputUAV{};
        DescriptorHandle m_FFTOutputSRV{};

        Texture m_FFTTmpTex;
        DescriptorHandle m_FFTTmpUAV{};
        DescriptorHandle m_FFTTmpSRV{};

        Texture m_FFTDebugTex;
        DescriptorHandle m_FFTDebugSRV{};
        DescriptorHandle m_FFTDebugUAV{};

        RootSignature m_FFTRootSig;
        // RootSignature m_RootSig{3, 0};

        ComputePSO m_FFTHorizPSO;
        ComputePSO m_FFTHorizBitReversedPSO;
        ComputePSO m_FFTVerticPSO;
        ComputePSO m_FFTVerticBitReversedPSO;
        ComputePSO m_FFTConvertDataPSO;
    };
}


#endif  // __FFT_H__