#include "GpuResource.h"
#include "GpuResourceAllocator.h"

namespace DSM {

    static std::map<DSMHeapDesc, GpuResourceAllocator> s_GpuResourceAllocators{};

    

    void GpuResource::Create(const std::wstring& name, const GpuResourceDesc& resourceDesc, const D3D12_CLEAR_VALUE* clearValue)
    {
        if (m_Resource != nullptr) {
            Destroy();
        }
        
        DSMHeapDesc heapDesc{};
        heapDesc.m_HeapType = resourceDesc.m_HeapType;
        heapDesc.m_HeapFlags = resourceDesc.m_HeapFlags;
        ID3D12Resource* resource = nullptr;
        if (auto it = s_GpuResourceAllocators.find(heapDesc); it != s_GpuResourceAllocators.end()) {
           resource =it->second.CreateResource(resourceDesc.m_Desc, resourceDesc.m_State, clearValue);
           m_Allocator = &it->second;
        }
        else {
           auto& allocator = s_GpuResourceAllocators[heapDesc];
           allocator.Create(heapDesc);
           resource = allocator.CreateResource(resourceDesc.m_Desc, resourceDesc.m_State, clearValue);
           m_Allocator = &allocator;
        }
        m_Resource = resource;
        m_UsageState = resourceDesc.m_State;

        m_Resource->SetName(name.c_str());
    }

    void GpuResource::Create(const std::wstring& name, ID3D12Resource* resource)
    {
        ASSERT(resource != nullptr);
        if (m_Resource != nullptr) {
            Destroy();
        }
        
        m_Resource.Attach(resource);
        m_Resource->SetName(name.c_str());
        m_UsageState = D3D12_RESOURCE_STATE_COMMON;
        m_Allocator = nullptr;
    }

    void GpuResource::Destroy()
    {
        if (m_Resource != nullptr && m_Allocator != nullptr) {
            m_Allocator->ReleaseResource(m_Resource.Get());
        }
        m_Resource = nullptr;
        m_Allocator = nullptr;
    }
}
