#pragma once
#ifndef __COMMANDQUEUE_H__
#define __COMMANDQUEUE_H__

#include "CommandAllocatorPool.h"

namespace DSM {
    class CommandQueue
    {
        friend class RenderContext;
        friend class CommandBuffer;
    public:
        CommandQueue(D3D12_COMMAND_LIST_TYPE listType) noexcept;
        ~CommandQueue()
        {
            Shutdown();
        }

        bool IsReady() const noexcept {return m_pCommandQueue != nullptr;}
        
        // 创建命令队列及栅栏
        void Create(ID3D12Device* device);
        void Shutdown();

        // 增加栅栏值
        std::uint64_t IncrementFence(void);
        bool IsFenceComplete(std::uint64_t fenceValue);
        // GPU 进行等待
        void StallForFence(std::uint64_t fenceValue);
        void StallForProducer(CommandQueue& producer);
        // CPU 进行等待
        void WaitForFence(std::uint64_t fenceValue);
        void WaitForIdle(void) { WaitForFence(IncrementFence()); }

        ID3D12CommandQueue* GetCommandQueue() const {return m_pCommandQueue.Get();}
        std::uint64_t GetNextFenceValue() {return m_NextFenceValue;}

    protected:
        std::uint64_t ExecuteCommandList(ID3D12CommandList* list);
        ID3D12CommandAllocator* RequestCommandAllocator();
        void DiscardCommandAllocator(std::uint64_t fenceValueForReset, ID3D12CommandAllocator* allocator);

    protected:
        const D3D12_COMMAND_LIST_TYPE m_CommandListType{};
        Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_pCommandQueue{};

        // 用于 CPU 与 GPU 同步的栅栏
        Microsoft::WRL::ComPtr<ID3D12Fence> m_pFence{};
        std::uint64_t m_NextFenceValue{};
        std::uint64_t m_LastCompletedFenceValue{};
        std::mutex m_FenceMutex{};

        // 当前命令队列的分配池
        CommandAllocatorPool m_CommandAllocatorPool;

        HANDLE m_FenceEventHandle{};
        std::mutex m_EventMutex{};
    };
}
#endif