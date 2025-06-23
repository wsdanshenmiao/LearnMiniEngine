#include "DynamicDescriptorHeap.h"
#include "DescriptorHeap.h"
#include "RenderContext.h"
#include "RootSignature.h"
#include "CommandList/CommandList.h"

namespace DSM {
    class DynamicDescriptorHeapAllocator
    {
    public:
        using DescriptorHeapArray = std::vector<std::unique_ptr<DescriptorHeap>>;
        using RetiredDescriptorHeapQueue = std::queue<std::pair<std::uint64_t, DescriptorHeap*>>;
        using DescriptorHeapQueue = std::queue<DescriptorHeap*>;

        DescriptorHeap* RequestDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType)
        {
            int index = heapType == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV ? 0 : 1;
            
            std::lock_guard lock{m_Mutex};

            while (!m_RetiredDescriptorHeaps[index].empty() &&
                g_RenderContext.IsFenceComplete(m_RetiredDescriptorHeaps[index].front().first)) {
                m_AvaildDescriptorHeaps[index].push(m_RetiredDescriptorHeaps[index].front().second);
                m_RetiredDescriptorHeaps[index].pop();
                }

            if (m_AvaildDescriptorHeaps[index].empty()) {
                D3D12_DESCRIPTOR_HEAP_DESC desc = {};
                desc.Type = heapType;
                desc.NumDescriptors = m_NumDescriptorsPerHeap;
                desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
                desc.NodeMask = 1;
                auto newHeap = new DescriptorHeap{L"DynamicDescriptorHeapManager::DescriptorHeap", heapType, m_NumDescriptorsPerHeap};
                m_DescriptorHeapPool[index].emplace_back(newHeap);
                return newHeap;
            }
            else {
                auto ptr = m_AvaildDescriptorHeaps[index].front();
                m_AvaildDescriptorHeaps[index].pop();
                return ptr;
            }    
        }
        
        void DiscardDescriptorHeap(
            D3D12_DESCRIPTOR_HEAP_TYPE heapType,
            std::uint64_t fenceValue,
            const std::vector<DescriptorHeap*>& descriptorHeaps)
        {
            int index = heapType == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV ? 0 : 1;
            
            std::lock_guard lock{m_Mutex};

            for (const auto& heap : descriptorHeaps) {
                m_RetiredDescriptorHeaps[index].push(std::make_pair(fenceValue, heap));
                heap->Clear();
            }
        }
        
        void DestroyAll()
        {
            for (int i = 0; i < 2; ++i) {
                m_DescriptorHeapPool[i].clear();
                while (m_RetiredDescriptorHeaps[i].empty()) {
                    m_RetiredDescriptorHeaps[i].pop();
                }
                while (m_AvaildDescriptorHeaps[i].empty()) {
                    m_AvaildDescriptorHeaps[i].pop();
                }
            }
        }

