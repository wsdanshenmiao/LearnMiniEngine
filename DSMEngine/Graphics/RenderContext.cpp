#include "RenderContext.h"
#include "DynamicDescriptorHeap.h"
#include "CommandList/GraphicsCommandList.h"
#include "GraphicsCommon.h"
#include "RootSignature.h"
#include "SwapChain.h"
#include "../Core/Window.h"
#include <dxgidebug.h>

using Microsoft::WRL::ComPtr;

namespace DSM {
    RenderContext::RenderContext()
        :m_GraphicsQueue(D3D12_COMMAND_LIST_TYPE_DIRECT),
        m_ComputeQueue(D3D12_COMMAND_LIST_TYPE_COMPUTE),
        m_CopyQueue(D3D12_COMMAND_LIST_TYPE_COPY) {}

    void RenderContext::Create(bool requireDXRSupport, const Window& window)
    {
        DWORD factoryFlags = 0;
#if defined(DEBUG) || defined(_DEBUG) || 1
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

        m_CpuBufferAllocator.Create(DynamicBufferAllocator::AllocateMode::CpuExclusive, sm_CpuBufferPageSize);
        m_GpuBufferAllocator.Create(DynamicBufferAllocator::AllocateMode::GpuExclusive, sm_GpuAllocatorPageSize);

        for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i) {
            m_DescriptorAllocator[i] = std::make_unique<DescriptorAllocator>(static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(i));
        }
        
        SwapChainDesc swapChainDesc = {};
        swapChainDesc.m_Width = window.GetWidth();
        swapChainDesc.m_Height = window.GetHeight();
        swapChainDesc.m_Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.m_FullScreen = false;
        swapChainDesc.m_hWnd = window.GetHandle();
        m_SwapChain = std::make_unique<SwapChain>(swapChainDesc);

        Graphics::InitializeCommon();
    }

    void RenderContext::Shutdown()
    {
        Graphics::DestroyCommon();
        
        m_pFactory = nullptr;
        m_pDevice = nullptr;

        m_CpuBufferAllocator.Shutdown();
        m_GpuBufferAllocator.Shutdown();
        
        m_GraphicsQueue.Shutdown();
        m_ComputeQueue.Shutdown();
        m_CopyQueue.Shutdown();

        m_SwapChain = nullptr;

        DescriptorAllocator::DestroyAll();
    }

    void RenderContext::OnResize(std::uint32_t width, std::uint32_t height)
    {
        if (m_SwapChain != nullptr) {
            m_SwapChain->OnResize(width, height);
        }
    }

    CommandQueue& DSM::RenderContext::GetCommandQueue(D3D12_COMMAND_LIST_TYPE listType) noexcept
    {
        switch (listType) {
            case D3D12_COMMAND_LIST_TYPE_COMPUTE: return m_ComputeQueue;
            case D3D12_COMMAND_LIST_TYPE_COPY: return m_CopyQueue;
            default:return m_GraphicsQueue;
        }
    }

    DescriptorHandle RenderContext::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE heapType, std::uint32_t count)
    {
        return m_DescriptorAllocator[heapType]->AllocateDescriptor(count);
    }

    void RenderContext::FreeDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE heapType, DescriptorHandle descriptor, std::uint32_t count)
    {
        m_DescriptorAllocator[heapType]->FreeDescriptor(descriptor, count);
    }

    void RenderContext::CreateCommandList(
        D3D12_COMMAND_LIST_TYPE listType,
        ID3D12GraphicsCommandList** ppList,
        ID3D12CommandAllocator** ppAllocator)
    {
        ASSERT(ppList != nullptr && ppAllocator != nullptr);
        ASSERT(listType != D3D12_COMMAND_LIST_TYPE_BUNDLE);
        
        switch (listType) {
            case D3D12_COMMAND_LIST_TYPE_DIRECT: {
                *ppAllocator = GetGraphicsQueue().RequestCommandAllocator(); break;
            }
            case D3D12_COMMAND_LIST_TYPE_COMPUTE: {
                *ppAllocator = GetCommandQueue().RequestCommandAllocator(); break;
            }
            case D3D12_COMMAND_LIST_TYPE_COPY: {
                *ppAllocator = GetCopyQueue().RequestCommandAllocator(); break;
            }
            case D3D12_COMMAND_LIST_TYPE_BUNDLE: break;
        }

        ASSERT_SUCCEEDED(m_pDevice->CreateCommandList(
            1, listType, *ppAllocator, nullptr, IID_PPV_ARGS(ppList)));
    }

    void RenderContext::IdleGPU()
    {
        m_GraphicsQueue.WaitForIdle();
        m_ComputeQueue.WaitForIdle();
        m_CopyQueue.WaitForIdle();
    }

    void RenderContext::WaitForFence(uint64_t FenceValue)
    {
        CommandQueue& Producer = GetCommandQueue((D3D12_COMMAND_LIST_TYPE)(FenceValue >> 56));
        Producer.WaitForFence(FenceValue);
    }


}
