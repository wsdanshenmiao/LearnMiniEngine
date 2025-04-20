#pragma once
#ifndef __D3D12DESCRIPTORHANDLE__H__
#define __D3D12DESCRIPTORHANDLE__H__

#include <d3d12.h>
#include <vector>
#include <wrl/client.h>
#include <mutex>
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
        DescriptorHeap(const std::wstring& name, D3D12_DESCRIPTOR_HEAP_TYPE heapType, std::uint32_t heapSize);
        ~DescriptorHeap() = default;
        DSM_NONCOPYABLE(DescriptorHeap);

        void Clear();

        bool HasValidSpace(std::uint32_t numDescriptors) const noexcept;
        bool IsValidHandle(const DescriptorHandle& handle) const noexcept;
        DescriptorHandle Allocate(std::uint32_t count = 1);
        
        ID3D12DescriptorHeap* GetHeap() const noexcept;
        std::uint32_t GetOffsetOfHandle(const DescriptorHandle& handle) const noexcept; 
        std::uint32_t GetDescriptorSize() const noexcept;

        DescriptorHandle operator[](std::uint32_t index) const noexcept;
        
    private:
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_DescriptorHeap;
        std::uint32_t m_DescriptorSize{};
        
        DescriptorHandle m_FirstHandle{};
        LinearAllocator m_Allocator;
    };

    // 无上限的描述符堆，储存cpu可见的描述符
    class DescriptorAllocator
    {
    public:
        DescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE heapTyep)
            :m_HeapType(heapTyep){}
        
        D3D12_CPU_DESCRIPTOR_HANDLE Allocate(std::uint32_t count);

        static void DetroyAll(){ sm_DescriptorHeapPool.clear(); }


    protected:
        static constexpr std::uint32_t sm_NumDescriptorsPerHeap = 256;
        static std::vector<DescriptorHeap> sm_DescriptorHeapPool;
        static std::mutex sm_Mutex;


        const D3D12_DESCRIPTOR_HEAP_TYPE m_HeapType{};
        DescriptorHeap* m_CurrHeap{};
    };
    
}

#endif

