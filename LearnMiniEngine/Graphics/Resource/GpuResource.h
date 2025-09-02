#pragma once
#ifndef __GPURESOURCE_H__
#define __GPURESOURCE_H__

#include "../../pch.h"

namespace DSM {
    class GpuResourceAllocator;
    
    struct GpuResourceDesc
    {
        D3D12_HEAP_TYPE m_HeapType = D3D12_HEAP_TYPE_DEFAULT;
        D3D12_HEAP_FLAGS m_HeapFlags = D3D12_HEAP_FLAG_NONE;
        D3D12_RESOURCE_DESC m_Desc{};
        D3D12_RESOURCE_STATES m_State = D3D12_RESOURCE_STATE_COMMON;
    };
    
    // GPU 资源的封装
    class GpuResource
    {
    public:
        GpuResource() = default;
        GpuResource(const std::wstring& name, const GpuResourceDesc& resourceDesc)
        {
            Create(name, resourceDesc);
        }
        GpuResource(const std::wstring& name, ID3D12Resource* resource)
        {
            Create(name, resource);
        }
        virtual ~GpuResource() { Destroy(); }
        GpuResource(GpuResource&& resource) noexcept = default;
        GpuResource& operator=(GpuResource&& resource) noexcept = default;
        DSM_NONCOPYABLE(GpuResource);

        void Create(const std::wstring& name, const GpuResourceDesc& resourceDesc, const D3D12_CLEAR_VALUE* clearValue = nullptr);
        void Create(const std::wstring& name, ID3D12Resource* resource);
        virtual void Destroy();

        ID3D12Resource* operator->() { return m_Resource.Get(); }
        const ID3D12Resource* operator->() const { return m_Resource.Get(); }
        
        ID3D12Resource* GetResource(){ return m_Resource.Get(); }
        const ID3D12Resource* GetResource() const { return m_Resource.Get(); }
        ID3D12Resource** GetAddressOf() { return m_Resource.GetAddressOf(); }
        ID3D12Resource* const * GetAddressOf() const { return m_Resource.GetAddressOf(); }

        D3D12_RESOURCE_STATES GetUsageState() const noexcept { return m_UsageState; }

        D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress() const noexcept { return m_Resource->GetGPUVirtualAddress(); }

        void SetUsageState(D3D12_RESOURCE_STATES usageState) noexcept { m_UsageState = usageState; }
        
    protected:
        Microsoft::WRL::ComPtr<ID3D12Resource> m_Resource{};
        // 资源当前的状态
        D3D12_RESOURCE_STATES m_UsageState{};

        // 该资源的创建者
        GpuResourceAllocator* m_Allocator{};
    };


    
}

#endif
