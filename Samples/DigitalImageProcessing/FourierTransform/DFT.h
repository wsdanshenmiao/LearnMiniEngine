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
        void Initialize(std::uint32_t width, std::uint32_t height)
        {
            Resize(width, height);

            m_RootSig[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1);
            m_RootSig[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1);
            m_RootSig.Finalize(L"DFT Root Signature");

            m_DFTPSO.SetRootSignature(m_RootSig);
            ShaderByteCode dftCS{ShaderDesc{
                .m_Type = ShaderType::Compute,
                .m_Mode = ShaderMode::SM_6_6,
                .m_FileName = "Shaders/FourierTransform/DFT.hlsl",
                .m_EnterPoint = "LuminanceDFTCS"
            }};
            m_DFTPSO.SetComputeShader(dftCS);
            m_DFTPSO.Finalize();

            m_IDFTPSO.SetRootSignature(m_RootSig);
            ShaderByteCode idftCS{ShaderDesc{
                .m_Type = ShaderType::Compute,
                .m_Mode = ShaderMode::SM_6_6,
                .m_FileName = "Shaders/FourierTransform/DFT.hlsl",
                .m_EnterPoint = "LuminanceIDFTCS"
            }};
            m_IDFTPSO.SetComputeShader(idftCS);
            m_IDFTPSO.Finalize();
        }

        void ExecuteDFT(class ComputeCommandList& cmdList, const Texture& inputTex, DescriptorHandle inputTexHandle)
        {
            cmdList.SetRootSignature(m_RootSig);
            cmdList.SetPipelineState(m_DFTPSO);

            cmdList.SetDescriptorTable(0, inputTexHandle);
            cmdList.SetDescriptorTable(1, m_DFTOutputUAV);

            size_t threadCountX = inputTex.GetWidth();
            size_t threadCountY = inputTex.GetHeight();
            cmdList.Dispatch2D(threadCountX, threadCountY, 1, 1);
            cmdList.ExecuteCommandList(true);
        }

        void ExecuteIDFT(class ComputeCommandList& cmdList, Texture& inputTex, DescriptorHandle inputTexHandle)
        {
            cmdList.SetRootSignature(m_RootSig);
            cmdList.SetPipelineState(m_IDFTPSO);

            cmdList.SetDescriptorTable(0, inputTexHandle);
            cmdList.SetDescriptorTable(1, m_IDFTOutputUAV);

            size_t threadCountX = inputTex.GetWidth();
            size_t threadCountY = inputTex.GetHeight();
            cmdList.Dispatch2D(threadCountX, threadCountY, 1, 1);
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
            m_DFTOutputUAV = g_Renderer.m_TextureHeap.Allocate();
            m_DFTOutputTex.CreateUnorderedAccessView(m_DFTOutputUAV);

            texDesc.m_Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
            m_IDFTOutputTex.Create(L"IDFT Output Texture", texDesc);
            m_IDFTOutputUAV = g_Renderer.m_TextureHeap.Allocate();
            m_IDFTOutputTex.CreateUnorderedAccessView(m_IDFTOutputUAV);
        }

        const Texture& GetDFTOutputTex() const { return m_DFTOutputTex; }
        Texture& GetDFTOutputTex() { return m_DFTOutputTex; }
        DescriptorHandle GetDFTUAV() const { return m_DFTOutputUAV; }

        const Texture& GetIDFTOutputTex() const { return m_IDFTOutputTex; }
        Texture& GetIDFTOutputTex() { return m_IDFTOutputTex; }
        DescriptorHandle GetIDFTUAV() const { return m_IDFTOutputUAV; }

    private:
        Texture m_DFTOutputTex{};
        DescriptorHandle m_DFTOutputUAV;

        Texture m_IDFTOutputTex{};
        DescriptorHandle m_IDFTOutputUAV;

        RootSignature m_RootSig{2, 0};
        ComputePSO m_DFTPSO;
        ComputePSO m_IDFTPSO;
    };
}


#endif // !__DFT_H__