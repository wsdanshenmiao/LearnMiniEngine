#include "GpuResource.h"
#include "GpuResourceAllocator.h"

namespace DSM {

    static std::map<DSMHeapDesc, GpuResourceAllocator> s_GpuResourceAllocators{};


    GpuResource::GpuResource(const GpuResourceDesc& resourceDesc)
    {
        DSMHeapDesc heapDesc{};
        heapDesc.m_HeapType = resourceDesc.m_HeapType;
        heapDesc.m_HeapFlags = resourceDesc.m_HeapFlags;
        ID3D12Resource* resource = nullptr;
        if (auto it = s_GpuResourceAllocators.find(heapDesc); it != s_GpuResourceAllocators.end()) {
            resource =it->second.CreateResource(resourceDesc.m_Desc, resourceDesc.m_State);
            m_Allocator = &it->second;
        }
        else {
            auto& allocator = s_GpuResourceAllocators[heapDesc];
            allocator.Create(heapDesc);
            resource = allocator.CreateResource(resourceDesc.m_Desc, resourceDesc.m_State);
            m_Allocator = &allocator;
        }
        m_Resource = resource;
        m_UsageState = resourceDesc.m_State;
        m_GpuAddress = m_Resource->GetGPUVirtualAddress();
        
        if (resourceDesc.m_HeapType == D3D12_HEAP_TYPE_UPLOAD) {
            ASSERT_SUCCEEDED(m_Resource->Map(0, nullptr, &m_MappedAddress));
        }
    }

    void GpuResource::Destroy() noexcept
    {
        m_Allocator->ReleaseResource(m_Resource.Get());
        Unmap();
        m_Resource = nullptr;
        m_GpuAddress = D3D12_GPU_VIRTUAL_ADDRESS_NULL;
    }
}
