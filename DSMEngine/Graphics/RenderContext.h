#pragma once
#ifndef __RENDERCONTEXT_H__
#define __RENDERCONTEXT_H__

#include "../pch.h"
#include "../Utilities/Singleton.h"
#include "CommandQueue.h"
#include "Resource/DynamicBufferAllocator.h"
#include "SwapChain.h"

namespace DSM {
    class Window;
    
    class RenderContext : public Singleton<RenderContext>
    {
    public:
        RenderContext();
        ~RenderContext()
        {
            Shutdown();
        }

        void Create(bool requireDXRSupport, const Window& window);
        void Shutdown();

        void OnResize(std::uint32_t width, std::uint32_t height);
        
        ID3D12Device5* GetDevice() const{return m_pDevice.Get();}
        IDXGIFactory7* GetFactory() const{return m_pFactory.Get();}

        CommandQueue& GetCommandQueue(D3D12_COMMAND_LIST_TYPE listType = D3D12_COMMAND_LIST_TYPE_DIRECT) noexcept;
        CommandQueue& GetGraphicsQueue() noexcept { return m_GraphicsQueue; }
        CommandQueue& GetComputeQueue() noexcept { return m_ComputeQueue; }
        CommandQueue& GetCopyQueue() noexcept { return m_CopyQueue; }

        SwapChain& GetSwapChain() noexcept { return *m_SwapChain; }

        DynamicBufferAllocator& GetCpuBufferAllocator() noexcept { return m_CpuBufferAllocator; }
        DynamicBufferAllocator& GetGpuBufferAllocator() noexcept { return m_GpuBufferAllocator; }

        DescriptorHandle AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE heapType, std::uint32_t count = 1);
        void FreeDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE heapType, DescriptorHandle descriptor, std::uint32_t count = 1);

        void CreateCommandList(
            D3D12_COMMAND_LIST_TYPE listType,
            ID3D12GraphicsCommandList** ppList,
            ID3D12CommandAllocator** ppAllocator);

        void IdleGPU();
        
        bool IsFenceComplete(std::uint64_t fenceValue) noexcept
        {
            return GetCommandQueue(D3D12_COMMAND_LIST_TYPE(fenceValue >> QUEUE_TYPE_MOVEBITS)).IsFenceComplete(fenceValue);
        }

        void CleanupDynamicBuffer(std::uint64_t fenceValue)
        {
            m_CpuBufferAllocator.Cleanup(fenceValue);
            m_GpuBufferAllocator.Cleanup(fenceValue);
        }

        void ExecuteCommandList(CommandList* cmdList, bool waitForCompletion = false);

    public:
        inline static bool sm_bTypedUAVLoadSupport_R11G11B10_FLOAT = false;
        inline static bool sm_bTypedUAVLoadSupport_R16G16B16A16_FLOAT = false;

        inline static constexpr std::uint64_t sm_GpuAllocatorPageSize = DEFAULT_BUFFER_PAGE_SIZE / 2;
        inline static constexpr std::uint64_t sm_CpuBufferPageSize = 0x200000;
        
    private:
        Microsoft::WRL::ComPtr<ID3D12Device5> m_pDevice{};
        Microsoft::WRL::ComPtr<IDXGIFactory7> m_pFactory{};

        CommandQueue m_GraphicsQueue;
        CommandQueue m_ComputeQueue;
        CommandQueue m_CopyQueue;

        std::unique_ptr<SwapChain> m_SwapChain;

        // 主要用于初始化默认资源
        DynamicBufferAllocator m_CpuBufferAllocator;
        // 主要用于常量缓冲区等创建在上传堆资源
        DynamicBufferAllocator m_GpuBufferAllocator;

        std::array<std::unique_ptr<DescriptorAllocator>, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> m_DescriptorAllocator;

    };

    

#define g_RenderContext	(RenderContext::GetInstance())
}

#endif

