#include "GpuBuffer.h"
#include "RenderContext.h"

namespace DSM{
    GpuBuffer::GpuBuffer(const GpuBufferDesc& bufferDesc, void* initData)
        :m_BufferDesc(bufferDesc){
        // 从资源池中分配资源
        auto& allocator = g_RenderContext.GetBufferAllocator((D3D12_HEAP_TYPE)(bufferDesc.m_Usage));
        auto alignment = 0;
        if (Utility::HasAllFlags(bufferDesc.m_BufferFlag, DSMBufferFlag::ConstantBuffer)) {
            alignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
        }
        
        auto flags = D3D12_RESOURCE_FLAG_NONE;
        auto transitioningState = D3D12_RESOURCE_STATE_COMMON;
        if (Utility::HasAllFlags(bufferDesc.m_BufferFlag, DSMBufferFlag::AccelStruct)) {
            transitioningState = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
        }
        if (bufferDesc.m_Usage == DSMResourceUsage::Upload) {
            transitioningState = D3D12_RESOURCE_STATE_GENERIC_READ;
        }
        else if (bufferDesc.m_Usage == DSMResourceUsage::Readback) {
            flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
            transitioningState = D3D12_RESOURCE_STATE_COPY_DEST;
        }
        
        if (Utility::HasAllFlags(bufferDesc.m_BindFlag, DSMBindFlag::UnorderedAccess)) {
            flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        }
        if (!Utility::HasAllFlags(bufferDesc.m_BindFlag, DSMBindFlag::ShaderResource)) {
            flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
        }
        
        auto resource = allocator.Allocate(bufferDesc.m_Size, alignment);
        m_Resource = resource.m_Resource->GetResource();
        m_UsageState = resource.m_Resource->GetUsageState();
        m_GpuAddress = resource.m_GpuAddress;
        m_MappedAddress = resource.m_MappedAddress;

        // TODO: 封装完命令列表后转变资源状态，并将初始话资源拷贝到Buffer中
    }


    void GpuBuffer::Update(void* data, std::uint64_t size, std::uint64_t offset)
    {
        // 之后上传堆才可写出数据
        ASSERT(m_BufferDesc.m_Usage == DSMResourceUsage::Upload);

        if (m_MappedAddress == nullptr) {
            Map();
        }
        memcpy(GetMappedAddress<char>() + offset, data, size);
    }
}
