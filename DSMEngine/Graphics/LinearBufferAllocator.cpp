#include "LinearBufferAllocator.h"
#include "RenderContext.h"

namespace DSM {
    

    void LinearBufferAllocator::ShutDown()
    {
        m_CurrPage = nullptr;
        m_FullPages.clear();
        while (!m_RetiredPages.empty()) {
            m_RetiredPages.pop();
        }
        while (!m_AvailablePages.empty()) {
            m_AvailablePages.pop();
        }
        m_PagePool.clear();
    }

    GpuResourceLocatioin LinearBufferAllocator::CreateBuffer(std::uint64_t bufferSize, std::uint32_t alignment)
    {
        GpuResourceLocatioin ret{};

        std::lock_guard lock(m_Mutex);
        // TODO: 后续考虑分配过大资源的情况
        if (!m_CurrPage->Allocate(bufferSize, alignment, ret)) {    // 创建新的Page
            // 记录已经满的Page
            m_FullPages[m_CurrPage->m_Resource.get()] = m_CurrPage;
            m_CurrPage = RequestPage();
            m_CurrPage->Allocate(bufferSize, alignment, ret);
        }
        
        return ret;
    }

    LinearBufferPage* LinearBufferAllocator::RequestPage()
    {
        // 清除已经完成的资源
        while (!m_RetiredPages.empty() &&
            g_RenderContext.IsFenceComplete(m_RetiredPages.front().first)) {
            m_AvailablePages.push(m_RetiredPages.front().second);
            m_RetiredPages.pop();
        }

        LinearBufferPage* ret = nullptr;
        if (m_AvailablePages.empty()) {
            auto buffer = CreateNewBuffer();
            auto newPage = std::make_unique<LinearBufferPage>(m_PageSize, buffer);
            ret = newPage.get();
            m_PagePool.emplace_back(std::move(newPage));
        }
        else {
            ret = m_AvailablePages.front();
            m_AvailablePages.pop();
        }
        
        return ret;
    }

    GpuResource* LinearBufferAllocator::CreateNewBuffer(std::uint64_t bufferSize)
    {
        D3D12_HEAP_PROPERTIES heapProperties{};
        heapProperties.Type = m_BufferDesc.m_HeapType;
        heapProperties.CreationNodeMask = 1;
        heapProperties.VisibleNodeMask = 1;

        D3D12_RESOURCE_DESC resourceDesc{};
        resourceDesc.Alignment = 0;
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
        resourceDesc.Height = 1;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        resourceDesc.MipLevels = 1;
        resourceDesc.SampleDesc = {1, 0};
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.Width = bufferSize == 0 ? m_PageSize : bufferSize;
        resourceDesc.Flags = m_BufferDesc.m_Flags;

        D3D12_RESOURCE_STATES resourceState = D3D12_RESOURCE_STATE_COMMON;
        if (m_BufferDesc.m_HeapType == D3D12_HEAP_TYPE_UPLOAD) {
            resourceState = D3D12_RESOURCE_STATE_GENERIC_READ;
        }
        else if (m_BufferDesc.m_HeapType == D3D12_HEAP_TYPE_DEFAULT) {
            resourceState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        }
        else if (m_BufferDesc.m_HeapType == D3D12_HEAP_TYPE_READBACK) {
            resourceState = D3D12_RESOURCE_STATE_COPY_DEST;
        }
        
        ID3D12Resource* resource = nullptr;
        ASSERT_SUCCEEDED(g_RenderContext.GetDevice()->CreateCommittedResource(
            &heapProperties,
            m_BufferDesc.m_HeapFlags,
            &resourceDesc,
            resourceState,
            nullptr,
            IID_PPV_ARGS(&resource)));
        resource->SetName(L"CommittedResourceManager SubResource");
        
        return new GpuResource(resource, resourceState);
    }

    void LinearBufferAllocator::ReleaseBuffer(GpuResource* buffer, std::uint64_t fenceValue)
    {
        if (!m_CurrPage->ReleaseResource(buffer)) {
            std::lock_guard lock(m_Mutex);
            
            auto pageIt = m_FullPages.find(buffer);
            ASSERT(pageIt != m_FullPages.end(), "Resource is not create by this allocator");
            pageIt->second->ReleaseResource(buffer);

            // 子资源都清空之后回收该资源
            if (pageIt->second->GetSubResourceCount() == 0) {
                m_FullPages.erase(buffer);
                m_RetiredPages.push(std::make_pair(fenceValue, pageIt->second));
            }
        }
    }

    void LinearBufferAllocator::Cleanup(std::uint64_t fenceValue)
    {
        std::lock_guard lock(m_Mutex);

        while (!m_DeletionResources.empty() && g_RenderContext.IsFenceComplete(m_DeletionResources.front().first))
        {
            // 超过子资源大小的资源直接删除
            delete m_DeletionResources.front().second;
            m_DeletionResources.pop();
        }
    }


}
