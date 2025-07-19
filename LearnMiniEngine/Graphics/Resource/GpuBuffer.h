#pragma once
#ifndef __GPUBUFFER_H__
#define __GPUBUFFER_H__

#include <d3d12.h>
#include "GpuResource.h"
#include "../../Utilities/Macros.h"

namespace DSM {
    // 用于创建 Buffer 的描述
    struct GpuBufferDesc
    {
        std::uint64_t m_Size = 0;
        std::uint32_t m_Stride = 0;
        D3D12_HEAP_TYPE m_HeapType = D3D12_HEAP_TYPE_DEFAULT;
        D3D12_RESOURCE_FLAGS m_Flags = D3D12_RESOURCE_FLAG_NONE;
    };

    
    // GPU 缓冲区
    class GpuBuffer : public GpuResource
    {
    public:
        GpuBuffer() = default;
        GpuBuffer(const std::wstring& name, const GpuBufferDesc& bufferDesc, void* initData = nullptr)
            :m_BufferDesc(bufferDesc)
        {
            Create(name, bufferDesc, initData);
        }
        ~GpuBuffer() = default;
        DSM_NONCOPYABLE(GpuBuffer);

        void Create(const std::wstring& name, const GpuBufferDesc& bufferDesc, void* initData = nullptr);
        void Create(const std::wstring& name, ID3D12Resource* resource, std::uint32_t stride);
        virtual void Destroy() override;

        const GpuBufferDesc& GetDesc() const noexcept { return m_BufferDesc; }
        std::uint64_t GetSize() const noexcept {return m_BufferDesc.m_Size; }
        std::uint32_t GetStride() const noexcept { return m_BufferDesc.m_Stride; }
        std::uint32_t GetCount() const
        {
            ASSERT(m_BufferDesc.m_Stride != 0);
            return static_cast<std::uint32_t>(m_BufferDesc.m_Size / m_BufferDesc.m_Stride);
        }

        bool Mappable() const noexcept
        {
            return m_BufferDesc.m_HeapType == D3D12_HEAP_TYPE_UPLOAD ||
                m_BufferDesc.m_HeapType == D3D12_HEAP_TYPE_READBACK;
        }
        void* Map();
        void Unmap();
        template <typename T = void>
        T* GetMappedData() const { return reinterpret_cast<T*>(m_MappedData); }
        void Update(const void* data, std::uint64_t size, std::uint64_t offset = 0);
        template <typename T>
        void Update(const T& data) { Update(&data, sizeof(T)); }

    protected:
        GpuBufferDesc m_BufferDesc{};
        void* m_MappedData{};
    };


    inline GpuBufferDesc GetReadBackBufferDesc(std::uint64_t size, std::uint32_t stride)
    {
        GpuBufferDesc bufferDesc;

        bufferDesc.m_Size = size;
        bufferDesc.m_Stride = stride;
        bufferDesc.m_HeapType = D3D12_HEAP_TYPE_READBACK;
        bufferDesc.m_Flags = D3D12_RESOURCE_FLAG_NONE;

        return bufferDesc;
    }

}

#endif