    private:
        // 管理需要绑定到渲染管线上的描述符
        std::array<DescriptorHeapArray, 2> m_DescriptorHeapPool;
        std::array<RetiredDescriptorHeapQueue, 2> m_RetiredDescriptorHeaps;
        std::array<DescriptorHeapQueue, 2> m_AvaildDescriptorHeaps;
        const std::uint32_t m_NumDescriptorsPerHeap = 1024;
        std::mutex m_Mutex;
    };

    static DynamicDescriptorHeapAllocator s_DynamicDescriptorHeapManager;





    
    //
    // DescriptorHandleCache
    //
    void DynamicDescriptorHeap::DescriptorHandleCache::ParseRootSignature(
        D3D12_DESCRIPTOR_HEAP_TYPE heapType,
        const RootSignature& rootSig)
    {
        // 重新绑定根签名
        m_StaleRootParamsBitMap = 0;
        m_RootDescriptorTableBitMap = heapType == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV ?
            rootSig.GetDescriptorTableBitMap() : rootSig.GetSamplerTableBitMap();

        
        std::uint32_t offset = 0;
        auto tableBitMap = m_RootDescriptorTableBitMap;
        unsigned long rootParamSize{};
        unsigned long rootIndex{};
        if (_BitScanReverse(&rootParamSize, tableBitMap)) {
            m_DescriptorTables.resize(rootParamSize + 1);
        }
        // 从最前面的位开始解析根签名的布局
        while (_BitScanForward(&rootIndex, tableBitMap)) {
            // 使用异或移除当前位
            tableBitMap ^= (1 << rootIndex);

            auto tableSize = rootSig.GetDescriptorTableSize(rootIndex);
            ASSERT(tableSize > 0);

            auto& desciptorTable = m_DescriptorTables[rootIndex];
            desciptorTable.m_AssignedHandlesBitMap = 0;
            desciptorTable.m_TableHandles.resize(tableSize);
        }
    }

    std::uint32_t DynamicDescriptorHeap::DescriptorHandleCache::ComputeStaledSize() const
    {
        std::uint32_t usedSize = 0;
        unsigned long rootIndex{};
        auto staleParam = m_StaleRootParamsBitMap;
        // 计算每一个绑定过的根参数
        while (_BitScanForward(&rootIndex, staleParam)) {
            staleParam ^= (1 << rootIndex);

            unsigned long maxBindTableSize{};
            ASSERT(_BitScanReverse(&maxBindTableSize, m_DescriptorTables[rootIndex].m_AssignedHandlesBitMap));
            usedSize += maxBindTableSize + 1;
        }
        return usedSize;
    }

    void DynamicDescriptorHeap::DescriptorHandleCache::UnbindAllValid()
    {
        m_StaleRootParamsBitMap = 0;

        std::uint32_t tableBitMap = m_RootDescriptorTableBitMap;
        unsigned long rootIndex{};
        while (_BitScanForward(&rootIndex, tableBitMap)) {
            tableBitMap ^= (1 << rootIndex);

            // 若绑定了描述符则记录
            if (m_DescriptorTables[rootIndex].m_AssignedHandlesBitMap != 0) {
                m_StaleRootParamsBitMap |= (1 << rootIndex);
            }
        }
    }

    void DynamicDescriptorHeap::DescriptorHandleCache::StageDescriptorHandles(
        std::uint32_t rootIndex,
        std::uint32_t offset,
        std::uint32_t numHandles,
        const D3D12_CPU_DESCRIPTOR_HANDLE handles[])
    {
        ASSERT((1 << rootIndex) & m_RootDescriptorTableBitMap,
            "Root paramter is not a CBV_SRV_UAV or Sample descriptor table");
        ASSERT(offset + numHandles <= m_DescriptorTables[rootIndex].m_TableHandles.size());

        auto dest = m_DescriptorTables[rootIndex].m_TableHandles.data() + offset;
        memcpy(dest, handles, numHandles * sizeof(D3D12_CPU_DESCRIPTOR_HANDLE));
        m_DescriptorTables[rootIndex].m_AssignedHandlesBitMap |= ((1 << numHandles) - 1) << offset;
        m_StaleRootParamsBitMap |= (1 << rootIndex);
    }

    void DynamicDescriptorHeap::DescriptorHandleCache::CopyAndBindStaleTables(D3D12_DESCRIPTOR_HEAP_TYPE heapType,
        std::uint32_t descriptorSize, DescriptorHandle handleStart,
        std::function<void(UINT, D3D12_GPU_DESCRIPTOR_HANDLE)> setFunc)
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
            std::uint32_t setHandle = descriptorTable.m_AssignedHandlesBitMap;
            auto srcHandle = descriptorTable.m_TableHandles.data();
            D3D12_CPU_DESCRIPTOR_HANDLE destHandle = handleStart;

            unsigned long skipCount{};
            while (_BitScanForward(&skipCount, setHandle)) {
                // 跳过未绑定的描述符
                setHandle >>= skipCount;
                srcHandle += skipCount;
                destHandle.ptr += skipCount * descriptorSize;

                // 计算描述符的数量,取反之后第一个索引即为数量
                unsigned long descriptorCount{};
                _BitScanForward(&descriptorCount, ~setHandle);
                setHandle >>= descriptorCount;

                // 若描述符超过16个，进行拷贝
                if (numDestDescriptorRanges + descriptorCount > maxDescriptorPerCopy) {
                    g_RenderContext.GetDevice()->CopyDescriptors(
                        numDestDescriptorRanges,pDestDescriptorRangesStarts,pDestDescriptorRangesSizes,
                        numSrcDescriptorRanges,pSrcDescriptorRangesStarts,pSrcDescriptorRangesSizes,heapType);
                }

                // 设置目标范围
                pDestDescriptorRangesStarts[numDestDescriptorRanges] = destHandle;
                pDestDescriptorRangesSizes[numDestDescriptorRanges] = descriptorCount;
                ++numDestDescriptorRanges;

                // 设置源的范围，由于只拷贝设置了描述符的描述符表索引，因此不一定是连续的
                for (int j = 0 ; j < descriptorCount; ++j) {
                    pSrcDescriptorRangesStarts[numDestDescriptorRanges] = srcHandle[j];
                    pSrcDescriptorRangesSizes[numDestDescriptorRanges] = 1;
                    ++numDestDescriptorRanges;
                }

                // 进行偏移
                destHandle.ptr += descriptorSize * descriptorSize;
                srcHandle += descriptorSize;
            }
            g_RenderContext.GetDevice()->CopyDescriptors(
                numDestDescriptorRanges,pDestDescriptorRangesStarts,pDestDescriptorRangesSizes,
                numSrcDescriptorRanges,pSrcDescriptorRangesStarts,pSrcDescriptorRangesSizes, heapType);
        }
    }




    //
    // DynamicDescriptorHeap
    //
    void DynamicDescriptorHeap::DescriptorHandleCache::Cleanup()
    {
        m_StaleRootParamsBitMap = 0;
        m_RootDescriptorTableBitMap = 0;
        m_DescriptorTables.clear();
    }

    void DynamicDescriptorHeap::CommitGraphicsRootDescriptorTables()
    {
        if (m_GraphicsHandleCache.m_StaleRootParamsBitMap != 0) {
            auto func = [&](UINT rootIndex, D3D12_GPU_DESCRIPTOR_HANDLE handle) {
                m_OwningCmdList->GetCommandList()->SetGraphicsRootDescriptorTable(rootIndex, handle);
            };
            CopyAndBindStaleTables(m_GraphicsHandleCache, func);
        }
    }

    void DynamicDescriptorHeap::CommitComputeRootDescriptorTables()
    {
        if (m_GraphicsHandleCache.m_StaleRootParamsBitMap != 0) {
            auto func = [&](UINT rootIndex, D3D12_GPU_DESCRIPTOR_HANDLE handle) {
                m_OwningCmdList->GetCommandList()->SetGraphicsRootDescriptorTable(rootIndex, handle);
            };
            CopyAndBindStaleTables(m_GraphicsHandleCache, func);
        }
    }

    D3D12_GPU_DESCRIPTOR_HANDLE DynamicDescriptorHeap::UploadDirect(D3D12_CPU_DESCRIPTOR_HANDLE handle)
    {
        if (m_pCurrentHeap == nullptr || !m_pCurrentHeap->HasValidSpace(1)){
            RequestDescriptorHeap();
            m_GraphicsHandleCache.UnbindAllValid();
            m_ComputeHandleCache.UnbindAllValid();
        }

        m_OwningCmdList->SetDescriptorHeap(m_pCurrentHeap->GetHeap());
        auto gpuHanlde = m_pCurrentHeap->Allocate();
        g_RenderContext.GetDevice()->CopyDescriptorsSimple(1, gpuHanlde, handle, m_HeapType);

        return gpuHanlde;
    }

    void DynamicDescriptorHeap::Cleanup(std::uint64_t fenceValue)
    {
        if (m_pCurrentHeap != nullptr) {
            m_FullDescriptorHeaps.push_back(m_pCurrentHeap);
            m_pCurrentHeap = nullptr;
        }
        s_DynamicDescriptorHeapManager.DiscardDescriptorHeap(m_HeapType, fenceValue, m_FullDescriptorHeaps);
        m_FullDescriptorHeaps.clear();
        m_GraphicsHandleCache.Cleanup();
        m_ComputeHandleCache.Cleanup();
    }

    DynamicDescriptorHeap* DynamicDescriptorHeap::AllocateDynamicDescriptorHeap(
        CommandList* owningList,
        D3D12_DESCRIPTOR_HEAP_TYPE heapType)
    {
        int index = heapType == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV ? 0 : 1;
        DynamicDescriptorHeap* ret{};

        std::lock_guard lock{sm_Mutex};
        
        if (sm_AvailableDescriptorHeaps.empty()) {
            ret = sm_AvailableDescriptorHeaps[index].front();
            sm_AvailableDescriptorHeaps[index].pop();
        }
        else {
            ret = new DynamicDescriptorHeap{heapType};
            sm_DynamicDescriptorHeapPools[index].emplace_back(ret);
        }
        ret->m_OwningCmdList = owningList;
        return ret;
    }

    void DynamicDescriptorHeap::FreeDynamicDescriptorHeap(std::uint64_t fenceValue, DynamicDescriptorHeap* heap)
    {
        ASSERT(heap != nullptr);
        heap->Cleanup(fenceValue);

        std::lock_guard lock{sm_Mutex};
        int index = heap->m_HeapType == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV ? 0 : 1;
        sm_AvailableDescriptorHeaps[index].push(heap);
    }

    void DynamicDescriptorHeap::DestroyAll()
    {
        for (int i = 0;i<sm_DynamicDescriptorHeapPools.size();++i) {
            while (sm_AvailableDescriptorHeaps[i].empty()) {
                sm_AvailableDescriptorHeaps[i].pop();
            }
            sm_DynamicDescriptorHeapPools[i].clear();
        }
        s_DynamicDescriptorHeapManager.DestroyAll();
    }

    void DynamicDescriptorHeap::CopyAndBindStaleTables(DescriptorHandleCache& handleCache,
        std::function<void(UINT, D3D12_GPU_DESCRIPTOR_HANDLE)> setFunc)
    {
        auto usedSize = handleCache.ComputeStaledSize();
        if (m_pCurrentHeap == nullptr || !m_pCurrentHeap->HasValidSpace(usedSize)) {
            RequestDescriptorHeap();
            m_GraphicsHandleCache.UnbindAllValid();
            m_ComputeHandleCache.UnbindAllValid();
        }

        m_OwningCmdList->SetDescriptorHeap(m_pCurrentHeap->GetHeap());
        handleCache.CopyAndBindStaleTables(m_HeapType, usedSize, m_pCurrentHeap->Allocate(usedSize), setFunc);
    }

    void DynamicDescriptorHeap::RequestDescriptorHeap()
    {
        if (m_pCurrentHeap != nullptr) {
            m_FullDescriptorHeaps.push_back(m_pCurrentHeap);
        }

        m_pCurrentHeap = s_DynamicDescriptorHeapManager.RequestDescriptorHeap(m_HeapType);
    }
}
