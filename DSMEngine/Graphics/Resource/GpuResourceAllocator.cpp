#include "GpuResourceAllocator.h"
#include "../RenderContext.h"

namespace DSM {
    //
    // GpuResourcePage Implementation
    //
    ID3D12Resource* GpuResourcePage::Allocate(const D3D12_RESOURCE_DESC& resourceDesc, D3D12_RESOURCE_STATES resourceState)
    {
        auto resourceSize = resourceDesc.Width * resourceDesc.Height * resourceDesc.DepthOrArraySize;
        auto offset = m_Allocator.Allocate(resourceSize, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
        ID3D12Resource* resource = nullptr;
        if (offset != Utility::INVALID_ALLOC_OFFSET) {
            ASSERT_SUCCEEDED(g_RenderContext.GetDevice()->CreatePlacedResource(
                m_Heap.Get(), offset, &resourceDesc,
                resourceState, nullptr, IID_PPV_ARGS(&resource)));
            m_SubResources.insert(resource);
        }
        return resource;
    }

    bool GpuResourcePage::ReleaseResource(ID3D12Resource* resource)
    {
        if (m_SubResources.contains(resource)) {
            m_SubResources.erase(resource);
            return true;
        }
        else{
            return false;
        }
    }




    //
    // GpuResourceAllocator Implementation
    //
    void GpuResourceAllocator::Create(DSMHeapDesc heapDesc, std::uint64_t heapSize) noexcept
    {
        std::lock_guard lock{m_Mutex};
        
        m_HeapDesc = heapDesc;
        m_HeapSize = heapSize;
        auto newHeap = CreateNewHeap();
        auto newPage = std::make_unique<GpuResourcePage>(newHeap);
        m_CurrPage = newPage.get();
        m_PagePool.emplace_back(std::move(newPage));
    }

    void GpuResourceAllocator::ShutDown()
    {
        std::lock_guard lock{m_Mutex};
        
        while (!m_AvailablePages.empty()) {
            m_AvailablePages.pop();
        }
        m_FullPages.clear();
        m_ResourceMappings.clear();
        m_CurrPage = nullptr;
        m_PagePool.clear();
    }

    ID3D12Resource* GpuResourceAllocator::CreateResource(
        const D3D12_RESOURCE_DESC& resourceDesc,
        D3D12_RESOURCE_STATES resourceState)
    {
        auto resourceSize = resourceDesc.Width * resourceDesc.Height * resourceDesc.DepthOrArraySize;
        
        std::lock_guard lock{m_Mutex};
        
        ID3D12Resource* resource = nullptr;
        if (resourceSize > m_HeapSize) {    // 过大或过小的资源不创建堆
            D3D12_HEAP_PROPERTIES prop = {};
            prop.Type = m_HeapDesc.m_HeapType;
            ASSERT_SUCCEEDED(g_RenderContext.GetDevice()->CreateCommittedResource(
                &prop,
                m_HeapDesc.m_HeapFlags,
                &resourceDesc,
                resourceState,
                nullptr,
                IID_PPV_ARGS(&resource)));
        }
        else {
            resource = m_CurrPage->Allocate(resourceDesc, resourceState);
            if (resource == nullptr) {
                m_FullPages.insert(m_CurrPage);
                m_CurrPage = RequestPage();
                resource = m_CurrPage->Allocate(resourceDesc, resourceState);
            }

            // 只对在堆中分配的资源进行映射
            m_ResourceMappings.insert(std::make_pair(resource, m_CurrPage));
        }

        return resource;
    }

    void GpuResourceAllocator::ReleaseResource(ID3D12Resource* resource)
    {
        ASSERT(resource != nullptr);
        
        std::lock_guard lock{m_Mutex};

        if (!m_CurrPage->ReleaseResource(resource) && m_ResourceMappings.contains(resource)) {
            auto it = m_FullPages.find(m_ResourceMappings[resource]);
            ASSERT((*it)->ReleaseResource(resource));
            if ((*it)->Empty()) {
                (*it)->Reset();
                m_AvailablePages.push(*it);
                m_FullPages.erase(it);
            }
            m_ResourceMappings.erase(resource);
        }
    }

    GpuResourcePage* GpuResourceAllocator::RequestPage()
    {
        GpuResourcePage* page = nullptr;
        // 清除已经完成的资源
        if (m_AvailablePages.empty()) {
            auto newHeap = CreateNewHeap();
            page = new GpuResourcePage{newHeap};
            m_PagePool.emplace_back(page);
        }
        else {
            page = m_AvailablePages.front();
            m_AvailablePages.pop();
        }
        
        return page;
    }

    ID3D12Heap* GpuResourceAllocator::CreateNewHeap(std::uint64_t heapSize)
    {
        D3D12_HEAP_PROPERTIES heapProperties{};
        heapProperties.Type = m_HeapDesc.m_HeapType;
        heapProperties.CreationNodeMask = 1;
        heapProperties.VisibleNodeMask = 1;
        
        D3D12_HEAP_DESC heapDesc{};
        heapDesc.Flags = m_HeapDesc.m_HeapFlags;
        heapDesc.Properties = heapProperties;
        heapDesc.SizeInBytes = heapSize == 0 ? m_HeapSize : heapSize;

        ID3D12Heap* heap = nullptr;
        ASSERT_SUCCEEDED(g_RenderContext.GetDevice()->CreateHeap(&heapDesc, IID_PPV_ARGS(&heap)));
        heap->SetName(L"PlacedResourceAllocator Heap");
        
        return heap;
    }




}
