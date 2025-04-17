#include "GpuBuffer.h"
#include "RenderContext.h"

namespace DSM{
    GpuBuffer::GpuBuffer(const GpuBufferDesc& bufferDesc, void* initData)
        :m_BufferDesc(bufferDesc){
        // 从资源池中分配资源
        auto alignment = 0;
        if (Utility::HasAllFlags(bufferDesc.m_BufferFlag, DSMBufferFlag::ConstantBuffer)) {
            alignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
        }
        auto bufferSize = Utility::AlignUp(bufferDesc.m_Size, alignment);
        
        auto resourceFlags = D3D12_RESOURCE_FLAG_NONE;
        auto heapType = D3D12_HEAP_TYPE_DEFAULT;
        auto resourceState = D3D12_RESOURCE_STATE_COMMON;
        if (Utility::HasAllFlags(bufferDesc.m_BufferFlag, DSMBufferFlag::AccelStruct)) {
            resourceState = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
        }
        if (bufferDesc.m_Usage == DSMResourceUsage::Upload) {
            resourceState = D3D12_RESOURCE_STATE_GENERIC_READ;
            heapType = D3D12_HEAP_TYPE_UPLOAD;
        }
        else if (bufferDesc.m_Usage == DSMResourceUsage::Readback) {
            resourceFlags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
            resourceState = D3D12_RESOURCE_STATE_COPY_DEST;
            heapType = D3D12_HEAP_TYPE_READBACK;
        }
        
        if (Utility::HasAllFlags(bufferDesc.m_BindFlag, DSMBindFlag::UnorderedAccess)) {
            resourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        }
        if (!Utility::HasAllFlags(bufferDesc.m_BindFlag, DSMBindFlag::ShaderResource)) {
            resourceFlags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
        }
        if (Utility::HasAllFlags(bufferDesc.m_BufferFlag, DSMBufferFlag::AccelStruct)) {
            resourceFlags |= D3D12_RESOURCE_FLAG_RAYTRACING_ACCELERATION_STRUCTURE;
        }

        D3D12_RESOURCE_DESC resourceDesc{};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
        resourceDesc.Width = bufferSize;
        resourceDesc.Height = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.Alignment = 0;
        resourceDesc.Flags = resourceFlags;
        resourceDesc.SampleDesc = {1,0};
        GpuResourceDesc gpuResourceDesc{};
        gpuResourceDesc.m_Desc = resourceDesc;
        gpuResourceDesc.m_State = resourceState;
        gpuResourceDesc.m_HeapType = heapType;
        gpuResourceDesc.m_HeapFlags = D3D12_HEAP_FLAG_NONE;

        Create(gpuResourceDesc);
        
        if (m_BufferDesc.m_Usage == DSMResourceUsage::Upload ||
            m_BufferDesc.m_Usage == DSMResourceUsage::Readback) {
            ASSERT_SUCCEEDED(m_Resource->Map(0, nullptr, &m_MappedData));
        }

        // TODO: 封装完命令列表后转变资源状态，并将初始话资源拷贝到Buffer中
    }

    void GpuBuffer::Destroy()
    {
        GpuResource::Destroy();
        Unmap();
    }

    void* GpuBuffer::Map()
    {
        if (m_MappedData == nullptr) {
            ASSERT_SUCCEEDED(m_Resource->Map(0, nullptr, &m_MappedData));
        }
        return m_MappedData;
    }

    void GpuBuffer::Unmap()
    {
        if (m_MappedData != nullptr) {
            m_Resource->Unmap(0, nullptr);
            m_MappedData = nullptr;
        }
    }

    void GpuBuffer::Update(void* data, std::uint64_t size, std::uint64_t offset)
    {
        ASSERT(m_BufferDesc.m_Usage == DSMResourceUsage::Upload);
        if (m_MappedData == nullptr) {
            Map();
        }
        memcpy(GetMappedData<char>() + offset, data, size);
    }
}
