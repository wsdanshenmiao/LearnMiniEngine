#include "GpuBuffer.h"
#include "RenderContext.h"

namespace DSM{
    GpuBuffer::GpuBuffer(const GpuBufferDesc& bufferDesc, void* initData)
        :m_BufferDesc(bufferDesc){
        auto& allocator = g_RenderContext.GetBufferAllocator((D3D12_HEAP_TYPE)(bufferDesc.m_Usage));

        auto resource = allocator.Allocate(bufferDesc.m_Size);
        m_Resource = resource.m_Resource->GetResource();
        m_GpuAddress = resource.m_GpuAddress;
        m_MappedAddress = resource.m_MappedAddress;
        m_UsageState = resource.m_Resource->GetUsageState();
        
    }


    void GpuBuffer::Update(void* data, std::uint64_t size, std::uint64_t offset)
    {
        ASSERT(m_BufferDesc.m_Usage == DSMResourceUsage::Upload);

        if (m_MappedAddress == nullptr) {
            Map();
        }
        memcpy(GetMappedAddress<char>() + offset, data, size);
    }
}
