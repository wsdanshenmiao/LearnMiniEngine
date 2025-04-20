#include "DynamicDescriptorHeap.h"
#include "DescriptorHeap.h"
#include "RenderContext.h"
#include "RootSignature.h"


namespace DSM {
    class DynamicDescriptorHeapManager
    {
    public:
        using DescriptorHeapArray = std::vector<DescriptorHeap>;
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
                auto& newHeap = m_DescriptorHeapPool[index].emplace_back(
                    L"DynamicDescriptorHeapManager::DescriptorHeap", heapType, m_NumDescriptorsPerHeap);
                return &newHeap;
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
        constexpr std::uint32_t m_NumDescriptorsPerHeap = 1024;
        std::mutex m_Mutex;
    };

    static DynamicDescriptorHeapManager s_DynamicDescriptorHeapManager;





    
    //
    // DescriptorHandleCache
    //
    void DynamicDescriptorHeap::DescriptorHandleCache::ParseRootSignature(
        D3D12_DESCRIPTOR_HEAP_TYPE heapType,
        const RootSignature* rootSig)
    {
        // 重新绑定根签名
        m_StaleRootParamsBitMap = 0;
        m_RootDescriptorTableBitMap = heapType == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV ?
            rootSig->GetDescriptorTableBitMap() : rootSig->GetSamplerTableBitMap();

        
        std::uint32_t offset = 0;
        auto tableBitMap = m_RootDescriptorTableBitMap;
        unsigned long rootParamSize{};
        unsigned long rootIndex{};
        _BitScanForward(&rootParamSize, tableBitMap);
        m_DescriptorTables.resize(rootParamSize + 1);
        // 从最前面的位开始解析根签名的布局
        while (_BitScanForward(&rootIndex, tableBitMap)) {
            // 使用异或移除当前位
            tableBitMap ^= (1 << rootIndex);

            auto tableSize = rootSig->GetDescriptorTableSize(rootIndex);
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
}
