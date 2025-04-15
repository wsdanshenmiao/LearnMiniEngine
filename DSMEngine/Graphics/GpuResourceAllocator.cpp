#include "GpuResourceAllocator.h"
#include "RenderContext.h"

namespace DSM {
    GpuResourceLocatioin GpuResourceAllocator::CreateBuffer(std::uint64_t bufferSize, LinearBufferDesc bufferDesc)
    {
        GpuResourceLocatioin ret;
        
        if (auto it = m_CommittedResourceAllocators.find(bufferDesc);
            it != m_CommittedResourceAllocators.end()) {
            ret = it->second.AllocateBuffer(bufferSize);
        }
        else {
            m_CommittedResourceAllocators[bufferDesc].Create(bufferDesc, m_SubResourceSize);
            ret = m_CommittedResourceAllocators[bufferDesc].AllocateBuffer(bufferSize);
        }

        return ret;
    }

    void GpuResourceAllocator::Cleanup(std::uint64_t fenceValue)
    {
        for (auto& [desc, allocator] : m_CommittedResourceAllocators) {
            allocator.Cleanup(fenceValue);
        }
    }


    



    GpuResourceLocatioin PlacedResourceAllocator::Allocate(
        D3D12_RESOURCE_DESC resourceDesc,
        D3D12_RESOURCE_STATES resourceState)
    {
        auto resourceSize = resourceDesc.Width * resourceDesc.Height * resourceDesc.DepthOrArraySize;

        GpuResourceLocatioin ret{};
        if (resourceSize > m_HeapSize) {
            auto heap = CreateNewHeap(resourceSize);
            m_FullHeaps.push_back(heap);

            ret.m_Heap = heap;
            ret.m_Offset = 0;
            ret.m_Size = resourceSize;
        }
        else {
            if (m_CurrOffset + resourceSize > m_HeapSize) {
                ASSERT(m_CurrHeap != nullptr);
                m_FullHeaps.push_back(m_CurrHeap);
                m_CurrHeap = nullptr;
            }

            // 当第一次分配或当前堆满的时候请求新的资源
            if (m_CurrHeap == nullptr) {
                m_CurrHeap = RequestHeap();
                m_CurrOffset = 0;
            }
        
            ret.m_Heap = m_CurrHeap;
            ret.m_Offset = m_CurrOffset;
            ret.m_Size = resourceSize;
        }
        
        return ret;  
    }

    ID3D12Heap* PlacedResourceAllocator::RequestHeap()
    {
        std::lock_guard lock(m_Mutex);

        // 清除已经完成的资源
        while (!m_RetiredHeaps.empty() &&
            g_RenderContext.IsFenceComplete(m_RetiredHeaps.front().first)) {
            m_AvailableHeaps.push(m_RetiredHeaps.front().second);
            m_RetiredHeaps.pop();
        }

        ID3D12Heap* ret = nullptr;
        if (m_AvailableHeaps.empty()) {
            ret = CreateNewHeap();
            m_HeapPool.emplace_back(ret);
        }
        else {
            ret = m_AvailableHeaps.front();
            m_AvailableHeaps.pop();
        }
        return ret;
    }

    ID3D12Heap* PlacedResourceAllocator::CreateNewHeap(std::uint64_t heapSize)
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

    void PlacedResourceAllocator::Cleanup(std::uint64_t fenceValue)
    {
        std::lock_guard lock(m_Mutex);

        while (!m_DeletionHeaps.empty() && g_RenderContext.IsFenceComplete(m_DeletionHeaps.front().first))
        {
            // 超过子堆大小的堆直接释放
            m_DeletionHeaps.front().second->Release();
            m_DeletionHeaps.pop();
        }
        
        for (const auto& heap : m_FullHeaps) {
            if (heap->GetDesc().SizeInBytes <= m_HeapSize) {
                m_RetiredHeaps.push(std::make_pair(fenceValue, heap));
            }
            else {
                m_DeletionHeaps.push(std::make_pair(fenceValue, heap));
            }
        }
    }

