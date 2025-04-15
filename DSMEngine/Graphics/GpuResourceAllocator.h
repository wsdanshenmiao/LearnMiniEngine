#pragma once
#ifndef __GPURESOURCEALLOCATOR_H__
#define __GPURESOURCEALLOCATOR_H__

#include "GpuResource.h"
#include "LinearAllocator.h"
#include "../Utilities/Hash.h"

namespace DSM {
    class LinearBufferAllocator;
    


    
    struct DSMHeapDesc
    {
        D3D12_HEAP_TYPE m_HeapType{};
        D3D12_HEAP_FLAGS m_HeapFlags = D3D12_HEAP_FLAG_NONE;
        std::strong_ordering operator<=>(const DSMHeapDesc& rhs) const = default;
    };
    
    // 管理缓冲区资源
    class GpuResourceAllocator
    {
    public:
        GpuResourceAllocator(std::uint64_t subResourceSize = DEFAULT_BUFFER_PAGE_SIZE)
            :m_SubResourceSize(subResourceSize){}
        DSM_NONCOPYABLE_NONMOVABLE(GpuResourceAllocator);
        
        // 使用 ManualSubAllocation 策略分配资源
        GpuResourceLocatioin CreateBuffer(std::uint64_t bufferSize, DSMBufferDesc bufferDesc);
        // 释放某个缓冲区
        void ReleaseBuffer(GpuResource* buffer, std::uint64_t fenceValue);
        

        // 使用 PlacedResource 策略分配资源
        GpuResourceLocatioin CreateResource(
            D3D12_HEAP_TYPE heapType,
            D3D12_RESOURCE_DESC resourceDesc,
            D3D12_RESOURCE_STATES resourceState,
            D3D12_HEAP_FLAGS heapFlags = D3D12_HEAP_FLAG_NONE);

        // 通过栅栏值清空使用完毕的资源
        void Cleanup(std::uint64_t fenceValue);
        
    private:
        std::uint64_t m_SubResourceSize{};
        
        std::unordered_map<DSMBufferDesc, LinearBufferAllocator> m_CommittedResourceAllocators{};
        
    };



    class PlacedResourceAllocator
    {
    public:
        PlacedResourceAllocator() = default;
        ~PlacedResourceAllocator() { ShutDown(); };
        DSM_NONCOPYABLE(PlacedResourceAllocator);

        void Create(DSMHeapDesc heapDesc, std::uint64_t heapSize = DEFAULT_BUFFER_PAGE_SIZE) noexcept
        {
            m_HeapDesc = heapDesc;
            m_HeapSize = heapSize;
        }
        void ShutDown();

        GpuResourceLocatioin Allocate(D3D12_RESOURCE_DESC resourceDesc, D3D12_RESOURCE_STATES resourceState);
        
        ID3D12Heap* RequestHeap();
        ID3D12Heap* CreateNewHeap(std::uint64_t heapSize = 0);
        
        void Cleanup(std::uint64_t fenceValue);

    private:
        DSMHeapDesc m_HeapDesc{};
        
        std::vector<Microsoft::WRL::ComPtr<ID3D12Heap>> m_HeapPool{};
        
        ID3D12Heap* m_CurrHeap{};
        std::vector<ID3D12Heap*> m_FullHeaps{};
        std::uint64_t m_CurrOffset{};
        
        // 等待使用完毕的资源
        std::queue<std::pair<std::uint64_t, ID3D12Heap*>> m_RetiredHeaps{};
        // 需要删除的资源
        std::queue<std::pair<std::uint64_t, ID3D12Heap*>> m_DeletionHeaps{};
        // 可重复使用的资源
        std::queue<ID3D12Heap*> m_AvailableHeaps{};
        
        std::uint64_t m_HeapSize{};
        
        std::mutex m_Mutex{};
    };



    /*
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
            D3D12_HEAP_FLAGS m_HeapFlags;           // 分配策略为 PlacedResource 时使用
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
            using RetiredResource = std::pair<std::uint64_t, GpuResource*>;
            GpuResource* m_CurrResource{};
            std::vector<std::unique_ptr<GpuResource>> m_ResourcePool{};
            std::vector<GpuResource*> m_FullResources{};
            std::map<D3D12_RESOURCE_FLAGS, std::queue<RetiredResource>> m_RetiredResources{};
            std::map<D3D12_RESOURCE_FLAGS, std::queue<GpuResource*>> m_AvailableResources{};
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
        GpuResourceLocatioin Allocate(
            std::uint64_t size,
            std::uint32_t alignment = 0,
            D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);
        void Cleanup(std::uint64_t fenceValue);

        AllocationStrategy GetAllocationStrategy() const noexcept { return m_InitData.m_Strategy; }
    
        
    private:
        GpuResourceLocatioin AllocateFormHeap(std::uint64_t bufferSize);
        GpuResourceLocatioin AllocateFormResource(
            std::uint64_t bufferSize,
            D3D12_RESOURCE_FLAGS flags);
        
        // 当前资源满的时候分配新的资源
        GpuResource* RequestResource(D3D12_RESOURCE_FLAGS flags);
        ID3D12Heap* RequestHeap();

        GpuResource* CreateNewResource(
            std::uint64_t size = DEFAULT_RESOURCE_POOL_SIZE,
            D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);
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
    */

}


#endif

