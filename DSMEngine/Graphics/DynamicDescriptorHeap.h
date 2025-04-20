#pragma once
#ifndef __DYNAMICDESCRIPTORHEAP_H__
#define __DYNAMICDESCRIPTORHEAP_H__

#include <d3d12.h>
#include <queue>
#include "../Utilities/Macros.h"

namespace DSM {
    class DescriptorHeap;
    class RootSignature;
    
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
            void ParseRootSignature(D3D12_DESCRIPTOR_HEAP_TYPE heapType, const RootSignature* rootSig);
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
            template <typename SetFunc>
            void CopyAndBindStaleTables(
                D3D12_DESCRIPTOR_HEAP_TYPE heapType,
                std::uint32_t descriptorSize,
                D3D12_CPU_DESCRIPTOR_HANDLE handleStart,
                SetFunc setFunc);
        };
        
    public:
        DynamicDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType);

        // 通过根签名的解析结果检测资源绑定是否正确，同时将CPU描述符拷贝到GPU可见的描述符中
        void SetGraphicsDescriptorHandle(
            std::uint32_t rootIndex,
            std::uint32_t offset,
            std::uint32_t numHandle,
            D3D12_CPU_DESCRIPTOR_HANDLE handles[]);
        void SetComputeDescriptorHandle(
            std::uint32_t rootIndex,
            std::uint32_t offset,
            std::uint32_t numHandle,
            D3D12_CPU_DESCRIPTOR_HANDLE handles[]);

        // 解析根描述符
        void ParseGraphicsRootSignature(const RootSignature& rootSig);
        void ParseComputeRootSignature(const RootSignature& rootSig);

        // 回收使用完毕的描述符堆
        void Cleanup(std::uint64_t fenceValue);

        static void DestroyAll();
        
    private:
        const D3D12_DESCRIPTOR_HEAP_TYPE m_HeapType{};
        DescriptorHeap* m_pCurrentHeap{};
        std::vector<DescriptorHeap*> m_FullDescriptorHeaps{};
    };

    
    template <typename SetFunc>
    inline void DynamicDescriptorHeap::DescriptorHandleCache::CopyAndBindStaleTables(
        D3D12_DESCRIPTOR_HEAP_TYPE heapType,
        std::uint32_t descriptorSize,
        D3D12_CPU_DESCRIPTOR_HANDLE handleStart,
        SetFunc setFunc)
    {
        std::uint32_t staleParamCount{};
        std::vector<std::uint32_t> usedTableSizes(m_DescriptorTables.size());
        std::vector<std::uint32_t> rootIndexs(m_DescriptorTables.size());
        auto staleBitMap = m_StaleRootParamsBitMap;
        m_StaleRootParamsBitMap = 0;
        unsigned long rootIndex{};
        // 获取绑定了资源的根参数
        while (_BitScanForward(&rootIndex, staleBitMap)) {
            staleBitMap ^= (1 << rootIndex);

            auto& descriptorTable = m_DescriptorTables[rootIndex];
            ASSERT(_BitScanReverse(&rootIndex, descriptorTable.m_AssignedHandlesBitMap));
            
            usedTableSizes[staleParamCount] = descriptorTable.m_TableHandles.size();
            rootIndexs[staleParamCount] = rootIndex;
            ++staleParamCount;
        }

        // 每次拷贝 16 个
        static constexpr std::uint32_t maxDescriptorPerCopy = 16;
        std::uint32_t numDestDescriptorRanges= 0 ;
        D3D12_CPU_DESCRIPTOR_HANDLE pDestDescriptorRangesStarts[maxDescriptorPerCopy];
        std::uint32_t pDestDescriptorRangesSizes[maxDescriptorPerCopy];
        std::uint32_t numSrcDescriptorRanges = 0;
        D3D12_CPU_DESCRIPTOR_HANDLE pSrcDescriptorRangesStarts[maxDescriptorPerCopy];
        std::uint32_t pSrcDescriptorRangesSizes[maxDescriptorPerCopy];
        
        // 每一个描述符表
        for (std::uint32_t i = 0; i < staleParamCount; ++i) {
            rootIndex = rootIndexs[i];
            setFunc(rootIndex, handleStart);

            auto& descriptorTable = m_DescriptorTables[rootIndex];
        }
    }
}


#endif