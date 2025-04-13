#pragma once
#ifndef __GPURESOURCEALLOCATOR_H__
#define __GPURESOURCEALLOCATOR_H__

#include "GpuResource.h"

namespace DSM {
    // 用于定位子资源在缓冲区中的位置
    struct GpuResourceLocatioin
    {
        GpuResource* m_Resource{};
        ID3D12Heap* m_Heap{};
        D3D12_GPU_VIRTUAL_ADDRESS m_GpuAddress{};
        void* m_MappedAddress{};
        std::uint64_t m_Offset{};
        std::uint64_t m_Size{};
    };

    
    // 用于统一管理资源
    class GpuResourceAllocator
    {
    public:
        // 分配策略，划分为子资源或是在堆上分配
        enum class AllocationStrategy
        {
            PlacedResource,
            ManualSubAllocation
        };
        
        struct AllocatorInitData
        {
            AllocationStrategy m_Strategy{};
            D3D12_HEAP_TYPE m_HeapType{};
            union
            {
                D3D12_HEAP_FLAGS m_HeapFlags;           // 分配策略为 PlacedResource 时使用
                D3D12_RESOURCE_FLAGS m_ResourceFlags;   // 分配策略为 ManualSubAllocation 时使用
            };
        };

    private:
        // 分配策略为 PlacedResource 时使用
        struct HeapData
        {
            ID3D12Heap* m_CurrHeap{};
            std::vector<Microsoft::WRL::ComPtr<ID3D12Heap>> m_HeapPool{};
            std::vector<ID3D12Heap*> m_FullHeaps{};
            std::queue<std::pair<std::uint64_t, ID3D12Heap*>> m_RetiredHeaps{};
            std::queue<ID3D12Heap*> m_AvailableHeaps{};
        };

        // 分配策略为 ManualSubAllocation 时使用
        struct ResourceData
        {
            GpuResource* m_CurrResource{};
            std::vector<std::unique_ptr<GpuResource>> m_ResourcePool{};
            std::vector<GpuResource*> m_FullResources{};
            std::queue<std::pair<std::uint64_t, GpuResource*>> m_RetiredResources{};
            std::queue<GpuResource*> m_AvailableResources{};
        };
        
    public:
        GpuResourceAllocator() = default;
        ~GpuResourceAllocator() { ShutDown(); };
        DSM_NONCOPYABLE(GpuResourceAllocator);

        void Create(const AllocatorInitData& initData,
            std::uint64_t subResourceSize = DEFAULT_RESOURCE_POOL_SIZE) noexcept
        {
            m_InitData = initData;
            m_SubResourceSize = subResourceSize;
        }
        void ShutDown() noexcept;
        
        // 请求资源
        GpuResourceLocatioin Allocate(std::uint64_t size, std::uint32_t alignment = 0);
        void Cleanup(std::uint64_t fenceValue);

        AllocationStrategy GetAllocationStrategy() const noexcept { return m_InitData.m_Strategy; }
    
        
    private:
        GpuResourceLocatioin AllocateFormHeap(std::uint64_t alignSize);
        GpuResourceLocatioin AllocateFormResource(std::uint64_t alignSize);
        
        // 当前资源满的时候分配新的资源
        GpuResource* RequestResource();
        ID3D12Heap* RequestHeap();

        GpuResource* CreateNewResource(std::uint64_t size = DEFAULT_RESOURCE_POOL_SIZE);
        ID3D12Heap* CreateNewHeap(std::uint64_t size = DEFAULT_RESOURCE_POOL_SIZE);

    private:
        AllocatorInitData m_InitData{};

        union
        {
            ResourceData m_ResourceData{};
            HeapData m_HeapData;
        };
        std::mutex m_Mutex{};
        
        std::uint64_t m_CurrOffset{};
        std::uint64_t m_SubResourceSize{};
    };

}


#endif

