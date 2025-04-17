#pragma once
#ifndef __DYNAMICDESCRIPTORHEAP_H__
#define __DYNAMICDESCRIPTORHEAP_H__

#include "DescriptorHeap.h"
#include <mutex>
#include <queue>

namespace DSM {
    class CommandList;
    
    class DynamicDescriptorHeap
    {
    public:
        using DescriptorHeapArray = std::vector<Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>>;
        using RetiredDescriptorHeapQueue = std::queue<std::pair<std::uint64_t, ID3D12DescriptorHeap*>>;
        using DescriptorHeapQueue = std::queue<ID3D12DescriptorHeap*>;


    public:
        static void DestroyAll();

    private:
        // 管理全局的描述符堆
        static std::array<DescriptorHeapArray, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> sm_DescriptorHeapPool;
        static std::array<RetiredDescriptorHeapQueue, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> sm_RetiredDescriptorHeaps;
        static std::array<DescriptorHeapQueue, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> sm_AvaildDescriptorHeaps;
        static constexpr std::uint32_t sm_NumDescriptorsPerHeap = 1024;
        static std::mutex sm_Mutex;

    private:
        const D3D12_DESCRIPTOR_HEAP_TYPE m_Type{};
        CommandList& m_OwningList;
        ID3D12DescriptorHeap* m_pCurrentHeap{};
        DescriptorHandle m_FirstHandle{};
        std::vector<ID3D12DescriptorHeap*> m_FullDescriptorHeaps{};
        std::uint32_t m_DescriptorSize{};
        std::uint32_t m_CurrOffset{};
        
    };

}


#endif