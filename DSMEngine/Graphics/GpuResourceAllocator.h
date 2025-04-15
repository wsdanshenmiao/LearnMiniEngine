#pragma once
#ifndef __GPURESOURCEALLOCATOR_H__
#define __GPURESOURCEALLOCATOR_H__

#include "GpuResource.h"
#include "../Utilities/Hash.h"
#include "../Utilities/LinearAllocator.h"

namespace DSM {
    
    struct DSMHeapDesc
    {
        D3D12_HEAP_TYPE m_HeapType{};
        D3D12_HEAP_FLAGS m_HeapFlags = D3D12_HEAP_FLAG_NONE;
        std::strong_ordering operator<=>(const DSMHeapDesc& rhs) const = default;
    };


    // 用于管理一个堆的内存分配,不负责管理资源的释放
    class GpuResourcePage
    {
        friend class GpuResourceAllocator;
    public:
        GpuResourcePage(ID3D12Heap* agentHeap)
            :m_Heap(agentHeap), m_Allocator(agentHeap->GetDesc().SizeInBytes){}
        ~GpuResourcePage() = default;
        DSM_NONCOPYABLE(GpuResourcePage)

        ID3D12Resource* Allocate(const D3D12_RESOURCE_DESC& resourceDesc, D3D12_RESOURCE_STATES resourceState);
        bool ReleaseResource(ID3D12Resource* resource);
        void Reset() noexcept
        {
            m_SubResources.clear();
            m_Allocator.Clear();
        }
        std::size_t GetSubresourcesCount() const noexcept{ return m_SubResources.size(); }
        
    private:
        Microsoft::WRL::ComPtr<ID3D12Heap> m_Heap{};
        std::set<ID3D12Resource*> m_SubResources{};
        LinearAllocator m_Allocator;
    };

    // 用于管理所有的资源分配
    class GpuResourceAllocator
    {
    public:
        GpuResourceAllocator() = default;
        ~GpuResourceAllocator() { ShutDown(); };
        DSM_NONCOPYABLE(GpuResourceAllocator);

        void Create(DSMHeapDesc heapDesc, std::uint64_t heapSize = DEFAULT_PLACED_RESOURCE_PAGE_SIZE) noexcept;
        void ShutDown();

        ID3D12Resource* CreateResource(
            const D3D12_RESOURCE_DESC& resourceDesc,
            D3D12_RESOURCE_STATES resourceState);
        void ReleaseResource(ID3D12Resource* resource);
        
        ID3D12Heap* CreateNewHeap(std::uint64_t heapSize = 0);
        

    private:
        GpuResourcePage* RequestPage();

    private:
        DSMHeapDesc m_HeapDesc{};
        
        std::vector<std::unique_ptr<GpuResourcePage>> m_PagePool{};
        
        GpuResourcePage* m_CurrPage{};
        std::set<GpuResourcePage*> m_FullPages{};
        // 建立各个资源与分配者的映射关系，便于快速索引
        std::map<ID3D12Resource*, GpuResourcePage*> m_ResourceMappings{};
        // 可重复使用的资源
        std::queue<GpuResourcePage*> m_AvailablePages{};
        
        std::uint64_t m_HeapSize{};
        
        std::mutex m_Mutex{};
    };

}


#endif

