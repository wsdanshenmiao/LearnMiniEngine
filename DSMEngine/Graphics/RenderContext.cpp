#include "RenderContext.h"

using Microsoft::WRL::ComPtr;

namespace DSM {
    RenderContext::RenderContext() noexcept
        :m_GraphicsQueue(D3D12_COMMAND_LIST_TYPE_DIRECT),
        m_ComputeQueue(D3D12_COMMAND_LIST_TYPE_COMPUTE),
        m_CopyQueue(D3D12_COMMAND_LIST_TYPE_COPY){
    }

    void RenderContext::Create(bool requireDXRSupport)
    {
        // TODO:创建工厂及设备
        ComPtr<ID3D12Device5> pDevice{};
        DWORD factoryFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
        // 开启调试层
        ComPtr<ID3D12Debug> pDebug{};
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(pDebug.GetAddressOf())))) {
            pDebug->EnableDebugLayer();
            ComPtr<ID3D12Debug1> pDebug1{};
            if (SUCCEEDED(pDebug->QueryInterface(IID_PPV_ARGS(pDebug1.GetAddressOf())))) {
                pDebug1->SetEnableGPUBasedValidation(true);
            }
        }
        else {
            Utility::Print("Warnint:    Failed to get D3D12 debug interface");
        }

        ComPtr<IDXGIInfoQueue> dxgiInfoQueue;
        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(dxgiInfoQueue.GetAddressOf()))))
        {
            factoryFlags = DXGI_CREATE_FACTORY_DEBUG;

            dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
            dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);

            DXGI_INFO_QUEUE_MESSAGE_ID hide[] =
            {
                80 /* IDXGISwapChain::GetContainingOutput: The swapchain's adapter does not control the output on which the swapchain's window resides. */,
            };
            DXGI_INFO_QUEUE_FILTER filter = {};
            filter.DenyList.NumIDs = _countof(hide);
            filter.DenyList.pIDList = hide;
            dxgiInfoQueue->AddStorageFilterEntries(DXGI_DEBUG_DXGI, &filter);
        }
#endif
        // 创建工厂
        ASSERT_SUCCEEDED(CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(m_pFactory.GetAddressOf())));
        

        // 初始化命令列表
        m_GraphicsQueue.Create(m_pDevice.Get());
        m_ComputeQueue.Create(m_pDevice.Get());
        m_CopyQueue.Create(m_pDevice.Get());
    }

    void RenderContext::Shutdown()
    {
        m_pFactory = nullptr;
        m_pDevice = nullptr;
        
        m_GraphicsQueue.Shutdown();
        m_ComputeQueue.Shutdown();
        m_CopyQueue.Shutdown();
    }

    CommandQueue& DSM::RenderContext::GetCommandQueue(D3D12_COMMAND_LIST_TYPE listType) noexcept
    {
        switch (listType) {
            case D3D12_COMMAND_LIST_TYPE_COMPUTE: return m_ComputeQueue;
            case D3D12_COMMAND_LIST_TYPE_COPY: return m_CopyQueue;
            default:return m_GraphicsQueue;
        }
    }
}
