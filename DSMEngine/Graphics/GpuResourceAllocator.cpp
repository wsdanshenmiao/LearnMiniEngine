#include "GpuResourceAllocator.h"
#include "RenderContext.h"

namespace DSM {
    GpuResourceLocatioin GpuResourceAllocator::Allocate(std::uint64_t size, std::uint32_t alignment)
    {
        auto bufferSize = size;
        // 对齐为2的指数倍 
        if (alignment != 0 ) {
            ASSERT(alignment & (alignment - 1) == 0);

            bufferSize = Utility::AlignUp(size, alignment);
        }
        m_CurrOffset = Utility::AlignUp(bufferSize, alignment);

        if (m_InitData.m_Strategy == AllocationStrategy::PlacedResource) {
            return AllocateFormHeap(bufferSize);
        }
        else {
            return AllocateFormResource(bufferSize);
        }
    }

    void GpuResourceAllocator::Cleanup(std::uint64_t fenceValue)
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        auto discardFunc = [fenceValue](auto& fullResource, auto& retiredResource) {
            for (const auto& resource : fullResource) {
                retiredResource.push(std::make_pair(fenceValue, resource));
            }
            fullResource.clear();
        };
        if (m_InitData.m_Strategy == AllocationStrategy::PlacedResource) {
            m_HeapData.m_FullHeaps.push_back(m_HeapData.m_CurrHeap);
            discardFunc(m_HeapData.m_FullHeaps, m_HeapData.m_RetiredHeaps);
        }
        else {
            m_ResourceData.m_FullResources.push_back(m_ResourceData.m_CurrResource);
            discardFunc(m_ResourceData.m_FullResources, m_ResourceData.m_RetiredResources);
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
            while (!m_ResourceData.m_AvailableResources.empty()) {
                m_ResourceData.m_AvailableResources.pop();
            }
            m_ResourceData.m_ResourcePool.clear();
        }
    }

    GpuResourceLocatioin GpuResourceAllocator::AllocateFormHeap(std::uint64_t alignSize)
    {
        auto& currHeap = m_HeapData.m_CurrHeap;
        if (m_CurrOffset + alignSize > m_SubResourceSize) {
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
        ret.m_Size = alignSize;
        return ret;
    }

    GpuResourceLocatioin GpuResourceAllocator::AllocateFormResource(std::uint64_t alignSize)
    {
        auto& currResource = m_ResourceData.m_CurrResource;
        if (m_CurrOffset + alignSize > m_SubResourceSize) {
            ASSERT(currResource != nullptr);
            m_ResourceData.m_FullResources.push_back(currResource);
            currResource = nullptr;
        }

        // 当第一次分配或当前堆满的时候请求新的资源
        if (currResource == nullptr) {
            currResource = RequestResource();
            m_CurrOffset = 0;
        }
        
        GpuResourceLocatioin ret{};
        ret.m_Resource = currResource;
        ret.m_GpuAddress = currResource->GetGpuAddress();
        ret.m_MappedAddress = currResource->GetMappedAddress();
        ret.m_Offset = m_CurrOffset;
        ret.m_Size = alignSize;

        m_CurrOffset += alignSize;
        return ret;
    }

    GpuResource* GpuResourceAllocator::RequestResource()
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        auto& retiredResource = m_ResourceData.m_RetiredResources;
        auto& availableResource = m_ResourceData.m_AvailableResources;
        // 清除已经完成的资源
        while (!retiredResource.empty() &&
            g_RenderContext.IsFenceComplete(retiredResource.front().first)) {
            availableResource.push(retiredResource.front().second);
            retiredResource.pop();
        }

        GpuResource* ret = nullptr;
        if (availableResource.empty()) {
            ret = CreateNewResource(m_SubResourceSize);
            if (m_InitData.m_HeapType == D3D12_HEAP_TYPE_UPLOAD) {
                ret->Map();
            }
            m_ResourceData.m_ResourcePool.emplace_back(ret);
        }
        else {
            ret = availableResource.front();
            availableResource.pop();
        }
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

    GpuResource* GpuResourceAllocator::CreateNewResource(std::uint64_t size)
    {
        D3D12_HEAP_PROPERTIES heapProperties{};
        heapProperties.Type = m_InitData.m_HeapType;
        heapProperties.CreationNodeMask = 1;
        heapProperties.VisibleNodeMask = 1;

        D3D12_RESOURCE_DESC resourceDesc{};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Flags = m_InitData.m_ResourceFlags;
        resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
        resourceDesc.Width = size;
        resourceDesc.Height = 1;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        resourceDesc.MipLevels = 1;
        resourceDesc.SampleDesc = { 1, 0};
        resourceDesc.DepthOrArraySize = 1;

        D3D12_RESOURCE_STATES resourceState{};
        if (heapProperties.Type == D3D12_HEAP_TYPE_UPLOAD) {
            resourceState = D3D12_RESOURCE_STATE_GENERIC_READ;
        }
        else if (heapProperties.Type == D3D12_HEAP_TYPE_DEFAULT) {
            resourceState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        }
        
        ID3D12Resource* resource = nullptr;
        ASSERT_SUCCEEDED(g_RenderContext.GetDevice()->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            resourceState,
            nullptr,
            IID_PPV_ARGS(&resource)));
        resource->SetName(L"GpuResourceAllocator SubResource");
        
        return new GpuResource(resource, resourceState);
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
    
}
