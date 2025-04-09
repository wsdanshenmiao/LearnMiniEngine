#pragma once
#ifndef __COMMANDALLOCATORPOOL_H__
#define __COMMANDALLOCATORPOOL_H__

#define NOMINMAX

#include <d3d12.h>
#include <mutex>
#include <queue>
#include <wrl/client.h>

namespace DSM {
    class CommandAllocatorPool
    {
    public:
        CommandAllocatorPool(D3D12_COMMAND_LIST_TYPE listType) noexcept
            :m_CommandListType(listType), m_pDevice(nullptr){}
        ~CommandAllocatorPool() noexcept
        {
            Shutdown();
        }

        void Create(ID3D12Device* pDevice) noexcept
        {
            m_pDevice = pDevice;
        }
        void Shutdown() noexcept
        {
            m_Allocators.clear();
        }

        // 请求一个命令分配
        ID3D12CommandAllocator* RequestAllocator(std::uint64_t completedFenceValue);
        // 提交命令列表后回收
        void DiscardAllocator(std::uint64_t fenceValue, ID3D12CommandAllocator* pAllocator);

        std::size_t Size() const noexcept{ return m_Allocators.size(); }

    private:
        std::mutex m_AllocatorMutex;

        const D3D12_COMMAND_LIST_TYPE m_CommandListType;
        ID3D12Device* m_pDevice = nullptr;

        std::vector<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>> m_Allocators;
        std::queue<std::pair<std::uint64_t, ID3D12CommandAllocator*>> m_ReadyAllocators;
    };
}

#endif
