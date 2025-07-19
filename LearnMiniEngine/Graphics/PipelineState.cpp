#include "PipelineState.h"
#include "RenderContext.h"
#include "RootSignature.h"
#include "../Utilities/Hash.h"

using Microsoft::WRL::ComPtr;

namespace DSM {
    
    static std::map<std::size_t, Microsoft::WRL::ComPtr<ID3D12PipelineState>> s_GraphicsPSOs{};
    static std::map<std::size_t, Microsoft::WRL::ComPtr<ID3D12PipelineState>> s_ComputePSOs{};


    void PSO::DestroyAll() noexcept
    {
        s_GraphicsPSOs.clear();
        s_ComputePSOs.clear();
    }


    
    GraphicsPSO::GraphicsPSO(const std::wstring& name)
        :PSO(name){
        ZeroMemory(&m_PSODesc, sizeof(m_PSODesc));
        m_PSODesc.NodeMask = 1;
        m_PSODesc.SampleMask = 0xFFFFFFFFu;
        m_PSODesc.SampleDesc.Count = 1;
        m_PSODesc.InputLayout.NumElements = 0;
    }

    
    void GraphicsPSO::SetBlendState( const D3D12_BLEND_DESC& blendDesc )
    {
        m_PSODesc.BlendState = blendDesc;
    }

    void GraphicsPSO::SetRasterizerState( const D3D12_RASTERIZER_DESC& rasterizerDesc )
    {
        m_PSODesc.RasterizerState = rasterizerDesc;
    }

    void GraphicsPSO::SetDepthStencilState( const D3D12_DEPTH_STENCIL_DESC& depthStencilDesc )
    {
        m_PSODesc.DepthStencilState = depthStencilDesc;
    }

    void GraphicsPSO::SetSampleMask( std::uint32_t sampleMask )
    {
        m_PSODesc.SampleMask = sampleMask;
    }

    void GraphicsPSO::SetPrimitiveTopologyType( D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType )
    {
        ASSERT(topologyType != D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED, "Can't draw with undefined topology");
        m_PSODesc.PrimitiveTopologyType = topologyType;
    }

    void GraphicsPSO::SetDepthTargetFormat(DXGI_FORMAT dsvFormat, std::uint32_t msaaCount, std::uint32_t msaaQuality)
    {
        SetRenderTargetFormats(0, nullptr, dsvFormat, msaaCount, msaaQuality);
    }

    void GraphicsPSO::SetRenderTargetFormat(DXGI_FORMAT rtvFormat, DXGI_FORMAT dsvFormat, std::uint32_t msaaCount, std::uint32_t msaaQuality)
    {
        SetRenderTargetFormats(1, &rtvFormat, dsvFormat, msaaCount, msaaQuality);
    }

    void GraphicsPSO::SetRenderTargetFormats(
        std::uint32_t numRTVs,
        const DXGI_FORMAT* rtvFormats,
        DXGI_FORMAT dsvFormat,
        std::uint32_t msaaCount,
        std::uint32_t msaaQuality)
    {
        ASSERT(numRTVs == 0 || rtvFormats != nullptr);

        for (std::uint32_t i = 0; i < numRTVs; ++i) {
            ASSERT(rtvFormats[i] != DXGI_FORMAT_UNKNOWN);
            m_PSODesc.RTVFormats[i] = rtvFormats[i];
        }
        for (std::uint32_t i = numRTVs; i < m_PSODesc.NumRenderTargets; ++i) {
            m_PSODesc.RTVFormats[i] = DXGI_FORMAT_UNKNOWN;
        }
        m_PSODesc.NumRenderTargets = numRTVs;
        m_PSODesc.SampleDesc.Count = msaaCount;
        m_PSODesc.SampleDesc.Quality = msaaQuality;
        m_PSODesc.DSVFormat = dsvFormat;
    }

