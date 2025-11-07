#pragma once
#ifndef __DFT_H__
#define __DFT_H__

#include "Graphics/CommandList/ComputeCommandList.h"
#include "Graphics/RootSignature.h"
#include "Graphics/PipelineState.h"
#include "Graphics/ShaderCompiler.h"

namespace DSM{
    class DFT
    {
    public:
        DFT(bool enableDebug = true) 
            :m_EnableDebug(enableDebug), m_RootSig(enableDebug ? 3 : 2, 0) {}

        void Initialize(std::uint32_t width, std::uint32_t height)
        {
            m_DFTOutputUAV = g_Renderer.m_TextureHeap.Allocate();
            m_DFTOutputSRV = g_Renderer.m_TextureHeap.Allocate();
            m_DFTTmpUAV = g_Renderer.m_TextureHeap.Allocate();
            m_DFTTmpSRV = g_Renderer.m_TextureHeap.Allocate();

            m_IDFTOutputUAV = g_Renderer.m_TextureHeap.Allocate();
            m_IDFTOutputSRV = g_Renderer.m_TextureHeap.Allocate();
            m_IDFTTmpUAV = g_Renderer.m_TextureHeap.Allocate();
            m_IDFTTmpSRV = g_Renderer.m_TextureHeap.Allocate();

            if(m_EnableDebug){
                m_DFTDebugUAV = g_Renderer.m_TextureHeap.Allocate();
                m_DFTDebugSRV = g_Renderer.m_TextureHeap.Allocate();
            }
            Resize(width, height);

            m_RootSig[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1);
            m_RootSig[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1);
            if(m_EnableDebug){
                m_RootSig[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1);
            }
            m_RootSig.Finalize(L"DFT Root Signature");

            // DFT
            m_DFTHorizPSO.SetRootSignature(m_RootSig);
            ShaderDesc horizCSDesc{
                .m_Type = ShaderType::Compute,
                .m_Mode = ShaderMode::SM_6_6,
                .m_FileName = "Shaders/FourierTransform/DFT.hlsl",
                .m_EnterPoint = "LuminanceDFTCS"
            };
            ShaderByteCode horizDFTCS{horizCSDesc};
            m_DFTHorizPSO.SetComputeShader(horizDFTCS);
            m_DFTHorizPSO.Finalize();

            m_DFTVerticPSO.SetRootSignature(m_RootSig);
            auto verticCSDesc = horizCSDesc;
            verticCSDesc.m_EnterPoint = "LuminanceDFTCS";
            verticCSDesc.m_Defines.AddDefine("IS_VERTIC_DFT", "1");
            if(m_EnableDebug){
                verticCSDesc.m_Defines.AddDefine("ENABLE_DEBUG_OUTPUT", "1");
            }
            ShaderByteCode verticDFTCS{verticCSDesc};
            m_DFTVerticPSO.SetComputeShader(verticDFTCS);
            m_DFTVerticPSO.Finalize();

            // IDFT
            m_IDFTHorizPSO.SetRootSignature(m_RootSig);
            auto idftHorizCSDesc = horizCSDesc;
            idftHorizCSDesc.m_EnterPoint = "LuminanceIDFTCS";
            ShaderByteCode idftHorizCS{idftHorizCSDesc};
            m_IDFTHorizPSO.SetComputeShader(idftHorizCS);
            m_IDFTHorizPSO.Finalize();

            m_IDFTVerticPSO.SetRootSignature(m_RootSig);
            auto idftVerticCSDesc = horizCSDesc;
            idftVerticCSDesc.m_Defines.AddDefine("IS_VERTIC_DFT", "1");
            idftVerticCSDesc.m_EnterPoint = "LuminanceIDFTCS";
            ShaderByteCode idftVerticCS{idftVerticCSDesc};
            m_IDFTVerticPSO.SetComputeShader(idftVerticCS);
            m_IDFTVerticPSO.Finalize();
        }

        void ExecuteDFT(class ComputeCommandList& cmdList, const Texture& inputTex, DescriptorHandle inputTexHandle)
        {
            size_t threadCountX = inputTex.GetWidth();
            size_t threadCountY = inputTex.GetHeight();

            cmdList.SetRootSignature(m_RootSig);
            
            // 水平变换
            cmdList.SetPipelineState(m_DFTHorizPSO);

            cmdList.SetDescriptorTable(0, inputTexHandle);
            cmdList.SetDescriptorTable(1, m_DFTTmpUAV);

            cmdList.Dispatch2D(threadCountX, threadCountY, sm_ThreadSize, 1);
            cmdList.InsertUAVBarrier(m_DFTTmpTex, true);
            
            // 垂直变换
            cmdList.SetPipelineState(m_DFTVerticPSO);

            cmdList.SetDescriptorTable(0, m_DFTTmpSRV);
            cmdList.SetDescriptorTable(1, m_DFTOutputUAV);
            if (m_EnableDebug) {
                cmdList.SetDescriptorTable(2, m_DFTDebugUAV);
            }

            cmdList.Dispatch2D(threadCountY, threadCountX, sm_ThreadSize, 1);
            cmdList.ExecuteCommandList(true);
        }

