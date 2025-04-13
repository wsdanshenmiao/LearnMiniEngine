#pragma once
#ifndef __GPURESOURCE_H__
#define __GPURESOURCE_H__

#include "../pch.h"

namespace DSM {
    // GPU 资源的封装
    class GpuResource
    {
    public:
        GpuResource() noexcept
            :m_Resource(nullptr),
            m_GpuAddress(D3D12_GPU_VIRTUAL_ADDRESS_NULL),
            m_UsageState(D3D12_RESOURCE_STATE_COMMON){}
        GpuResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES currState)
            :m_Resource(resource),
            m_UsageState(currState)
        {
            if (resource != nullptr) {
                m_GpuAddress = resource->GetGPUVirtualAddress();
            }
        }
        ~GpuResource() noexcept { Destroy(); }
        
        DSM_NONCOPYABLE(GpuResource);

        virtual void Destroy() noexcept
        {
            Unmap();
            m_Resource = nullptr;
            m_GpuAddress = D3D12_GPU_VIRTUAL_ADDRESS_NULL;
            ++m_VersitonID;
        }

        ID3D12Resource* operator->() { return m_Resource.Get(); }
        const ID3D12Resource* operator->() const { return m_Resource.Get(); }
        
        ID3D12Resource* GetResource(){ return m_Resource.Get(); }
        const ID3D12Resource* GetResource() const { return m_Resource.Get(); }

        D3D12_RESOURCE_STATES GetUsageState() const noexcept { return m_UsageState; }

        D3D12_GPU_VIRTUAL_ADDRESS GetGpuAddress() const noexcept { return m_GpuAddress; }
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

        std::uint32_t GetVertexID() const noexcept { return m_VersitonID; }


    protected:
        Microsoft::WRL::ComPtr<ID3D12Resource> m_Resource;
        D3D12_GPU_VIRTUAL_ADDRESS m_GpuAddress;
        void* m_MappedAddress{};
        D3D12_RESOURCE_STATES m_UsageState;

        // 资源的销毁次数
        std::uint32_t m_VersitonID = 0;
    };


    
}

#endif