    void GraphicsPSO::SetInputLayout(std::span<const D3D12_INPUT_ELEMENT_DESC> inputElements)
    {
        m_PSODesc.InputLayout.NumElements = inputElements.size();

        if (inputElements.size() > 0) {
            m_InputLayouts.resize(inputElements.size());
            memcpy(m_InputLayouts.data(), inputElements.data(), inputElements.size() * sizeof(D3D12_INPUT_ELEMENT_DESC));
        }
        else {
            m_InputLayouts.clear();
        }
    }

    void GraphicsPSO::SetPrimitiveRestart( D3D12_INDEX_BUFFER_STRIP_CUT_VALUE ibProps )
    {
        m_PSODesc.IBStripCutValue = ibProps;
    }

    void GraphicsPSO::Finalize()
    {
        m_PSODesc.pRootSignature = m_pRootSignature->GetRootSignature();
        ASSERT(m_PSODesc.pRootSignature != nullptr);

        // 不将地址 Hash
        m_PSODesc.InputLayout.pInputElementDescs = nullptr;
        auto hash = Utility::HashState(&m_PSODesc);
        hash = Utility::HashState(m_InputLayouts.data(), m_InputLayouts.size(), hash);
        m_PSODesc.InputLayout.pInputElementDescs = m_InputLayouts.size() == 0 ? nullptr : m_InputLayouts.data();

        // 与根签名类似
        bool firstCompile = false;
        ID3D12PipelineState** ppPSO = nullptr;
        {
            static std::mutex psoMutex{};
            // 加锁互斥锁
            std::lock_guard{psoMutex};
            
            if (auto it = s_GraphicsPSOs.find(hash); it != s_GraphicsPSOs.end()) {
                ppPSO = it->second.GetAddressOf();
            }
            else {
                ppPSO = s_GraphicsPSOs[hash].GetAddressOf();
                firstCompile = true;
            }
        }

        if (firstCompile) {
            ASSERT(m_PSODesc.DepthStencilState.DepthEnable != (m_PSODesc.DSVFormat == DXGI_FORMAT_UNKNOWN));
            ASSERT_SUCCEEDED(g_RenderContext.GetDevice()->CreateGraphicsPipelineState(&m_PSODesc, IID_PPV_ARGS(&m_pPSO)));
            s_GraphicsPSOs[hash].Attach(m_pPSO);
            ASSERT(m_pPSO == *ppPSO);
            m_pPSO->SetName(m_Name.c_str());
        }
        else {
            while (*ppPSO == nullptr) {
                std::this_thread::yield();
            }
            m_pPSO = *ppPSO;
        }
    }

    void ComputePSO::Finalize()
    {
        m_PSODesc.pRootSignature = m_pRootSignature->GetRootSignature();
        ASSERT(m_PSODesc.pRootSignature != nullptr);

        // 不将地址 Hash
        auto hash = Utility::HashState(&m_PSODesc);

        // 与根签名类似
        bool firstCompile = false;
        ID3D12PipelineState** ppPSO = nullptr;
        {
        static std::mutex psoMutex{};
        // 加锁互斥锁
        std::lock_guard<std::mutex>{psoMutex};
            
        if (auto it = s_ComputePSOs.find(hash); it != s_ComputePSOs.end()) {
            ppPSO = it->second.GetAddressOf();
        }
        else {
            ppPSO = s_ComputePSOs[hash].GetAddressOf();
            firstCompile = true;
        }
        }

        if (firstCompile) {
            ASSERT_SUCCEEDED(g_RenderContext.GetDevice()->CreateComputePipelineState(&m_PSODesc, IID_PPV_ARGS(&m_pPSO)));
            s_ComputePSOs[hash].Attach(m_pPSO);
            ASSERT(m_pPSO == *ppPSO);
            m_pPSO->SetName(m_Name.c_str());
        }
        else {
            while (*ppPSO == nullptr) {
                std::this_thread::yield();
            }
            m_pPSO = *ppPSO;
        }
    }
}