        void ExecuteIDFT(class ComputeCommandList& cmdList, Texture& inputTex, DescriptorHandle inputTexHandle)
        {
            cmdList.SetRootSignature(m_RootSig);
            cmdList.SetPipelineState(m_IDFTHorizPSO);

            cmdList.SetDescriptorTable(0, inputTexHandle);
            cmdList.SetDescriptorTable(1, m_IDFTTmpUAV);

            size_t threadCountX = inputTex.GetWidth();
            size_t threadCountY = inputTex.GetHeight();
            cmdList.Dispatch2D(threadCountX, threadCountY, sm_ThreadSize, 1);
            cmdList.ExecuteCommandList(true);
            
            // 垂直变换
            cmdList.SetPipelineState(m_IDFTVerticPSO);

            cmdList.SetDescriptorTable(0, m_IDFTTmpSRV);
            cmdList.SetDescriptorTable(1, m_IDFTOutputUAV);

            cmdList.Dispatch2D(threadCountY, threadCountX, sm_ThreadSize, 1);
            cmdList.ExecuteCommandList(true);
        }
        
        void Resize(std::uint32_t width, std::uint32_t height)
        {
            TextureDesc texDesc{};
            texDesc.m_Width = width;
            texDesc.m_Height = height;
            texDesc.m_Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            texDesc.m_Format = DXGI_FORMAT_R32G32_FLOAT;
            texDesc.m_Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

            m_DFTOutputTex.Create(L"DFT Output Texture", texDesc);
            m_DFTTmpTex.Create(L"DFT Temp Texture", texDesc);

            m_DFTOutputTex.CreateUnorderedAccessView(m_DFTOutputUAV);
            m_DFTOutputTex.CreateShaderResourceView(m_DFTOutputSRV);
            m_DFTTmpTex.CreateUnorderedAccessView(m_DFTTmpUAV);
            m_DFTTmpTex.CreateShaderResourceView(m_DFTTmpSRV);

            if(m_EnableDebug){
                texDesc.m_Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                m_DFTDebugOutputTex.Create(L"DFT Debug Output Texture", texDesc);
                m_DFTDebugOutputTex.CreateUnorderedAccessView(m_DFTDebugUAV);
                m_DFTDebugOutputTex.CreateShaderResourceView(m_DFTDebugSRV);
            }

            texDesc.m_Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
            m_IDFTOutputTex.Create(L"IDFT Output Texture", texDesc);
            m_IDFTTmpTex.Create(L"IDFT Temp Texture", texDesc);

            m_IDFTOutputTex.CreateUnorderedAccessView(m_IDFTOutputUAV);
            m_IDFTOutputTex.CreateShaderResourceView(m_IDFTOutputSRV);
            m_IDFTTmpTex.CreateUnorderedAccessView(m_IDFTTmpUAV);
            m_IDFTTmpTex.CreateShaderResourceView(m_IDFTTmpSRV);
        }

        const Texture& GetDFTOutputTex() const { return m_DFTOutputTex; }
        Texture& GetDFTOutputTex() { return m_DFTOutputTex; }
        DescriptorHandle GetDFTUAV() const { return m_DFTOutputUAV; }
        DescriptorHandle GetDFTSRV() const { return m_DFTOutputSRV; }

        const Texture& GetIDFTOutputTex() const { return m_IDFTOutputTex; }
        Texture& GetIDFTOutputTex() { return m_IDFTOutputTex; }
        DescriptorHandle GetIDFTUAV() const { return m_IDFTOutputUAV; }
        DescriptorHandle GetIDFTSRV() const { return m_IDFTOutputSRV; }

        const Texture& GetDFTDebugOutputTex() const { return m_DFTDebugOutputTex; }
        Texture& GetDFTDebugOutputTex() { return m_DFTDebugOutputTex; }
        DescriptorHandle GetDFTDebugUAV() const { return m_DFTDebugUAV; }
        DescriptorHandle GetDFTDebugSRV() const { return m_DFTDebugSRV; }

    private:
        static constexpr uint32_t sm_ThreadSize = 512;

        const bool m_EnableDebug = true;

        Texture m_DFTOutputTex{};
        DescriptorHandle m_DFTOutputUAV;
        DescriptorHandle m_DFTOutputSRV;

        Texture m_DFTTmpTex{};
        DescriptorHandle m_DFTTmpUAV;
        DescriptorHandle m_DFTTmpSRV;

        Texture m_DFTDebugOutputTex{};
        DescriptorHandle m_DFTDebugUAV;
        DescriptorHandle m_DFTDebugSRV;

        Texture m_IDFTOutputTex{};
        DescriptorHandle m_IDFTOutputUAV;
        DescriptorHandle m_IDFTOutputSRV;

        Texture m_IDFTTmpTex{};
        DescriptorHandle m_IDFTTmpUAV;
        DescriptorHandle m_IDFTTmpSRV;

        RootSignature m_RootSig;
        ComputePSO m_DFTHorizPSO;
        ComputePSO m_DFTVerticPSO;
        ComputePSO m_IDFTHorizPSO;
        ComputePSO m_IDFTVerticPSO;
    };
}


#endif // !__DFT_H__