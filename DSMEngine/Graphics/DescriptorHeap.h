#pragma once
#ifndef __D3D12DESCRIPTORHANDLE__H__
#define __D3D12DESCRIPTORHANDLE__H__

#include <array>
#include <d3d12.h>
#include <vector>
#include <wrl/client.h>
#include <mutex>
#include <queue>
#include <set>
#include "../Utilities/LinearAllocator.h"
#include "../Utilities/Macros.h"

namespace DSM {
    // 描述符的句柄
    class DescriptorHandle
    {
    public:
        DescriptorHandle();
        DescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle);

        std::size_t GetCpuPtr() const noexcept;
        std::uint64_t GetGpuPtr() const noexcept;
        // 检测句柄是否有效
        bool IsValid() const noexcept;
        bool IsShaderVisible() const noexcept;

        // 便于偏移内部虚拟地址
        DescriptorHandle operator+(int offset) const noexcept;
        void operator+=(int offset) noexcept;
        const D3D12_CPU_DESCRIPTOR_HANDLE* operator&() const noexcept;
        // 隐式转换
        operator D3D12_CPU_DESCRIPTOR_HANDLE() const noexcept;
        operator D3D12_GPU_DESCRIPTOR_HANDLE() const noexcept;

    private:
        D3D12_CPU_DESCRIPTOR_HANDLE m_CPUHandle{};
        D3D12_GPU_DESCRIPTOR_HANDLE m_GPUHandle{};
    };


    class DescriptorHeap
    {
    public:
        DescriptorHeap(
            const std::wstring& name,
            D3D12_DESCRIPTOR_HEAP_TYPE heapType,
            std::uint32_t heapSize);
        DescriptorHeap(
            const std::wstring& name, 
            D3D12_DESCRIPTOR_HEAP_TYPE heapType, 
            std::uint32_t heapSize,
            D3D12_DESCRIPTOR_HEAP_FLAGS flags)
            :m_Allocator(heapSize) {
            Create(name, heapType, heapSize, flags);
        }
        ~DescriptorHeap() = default;
        DescriptorHeap(DescriptorHeap&&) = default;
        DescriptorHeap& operator=(DescriptorHeap&&) = default;
        DSM_NONCOPYABLE(DescriptorHeap);

        void Clear();

        bool HasValidSpace(std::uint32_t numDescriptors) const noexcept;
        bool IsValidHandle(const DescriptorHandle& handle) const noexcept;
        DescriptorHandle Allocate(std::uint32_t count = 1);
        
        ID3D12DescriptorHeap* GetHeap() const noexcept;
        std::uint32_t GetHeapSize() const noexcept { return m_Allocator.MaxSize(); }
        std::uint32_t GetOffsetOfHandle(const DescriptorHandle& handle) const noexcept; 
        std::uint32_t GetDescriptorSize() const noexcept;

        DescriptorHandle operator[](std::uint32_t index) const noexcept;
        
    private:
        void Create(
            const std::wstring& name,
            D3D12_DESCRIPTOR_HEAP_TYPE heapType,
            std::uint32_t heapSize,
            D3D12_DESCRIPTOR_HEAP_FLAGS flags);


    private:
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_DescriptorHeap;
        std::uint32_t m_DescriptorSize{};
        
        DescriptorHandle m_FirstHandle{};
        LinearAllocator m_Allocator;
    };


    
    /// <summary>
    /// 存放CPU可见的描述符
    /// </summary>
    class DescriptorAllocator
    {
    private:
        struct DescriptorPage
        {
            DescriptorHeap m_Heap;
            std::uint32_t m_UsedCount = 0;
        };
        
    public:
        DescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE heapTyep)
            :m_HeapType(heapTyep){}
        ~DescriptorAllocator();
        
        DescriptorHandle AllocateDescriptor(std::uint32_t count);
        void FreeDescriptor(const DescriptorHandle& handle, std::uint32_t count);

        static void DestroyAll()
        {
            sm_DescriptorPagePool.clear();
            sm_AvailablePages.fill({});
        }

    protected:
        inline static constexpr std::uint32_t sm_NumDescriptorsPerHeap = 256;
        inline static std::vector<std::unique_ptr<DescriptorPage>> sm_DescriptorPagePool{};
        inline static std::array<std::queue<DescriptorPage*>, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> sm_AvailablePages{};
        inline static std::mutex sm_Mutex{};
        
        const D3D12_DESCRIPTOR_HEAP_TYPE m_HeapType{};
        DescriptorPage* m_CurrHeap{};
        std::vector<DescriptorPage*> m_FullPages{};
    };
    
}

#endif

