#pragma once
#ifndef __GPUBUFFER_H__
#define __GPUBUFFER_H__

#include "GpuResource.h"
#include "GraphicsCommon.h"

namespace DSM {
    // 用于创建 Buffer 的描述
    struct GpuBufferDesc
    {
        std::uint64_t m_Size = 0;
        DSMResourceUsage m_Usage = DSMResourceUsage::Default;
        DSMBindFlag m_BindFlag = DSMBindFlag::None;
        DSMBufferFlag m_BufferFlag = DSMBufferFlag::None;
        std::uint32_t m_Stride = 0;
        DXGI_FORMAT m_Format = DXGI_FORMAT_UNKNOWN;
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
        virtual void Destroy() override;

        const GpuBufferDesc& GetDesc() const noexcept { return m_BufferDesc; }
        std::uint64_t GetSize() const noexcept {return m_BufferDesc.m_Size; }
        std::uint32_t GetStride() const noexcept { return m_BufferDesc.m_Stride; }
        std::uint32_t GetCount() const
        {
            ASSERT(m_BufferDesc.m_Stride != 0);
            return static_cast<std::uint32_t>(m_BufferDesc.m_Size / m_BufferDesc.m_Stride);
        }
        DXGI_FORMAT GetFormat() const noexcept { return m_BufferDesc.m_Format; }

        bool Mappable() const noexcept
        {
            return m_BufferDesc.m_Usage == DSMResourceUsage::Upload ||
                m_BufferDesc.m_Usage == DSMResourceUsage::Readback ;
        }
        void* Map();
        void Unmap();
        template <typename T = void>
        T* GetMappedData() const { return reinterpret_cast<T*>(m_MappedData); }
        void Update(void* data, std::uint64_t size, std::uint64_t offset = 0);
        template <typename T>
        void Update(const T& data) { Update(&data, sizeof(T)); }

    protected:
        GpuBufferDesc m_BufferDesc{};
        void* m_MappedData{};
    };

}

#endif