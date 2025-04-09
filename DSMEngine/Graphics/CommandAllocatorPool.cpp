#include "CommandAllocatorPool.h"
#include "../Utilities/Macros.h"

namespace DSM{
    ID3D12CommandAllocator* CommandAllocatorPool::RequestAllocator(std::uint64_t completedFenceValue)
    {
        std::lock_guard<std::mutex> guard{m_AllocatorMutex};

        ID3D12CommandAllocator* pAllocator = nullptr;

        // 从已有的分配者获取
        if (!m_ReadyAllocators.empty()) {
            auto& [fenceValue, allocator] = m_ReadyAllocators.front();

            // 若命令队列已经执行完内部命令,重置后获取
            if (fenceValue <= completedFenceValue) {
                pAllocator = allocator;
                ASSERT_SUCCEEDED(pAllocator->Reset());
                m_ReadyAllocators.pop();
            }
        }

        // 获取失败则额外创建
        if (pAllocator == nullptr) {
            ASSERT_SUCCEEDED(m_pDevice->CreateCommandAllocator(m_CommandListType, IID_PPV_ARGS(&pAllocator)));
            std::wstring name = L"CommandAllocatorPool::pAllocator" + std::to_wstring(completedFenceValue);
            pAllocator->SetName(name.c_str());
            m_Allocators.emplace_back(pAllocator);
        }

        return pAllocator;
    }

    void CommandAllocatorPool::DiscardAllocator(std::uint64_t fenceValue, ID3D12CommandAllocator* pAllocator)
    {
        std::lock_guard<std::mutex> guard{m_AllocatorMutex};
        m_ReadyAllocators.emplace(std::make_pair(fenceValue, pAllocator));
    }
}
