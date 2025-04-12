#include "GpuBuffer.h"

namespace DSM{
    GpuBuffer::GpuBuffer(const GpuBufferDesc& bufferDesc, void* initData)
    {
        
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
