#pragma once
#ifndef __LINEARBUFFERALLOCATOR_H__
#define __LINEARBUFFERALLOCATOR_H__

#include "GpuResource.h"
#include "../Utilities/LinearAllocator.h"

namespace DSM {
    
    struct LinearBufferDesc
    {
        D3D12_HEAP_TYPE m_HeapType{};
        D3D12_HEAP_FLAGS m_HeapFlags = D3D12_HEAP_FLAG_NONE;
        D3D12_RESOURCE_FLAGS m_Flags = D3D12_RESOURCE_FLAG_NONE;
        std::strong_ordering operator<=>(const LinearBufferDesc& rhs) const = default;
    };
    
    class LinearBufferPage
    {
        friend class LinearBufferAllocator;
    public:
        LinearBufferPage(std::uint64_t bufferSize, GpuResource* agentResource) noexcept
            :m_LiearAllocator(bufferSize), m_Resource(agentResource){}
        ~LinearBufferPage() = default;

        bool Allocate(std::uint64_t size, std::uint32_t alignment, GpuResourceLocatioin& outResource)
        {
            auto offset = m_LiearAllocator.Allocate(size, alignment);
            if (offset == Utility::INVALID_ALLOC_OFFSET) {
                return false;
            }
            else {
                outResource.m_Resource = m_Resource.get();
                outResource.m_Offset = offset;
                outResource.m_Size = size;
                outResource.m_GpuAddress = m_Resource->GetGpuAddress();
                outResource.m_MappedAddress = m_Resource->GetMappedAddress();
                ++m_SubResourceCount;
                return true;
            }
        }

        bool ReleaseResource(GpuResource* resource) noexcept
        {
            if (resource == m_Resource.get()) {
                --m_SubResourceCount;
                return true;
            }
            return false;
        }
        
        void Reset() noexcept
        {
            m_LiearAllocator.Clear();
            m_SubResourceCount = 0;
        }

        std::uint32_t GetSubResourceCount() const noexcept{ return m_SubResourceCount; }

    private:
        std::unique_ptr<GpuResource> m_Resource{};
        LinearAllocator m_LiearAllocator;
        std::uint32_t m_SubResourceCount{};
    };
    
    class LinearBufferAllocator
    {
    public:
        LinearBufferAllocator() = default;
        ~LinearBufferAllocator() { ShutDown(); };
        DSM_NONCOPYABLE(LinearBufferAllocator);

        void Create(LinearBufferDesc bufferDesc, std::uint64_t pageSize = DEFAULT_BUFFER_PAGE_SIZE) noexcept
        {
            m_PageSize = pageSize;
            m_BufferDesc = bufferDesc;
            
            auto newPage = std::make_unique<LinearBufferPage>(m_PageSize, CreateNewBuffer());
            m_CurrPage = newPage.get();
            m_PagePool.emplace_back(std::move(newPage));
        }
        void ShutDown();

        GpuResourceLocatioin CreateBuffer(std::uint64_t bufferSize, std::uint32_t alignment = 0);
        
        GpuResource* CreateNewBuffer(std::uint64_t bufferSize = 0);

        // 释放某个缓冲区
        void ReleaseBuffer(GpuResource* buffer, std::uint64_t fenceValue);

        // 清除已经使用完毕的资源
        void Cleanup(std::uint64_t fenceValue);

    private:
        LinearBufferPage* RequestPage();

    private:
        LinearBufferDesc m_BufferDesc{};

        std::vector<std::unique_ptr<LinearBufferPage>> m_PagePool;
        // 用于管理每个子资源与父资源的对应关系
        std::map<GpuResource*, LinearBufferPage*> m_FullPages{};

        LinearBufferPage* m_CurrPage{};
        // 等待使用完毕的资源
        std::queue<std::pair<std::uint64_t, LinearBufferPage*>> m_RetiredPages{};
        // 可重复使用的资源
        std::queue<LinearBufferPage*> m_AvailablePages{};

        std::uint64_t m_PageSize{};
        
        std::mutex m_Mutex{};



        
        
        
        // 需要删除的资源
        std::queue<std::pair<std::uint64_t, GpuResource*>> m_DeletionResources{};
        
        
    };

}

#endif

