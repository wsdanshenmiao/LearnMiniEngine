#pragma once
#ifndef __LINEARBUFFERALLOCATOR_H__
#define __LINEARBUFFERALLOCATOR_H__

#include "GpuBuffer.h"
#include "../../Utilities/LinearAllocator.h"

namespace DSM {
    // 用于定位子资源在缓冲区中的位置
    struct GpuResourceLocation
    {
        GpuResource* m_Resource{};
        D3D12_GPU_VIRTUAL_ADDRESS m_GpuAddress{};
        void* m_MappedAddress{};
        std::uint64_t m_Offset{};
        std::uint64_t m_Size{};
    };
    
    class DynamicBufferPage
    {
        friend class DynamicBufferAllocator;
    public:
        DynamicBufferPage(GpuResource* agentResource, bool mappedAble) 
            :m_Resource(agentResource), m_LiearAllocator(agentResource->GetResource()->GetDesc().Width)
        {
            if (mappedAble) {
                ASSERT_SUCCEEDED(m_Resource->GetResource()->Map(0, nullptr, reinterpret_cast<void**>(&m_MappedAddress)));
            }
        }
        ~DynamicBufferPage() = default;

        bool Allocate(std::uint64_t size, std::uint32_t alignment, GpuResourceLocation& outResource)
        {
            auto offset = m_LiearAllocator.Allocate(size, alignment);
            if (offset == Utility::INVALID_ALLOC_OFFSET) {
                return false;
            }
            else {
                outResource.m_Resource = m_Resource.get();
                outResource.m_Offset = offset;
                outResource.m_Size = size;
                outResource.m_GpuAddress = m_Resource->GetGpuVirtualAddress() + offset;
                if (m_MappedAddress != nullptr) {
                    outResource.m_MappedAddress = m_MappedAddress + offset;
                }
                return true;
            }
        }
        
        void Reset() noexcept
        {
            m_LiearAllocator.Clear();
        }

    private:
        std::unique_ptr<GpuResource> m_Resource{};
        LinearAllocator m_LiearAllocator;
        std::uint8_t* m_MappedAddress{};
    };
    
    class DynamicBufferAllocator
    {
    public:
        enum class AllocateMode
        {
            CpuExclusive, GpuExclusive
        };
        
        DynamicBufferAllocator() = default;
        ~DynamicBufferAllocator() { Shutdown(); };
        DSM_NONCOPYABLE_NONMOVABLE(DynamicBufferAllocator);

        void Create(AllocateMode mode, std::uint64_t pageSize = DEFAULT_BUFFER_PAGE_SIZE);
        void Shutdown();

        GpuResourceLocation Allocate(std::uint64_t bufferSize, std::uint32_t alignment = 0);
        // 清理所有的缓冲区
        void Cleanup(std::uint64_t fenceValue);

    private:
        DynamicBufferPage* RequestPage();
        GpuResource* CreateNewBuffer(std::uint64_t bufferSize = 0);

    private:
        AllocateMode m_AllocateMode = AllocateMode::CpuExclusive;
        
        std::vector<std::unique_ptr<DynamicBufferPage>> m_PagePool;

        std::vector<DynamicBufferPage*> m_FullPages;
        std::vector<GpuResource*> m_LargePages;

        DynamicBufferPage* m_CurrPage{};
        // 等待使用完毕的资源
        std::queue<std::pair<std::uint64_t, DynamicBufferPage*>> m_RetiredPages{};
        // 可重复使用的资源
        std::queue<DynamicBufferPage*> m_AvailablePages{};
        // 需要删除的资源
        std::queue<std::pair<std::uint64_t, GpuResource*>> m_DeletionPages{};

        std::uint64_t m_PageSize{};
        
        std::mutex m_Mutex{};
        
    };

}

#endif

