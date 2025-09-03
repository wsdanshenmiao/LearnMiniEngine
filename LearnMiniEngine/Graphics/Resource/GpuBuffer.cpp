#include "GpuBuffer.h"
#include "../CommandList/CommandList.h"

namespace DSM{

    void GpuBuffer::Create(const std::wstring& name, const GpuBufferDesc& bufferDesc, void* initData)
    {
        m_BufferDesc = bufferDesc;
        
        // 从资源池中分配资源
        D3D12_RESOURCE_DESC resourceDesc{};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
        resourceDesc.Width = bufferDesc.m_Size;
        resourceDesc.Height = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.Alignment = 0;
        resourceDesc.Flags = bufferDesc.m_Flags;
        resourceDesc.SampleDesc = {1,0};
        GpuResourceDesc gpuResourceDesc{};
        gpuResourceDesc.m_Desc = resourceDesc;
        gpuResourceDesc.m_State = D3D12_RESOURCE_STATE_COMMON;
        gpuResourceDesc.m_HeapType = bufferDesc.m_HeapType;
        gpuResourceDesc.m_HeapFlags = D3D12_HEAP_FLAG_NONE;

        GpuResource::Create(name, gpuResourceDesc);
        
        if (m_BufferDesc.m_HeapType == D3D12_HEAP_TYPE_UPLOAD ||
            m_BufferDesc.m_HeapType == D3D12_HEAP_TYPE_READBACK) {
            ASSERT_SUCCEEDED(m_Resource->Map(0, nullptr, &m_MappedData));
        }

        if (initData != nullptr) {
            if (m_BufferDesc.m_HeapType == D3D12_HEAP_TYPE_UPLOAD) {
                memcpy(m_MappedData, initData, bufferDesc.m_Size);
            }
            else {
                CommandList::InitBuffer(*this, initData, bufferDesc.m_Size);
            }
        }
    }

    void GpuBuffer::Create(const std::wstring& name, ID3D12Resource* resource, std::uint32_t stride)
    {
        D3D12_HEAP_PROPERTIES heapProp{};
        D3D12_HEAP_FLAGS heapFlags{};
        GpuResource::Create(name, resource);
        m_MappedData = nullptr;
        m_BufferDesc.m_Size = resource->GetDesc().Width;
        resource->GetHeapProperties(&heapProp, &heapFlags);
        m_BufferDesc.m_Flags = resource->GetDesc().Flags;
        m_BufferDesc.m_HeapType = heapProp.Type;
        m_BufferDesc.m_Stride = stride;
    }

    void GpuBuffer::Destroy()
    {
        Unmap();
        GpuResource::Destroy();
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
        if (m_Resource != nullptr && m_MappedData != nullptr) {
            m_Resource->Unmap(0, nullptr);
            m_MappedData = nullptr;
        }
    }

    void GpuBuffer::Update(const void* data, std::uint64_t size, std::uint64_t offset)
    {
        ASSERT(m_BufferDesc.m_HeapType == D3D12_HEAP_TYPE_UPLOAD);
        if (m_MappedData == nullptr) {
            Map();
        }
        memcpy(GetMappedData<char>() + offset, data, size);
    }
}