    /*
    GpuResourceLocatioin GpuResourceAllocator::Allocate(
        std::uint64_t size,
        std::uint32_t alignment,
        D3D12_RESOURCE_FLAGS flags)
    {
        auto bufferSize = size;
        if (alignment != 0 ) {
            // 对齐为2的指数倍 
            ASSERT(std::has_single_bit(alignment));
            bufferSize = Utility::AlignUp(size, alignment);
            m_CurrOffset = Utility::AlignUp(m_CurrOffset, alignment);
        }

        if (m_InitData.m_Strategy == AllocationStrategy::PlacedResource) {
            return AllocateFormHeap(bufferSize);
        }
        else {
            return AllocateFormResource(bufferSize, flags);
        }

        m_CurrOffset += bufferSize;
    }


    void GpuResourceAllocator::Cleanup(std::uint64_t fenceValue)
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        if (m_InitData.m_Strategy == AllocationStrategy::PlacedResource) {
            m_HeapData.m_FullHeaps.push_back(m_HeapData.m_CurrHeap);
            for (const auto& heap : m_HeapData.m_FullHeaps) {
                m_HeapData.m_RetiredHeaps.push(std::make_pair(fenceValue, heap));
            }
            m_HeapData.m_FullHeaps.clear();
        }
        else {
            m_ResourceData.m_FullResources.push_back(m_ResourceData.m_CurrResource);
            for (const auto& resource : m_ResourceData.m_FullResources) {
                m_ResourceData.m_RetiredResources.push(std::make_pair(fenceValue, resource));
            }
            m_ResourceData.m_FullResources.clear();
        }
    }


    void GpuResourceAllocator::ShutDown() noexcept
    {
        if (m_InitData.m_Strategy == AllocationStrategy::PlacedResource) {
            m_HeapData.m_CurrHeap = nullptr;
            m_HeapData.m_FullHeaps.clear();
            while (!m_HeapData.m_RetiredHeaps.empty()) {
                m_HeapData.m_RetiredHeaps.pop();
            }
            while (!m_HeapData.m_AvailableHeaps.empty()) {
                m_HeapData.m_AvailableHeaps.pop();
            }
            m_HeapData.m_HeapPool.clear();
        }
        else {
            m_ResourceData.m_CurrResource = nullptr;
            m_ResourceData.m_FullResources.clear();
            while (!m_ResourceData.m_RetiredResources.empty()) {
                m_ResourceData.m_RetiredResources.pop();
            }
            m_ResourceData.m_AvailableResources.clear();
            m_ResourceData.m_ResourcePool.clear();
        }
    }

    GpuResourceLocatioin GpuResourceAllocator::AllocateFormHeap(std::uint64_t bufferSize)
    {
        auto& currHeap = m_HeapData.m_CurrHeap;
        if (m_CurrOffset + bufferSize > m_SubResourceSize) {
            ASSERT(currHeap != nullptr);
            m_HeapData.m_FullHeaps.push_back(currHeap);
            currHeap = nullptr;
        }

        // 当第一次分配或当前堆满的时候请求新的资源
        if (currHeap == nullptr) {
            currHeap = RequestHeap();
            m_CurrOffset = 0;
        }
        
        GpuResourceLocatioin ret{};
        ret.m_Heap = currHeap;
        ret.m_Offset = m_CurrOffset;
        ret.m_Size = bufferSize;
        return ret;
    }

    GpuResourceLocatioin GpuResourceAllocator::AllocateFormResource(
        std::uint64_t bufferSize,
        D3D12_RESOURCE_FLAGS flags)
    {
        auto& currResource = m_ResourceData.m_CurrResource;
        if (m_CurrOffset + bufferSize > m_SubResourceSize) {
            ASSERT(currResource != nullptr);
            m_ResourceData.m_FullResources.push_back(currResource);
            currResource = nullptr;
        }

        // 当第一次分配或当前堆满的时候请求新的资源
        if (currResource == nullptr) {
            currResource = RequestResource(flags);
            m_CurrOffset = 0;
        }
        
        GpuResourceLocatioin ret{};
        ret.m_Resource = currResource;
        ret.m_GpuAddress = currResource->GetGpuAddress();
        ret.m_MappedAddress = currResource->GetMappedAddress();
        ret.m_Offset = m_CurrOffset;
        ret.m_Size = bufferSize;
        return ret;
    }

    ID3D12Heap* GpuResourceAllocator::RequestHeap()
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        auto& retiredHeap = m_HeapData.m_RetiredHeaps;
        auto& availableHeap = m_HeapData.m_AvailableHeaps;
        // 清除已经完成的资源
        while (!retiredHeap.empty() &&
            g_RenderContext.IsFenceComplete(retiredHeap.front().first)) {
            availableHeap.push(retiredHeap.front().second);
            retiredHeap.pop();
        }

        ID3D12Heap* ret = nullptr;
        if (availableHeap.empty()) {
            ret = CreateNewHeap(m_SubResourceSize);
            m_HeapData.m_HeapPool.emplace_back(ret);
        }
        else {
            ret = availableHeap.front();
            availableHeap.pop();
        }
        return ret;
    }

    ID3D12Heap* GpuResourceAllocator::CreateNewHeap(std::uint64_t size)
    {
        D3D12_HEAP_PROPERTIES heapProperties{};
        heapProperties.Type = m_InitData.m_HeapType;
        heapProperties.CreationNodeMask = 1;
        heapProperties.VisibleNodeMask = 1;
        
        D3D12_HEAP_DESC heapDesc{};
        heapDesc.Flags = m_InitData.m_HeapFlags;
        heapDesc.Properties = heapProperties;
        heapDesc.SizeInBytes = size;

        ID3D12Heap* heap = nullptr;
        ASSERT_SUCCEEDED(g_RenderContext.GetDevice()->CreateHeap(&heapDesc, IID_PPV_ARGS(&heap)));
        heap->SetName(L"GpuResourceAllocator Heap");
        
        return heap;
    }
    */
}
