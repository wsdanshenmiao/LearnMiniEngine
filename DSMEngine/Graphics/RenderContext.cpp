#include "RenderContext.h"
#include "GraphicsCommon.h"

using Microsoft::WRL::ComPtr;

namespace DSM {
    RenderContext::RenderContext() noexcept
        :m_GraphicsQueue(D3D12_COMMAND_LIST_TYPE_DIRECT),
        m_ComputeQueue(D3D12_COMMAND_LIST_TYPE_COMPUTE),
        m_CopyQueue(D3D12_COMMAND_LIST_TYPE_COPY){
    }

    void RenderContext::Create(bool requireDXRSupport)
    {
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

        // 枚举适配器并创建设备
        ComPtr<IDXGIAdapter1> dxgiAdapter;
        SIZE_T maxSize{};
        for (std::uint32_t i = 0; DXGI_ERROR_NOT_FOUND != m_pFactory->EnumAdapters1(i, &dxgiAdapter); ++i) {
            DXGI_ADAPTER_DESC1 dxgiDesc{};
            dxgiAdapter->GetDesc1(&dxgiDesc);

            // 跳过软适配器
            if (dxgiDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
                continue;
            }

            // 不能创建设备则跳过
            if (FAILED(D3D12CreateDevice(dxgiAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(m_pDevice.GetAddressOf())))) {
                continue;
            }

            // 是否支持光追
            if (requireDXRSupport && !Graphics::IsDirectXRaytracingSupported(m_pDevice.Get())) {
                continue;
            }

            // 使用内存最大的适配器
            if (dxgiDesc.DedicatedVideoMemory < maxSize) {
                continue;
            }
            maxSize = dxgiDesc.DedicatedVideoMemory;

            Utility::Print(L"Selected GPU:  {} ({} MB)\n", dxgiDesc.Description, dxgiDesc.DedicatedVideoMemory >> 20);
        }

        if (requireDXRSupport && m_pDevice == nullptr) {
            Utility::Print("Unable to find a DXR-capable device. Halting.\n");
            __debugbreak();
        }

        // 硬件不支持则使用软适配器
        if (m_pDevice == nullptr) {
            Utility::Print("Failed to find a hardware adapter.  Falling back to WARP.\n");
            ASSERT_SUCCEEDED(m_pFactory->EnumWarpAdapter(IID_PPV_ARGS(dxgiAdapter.GetAddressOf())));
            ASSERT_SUCCEEDED(D3D12CreateDevice(dxgiAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(m_pDevice.GetAddressOf())));
        }

        D3D12_FEATURE_DATA_D3D12_OPTIONS featureData = {};
        if (SUCCEEDED(m_pDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &featureData, sizeof(featureData)))) {
            if (featureData.TypedUAVLoadAdditionalFormats) {
                D3D12_FEATURE_DATA_FORMAT_SUPPORT support = {
                    DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_FORMAT_SUPPORT1_NONE, D3D12_FORMAT_SUPPORT2_NONE};

                if (m_pDevice->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &support, sizeof(support)) &&
                    support.Support2 & D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD != 0) {
                    sm_bTypedUAVLoadSupport_R11G11B10_FLOAT = true;
                }
                support.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
                if (m_pDevice->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &support, sizeof(support)) &&
                    support.Support2 & D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD != 0) {
                    sm_bTypedUAVLoadSupport_R16G16B16A16_FLOAT = true;
                }
            }
        }

        // 初始化命令队列
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
