#pragma once
#ifndef __PIPELINESTATE_H__
#define __PIPELINESTATE_H__

#include "../pch.h"


namespace DSM {
    class RootSignature;
    
    class PSO
    {
    public:
        PSO(const std::wstring& name)
            :m_Name(name), m_pRootSignature(nullptr), m_pPSO(nullptr){}

        const RootSignature& GetRootSignature() const
        {
            ASSERT(m_pRootSignature != nullptr);
            return *m_pRootSignature;
        }

        ID3D12PipelineState* GetPipelineStateObject() const noexcept { return m_pPSO; }

        void SetRootSignature(const RootSignature& rootSignature) noexcept { m_pRootSignature = &rootSignature; }


        static void DestroyAll() noexcept;

    protected:
        const std::wstring m_Name;
        const RootSignature* m_pRootSignature;
        ID3D12PipelineState* m_pPSO;
    };


    class GraphicsPSO : public PSO
    {
    public:
        GraphicsPSO(const std::wstring& name = L"Unnamed GraphicsPSO");

        void SetBlendState( const D3D12_BLEND_DESC& blendDesc );
        void SetRasterizerState( const D3D12_RASTERIZER_DESC& rasterizerDesc );
        void SetDepthStencilState( const D3D12_DEPTH_STENCIL_DESC& depthStencilDesc );
        void SetSampleMask( std::uint32_t sampleMask );
        void SetPrimitiveTopologyType( D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType );
        void SetDepthTargetFormat( DXGI_FORMAT dsvFormat, std::uint32_t msaaCount = 1, std::uint32_t msaaQuality = 0 );
        void SetRenderTargetFormat(
            DXGI_FORMAT rtvFormats,
            DXGI_FORMAT dsvFormat,
            std::uint32_t msaaCount = 1,
            std::uint32_t msaaQuality = 0 );
        void SetRenderTargetFormats(
            std::uint32_t numRTVs,
            const DXGI_FORMAT* rtvFormats,
            DXGI_FORMAT dsvFormat,
            std::uint32_t msaaCount = 1,
            std::uint32_t msaaQuality = 0 );
        void SetInputLayout( std::uint32_t numElements, const D3D12_INPUT_ELEMENT_DESC* pInputElementDescs );
        void SetPrimitiveRestart( D3D12_INDEX_BUFFER_STRIP_CUT_VALUE ibProps );

        void SetVertexShader(const void* binary, std::size_t size){ m_PSODesc.VS = {binary, size}; }
        void SetHullShader(const void* binary, std::size_t size){ m_PSODesc.HS = {binary, size}; }
        void SetDomainShader(const void* binary, std::size_t size){ m_PSODesc.DS = {binary, size}; }
        void SetGeometryShader(const void* binary, std::size_t size){ m_PSODesc.GS = {binary, size}; }
        void SetPixelShader(const void* binary, std::size_t size){ m_PSODesc.PS = {binary, size}; }

        void SetVertexShader(const D3D12_SHADER_BYTECODE& bytecode) { m_PSODesc.VS = bytecode; }
        void SetHullShader(const D3D12_SHADER_BYTECODE& bytecode) { m_PSODesc.HS = bytecode; }
        void SetDomainShader(const D3D12_SHADER_BYTECODE& bytecode) { m_PSODesc.DS = bytecode; }
        void SetGeometryShader(const D3D12_SHADER_BYTECODE& bytecode) { m_PSODesc.GS = bytecode; }
        void SetPixelShader(const D3D12_SHADER_BYTECODE& bytecode) { m_PSODesc.PS = bytecode; }

        void Finalize();

    private:
        D3D12_GRAPHICS_PIPELINE_STATE_DESC m_PSODesc{};
        std::vector<D3D12_INPUT_ELEMENT_DESC> m_InputLayouts{};
    };

    class ComputePSO : public PSO
    {
    public:
        ComputePSO(const std::wstring& name = L"Unnamed ComputePSO");

        void SetComputeShader(const void* binary, std::size_t size) { m_PSODesc.CS = {binary, size}; };
        void SetComputeShader(const D3D12_SHADER_BYTECODE& binary) { m_PSODesc.CS = binary; }

        void Finalize();

    private:
        D3D12_COMPUTE_PIPELINE_STATE_DESC m_PSODesc{};
    };
}

#endif

