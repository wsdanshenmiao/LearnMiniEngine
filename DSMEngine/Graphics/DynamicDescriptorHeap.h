#pragma once
#ifndef __DYNAMICDESCRIPTORHEAP_H__
#define __DYNAMICDESCRIPTORHEAP_H__

#include <queue>
#include <functional>
#include <array>
#include "DescriptorHeap.h"

namespace DSM {
    class RootSignature;
    class CommandList;
    class DescriptorHandle;
    
    class DynamicDescriptorHeap
    {
    private:
        // 描述符表的缓冲
        struct DescriptorTableCache
        {
            // 用于描述描述符表中绑定了多少描述符
            std::uint32_t m_AssignedHandlesBitMap{};
            std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> m_TableHandles;
        };
        // 储存所有的描述符表及描述符
        struct DescriptorHandleCache
        {
            // 从根签名中获取的描述符表的布局
            std::uint32_t m_RootDescriptorTableBitMap{};
            // 用于记录有哪些根签名绑定了资源
            std::uint32_t m_StaleRootParamsBitMap{};

            std::vector<DescriptorTableCache> m_DescriptorTables{};

            // 解析根签名
            void ParseRootSignature(D3D12_DESCRIPTOR_HEAP_TYPE heapType, const RootSignature& rootSig);
            // 计算需要使用的描述符
            std::uint32_t ComputeStaledSize() const;
            // 解除先前的绑定并重新计算需要绑定的根参数
            void UnbindAllValid();
            // 将需要绑定到渲染管线的描述符储存到缓冲区中
            void StageDescriptorHandles(
                std::uint32_t rootIndex,
                std::uint32_t offset,
                std::uint32_t numHandles,
                const D3D12_CPU_DESCRIPTOR_HANDLE handles[]);
            void CopyAndBindStaleTables(
                D3D12_DESCRIPTOR_HEAP_TYPE heapType,
                std::uint32_t descriptorSize,
                DescriptorHandle handleStart,
                std::function<void(UINT, D3D12_GPU_DESCRIPTOR_HANDLE)> setFunc);
            void Cleanup();
        };
        
    public:
        DynamicDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType)
            :m_HeapType(heapType){}
        DSM_NONCOPYABLE(DynamicDescriptorHeap);

        void Reset();

        // 通过根签名的解析结果检测资源绑定是否正确，同时将CPU描述符拷贝到GPU可见的描述符中
        void SetGraphicsDescriptorHandle(
            std::uint32_t rootIndex,
            std::uint32_t offset,
            std::uint32_t numHandle,
            D3D12_CPU_DESCRIPTOR_HANDLE handles[])
        {
            m_GraphicsHandleCache.StageDescriptorHandles(rootIndex, offset, numHandle, handles);
        }
        void SetComputeDescriptorHandle(
            std::uint32_t rootIndex,
            std::uint32_t offset,
            std::uint32_t numHandle,
            D3D12_CPU_DESCRIPTOR_HANDLE handles[])
        {
            m_ComputeHandleCache.StageDescriptorHandles(rootIndex, offset, numHandle, handles);
        }

        // 解析根描述符
        void ParseGraphicsRootSignature(const RootSignature& rootSig)
        {
            m_GraphicsHandleCache.ParseRootSignature(m_HeapType, rootSig);
        }
        void ParseComputeRootSignature(const RootSignature& rootSig)
        {
            m_ComputeHandleCache.ParseRootSignature(m_HeapType, rootSig);
        }

        void CommitGraphicsRootDescriptorTables();
        void CommitComputeRootDescriptorTables();
        
        D3D12_GPU_DESCRIPTOR_HANDLE UploadDirect( D3D12_CPU_DESCRIPTOR_HANDLE handle);

        // 回收使用完毕的描述符堆
        void Cleanup(std::uint64_t fenceValue);

        static DynamicDescriptorHeap* AllocateDynamicDescriptorHeap(CommandList* owningList, D3D12_DESCRIPTOR_HEAP_TYPE heapType);
        static void FreeDynamicDescriptorHeap(std::uint64_t fenceValue, DynamicDescriptorHeap* heap);
        static void DestroyAll();

    private:
        void CopyAndBindStaleTables(
            DescriptorHandleCache& handleCache,
            std::function<void(UINT, D3D12_GPU_DESCRIPTOR_HANDLE)> setFunc);
        void RequestDescriptorHeap();
        
    private:
        using DynamicDescriptorHeapPool = std::vector<std::unique_ptr<DynamicDescriptorHeap>>;
        inline static std::array<DynamicDescriptorHeapPool, 2> sm_DynamicDescriptorHeapPools{};
        inline static std::array<std::queue<DynamicDescriptorHeap*>, 2> sm_AvailableDescriptorHeaps{};
        inline static std::mutex sm_Mutex;
        
        CommandList* m_OwningCmdList{};
        const D3D12_DESCRIPTOR_HEAP_TYPE m_HeapType{};
        DescriptorHeap* m_pCurrentHeap{};
        std::vector<DescriptorHeap*> m_FullDescriptorHeaps{};

        DescriptorHandleCache m_GraphicsHandleCache{};
        DescriptorHandleCache m_ComputeHandleCache{};
    };

    

}


#endif