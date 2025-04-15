#pragma once
#ifndef __GPURESOURCE_H__
#define __GPURESOURCE_H__

#include "../pch.h"

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
        GpuResource(const GpuResourceDesc& resourceDesc);
        ~GpuResource() noexcept { Destroy(); }
        GpuResource(GpuResource&& resource) noexcept = default;
        GpuResource& operator=(GpuResource&& resource) noexcept = default;
        DSM_NONCOPYABLE(GpuResource);

        virtual void Destroy() noexcept;

        ID3D12Resource* operator->() { return m_Resource.Get(); }
        const ID3D12Resource* operator->() const { return m_Resource.Get(); }
        
        ID3D12Resource* GetResource(){ return m_Resource.Get(); }
        const ID3D12Resource* GetResource() const { return m_Resource.Get(); }
        ID3D12Resource** GetAddressOf() { return m_Resource.GetAddressOf(); }
         ID3D12Resource* const * GetAddressOf() const { return m_Resource.GetAddressOf(); }

        D3D12_RESOURCE_STATES GetUsageState() const noexcept { return m_UsageState; }

        D3D12_GPU_VIRTUAL_ADDRESS GetGpuAddress() noexcept
        {
            if (m_GpuAddress == D3D12_GPU_VIRTUAL_ADDRESS_NULL && m_Resource != nullptr) {
                m_GpuAddress = m_Resource->GetGPUVirtualAddress();
            }
            return m_GpuAddress;
        }
        void* Map()
        {
            if (m_MappedAddress == nullptr) {
                ASSERT_SUCCEEDED(m_Resource->Map(0, nullptr, &m_MappedAddress));
            }
        
            return m_MappedAddress;
        }

        void Unmap()
        {
            if (m_MappedAddress != nullptr) {
                m_Resource->Unmap(0, nullptr);
                m_MappedAddress = nullptr;
            }
        }
        template<typename T = void>
        T* GetMappedAddress() const
        {
            return static_cast<T*>(m_MappedAddress);
        }


    protected:
        Microsoft::WRL::ComPtr<ID3D12Resource> m_Resource;
        D3D12_GPU_VIRTUAL_ADDRESS m_GpuAddress;
        void* m_MappedAddress{};
        D3D12_RESOURCE_STATES m_UsageState;

        GpuResourceAllocator* m_Allocator;
    };


    // 用于定位子资源在缓冲区中的位置
    struct GpuResourceLocatioin
    {
        GpuResource* m_Resource{};
        ID3D12Heap* m_Heap{};
        D3D12_GPU_VIRTUAL_ADDRESS m_GpuAddress{};
        void* m_MappedAddress{};
        std::uint64_t m_Offset{};
        std::uint64_t m_Size{};
    };
    
}

#endif
