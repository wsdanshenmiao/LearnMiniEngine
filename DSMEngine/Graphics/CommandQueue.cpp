#include "CommandQueue.h"
#include "../Utilities/Macros.h"
#include "RenderContext.h"

namespace DSM {
    CommandQueue::CommandQueue(D3D12_COMMAND_LIST_TYPE listType) noexcept
        :m_CommandListType(listType),
        m_NextFenceValue((uint64_t)m_CommandListType << 56 | 1),
        m_LastCompletedFenceValue((uint64_t)m_CommandListType << 56),
        m_CommandAllocatorPool(listType){
    }

    void CommandQueue::Create(ID3D12Device* device)
    {
        ASSERT(device != nullptr);
        ASSERT(!IsReady());
        ASSERT(m_CommandAllocatorPool.Size() == 0);

        // 创建队列
        D3D12_COMMAND_QUEUE_DESC queueDesc{};
        queueDesc.Type = m_CommandListType;
        queueDesc.NodeMask = 1;
        ASSERT_SUCCEEDED(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(m_pCommandQueue.GetAddressOf())));

        // 创建栅栏
        ASSERT_SUCCEEDED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_pFence.GetAddressOf())));
        m_pFence->SetName(L"CommandQueue::m_pFence");
        // 区分不同队列的 Fence
        m_pFence->Signal((std::uint64_t)m_CommandListType << 56);

        // 创建事件
        m_FenceEventHandle = CreateEvent(nullptr, false, false, nullptr);
        ASSERT(m_FenceEventHandle != nullptr);
        
        m_CommandAllocatorPool.Create(device);
        
        ASSERT(IsReady());
    }

    void CommandQueue::Shutdown()
    {
        if (m_pCommandQueue == nullptr) return;

        m_CommandAllocatorPool.Shutdown();
        CloseHandle(m_FenceEventHandle);
    }

    std::uint64_t CommandQueue::IncrementFence()
    {
        std::lock_guard<std::mutex> guard{m_FenceMutex};

        m_pCommandQueue->Signal(m_pFence.Get(), m_NextFenceValue);
        return m_NextFenceValue++;
    }

    bool CommandQueue::IsFenceComplete(std::uint64_t fenceValue)
    {
        std::lock_guard<std::mutex> guard{m_FenceMutex};

        if (m_LastCompletedFenceValue < fenceValue) {
            m_LastCompletedFenceValue = std::max(m_LastCompletedFenceValue, m_pFence->GetCompletedValue());
        }

        return fenceValue <= m_LastCompletedFenceValue;
    }

    void CommandQueue::StallForFence(std::uint64_t fenceValue)
    {
        // 当前队列等待其他队列
        auto& producer = g_RenderContext.GetCommandQueue((D3D12_COMMAND_LIST_TYPE)(fenceValue >> 56));
        m_pCommandQueue->Wait(producer.m_pFence.Get(), fenceValue);
    }

    void CommandQueue::StallForProducer(CommandQueue& producer)
    {
        ASSERT(producer.GetNextFenceValue() > 0);
        m_pCommandQueue->Wait(producer.m_pFence.Get(), producer.GetNextFenceValue() - 1);
    }

    void CommandQueue::WaitForFence(std::uint64_t fenceValue)
    {
        // 已经执行过了则无需等待
        if (IsFenceComplete(fenceValue)) return;

        // 等待 GPU 执行到栅栏点
        std::lock_guard<std::mutex> guard{m_EventMutex};
        m_pFence->SetEventOnCompletion(fenceValue, m_FenceEventHandle);
        WaitForSingleObject(m_FenceEventHandle, INFINITE);
        m_LastCompletedFenceValue = fenceValue;
    }

    std::uint64_t CommandQueue::ExecuteCommandList(ID3D12CommandList* list)
    {
        ASSERT(list != nullptr);

        ASSERT_SUCCEEDED(((ID3D12GraphicsCommandList*)list)->Close());

        std::lock_guard<std::mutex> guard{m_EventMutex};
        m_pCommandQueue->Signal(m_pFence.Get(), m_NextFenceValue);
        return m_NextFenceValue++;
    }

    ID3D12CommandAllocator* CommandQueue::RequestCommandAllocator()
    {
        return m_CommandAllocatorPool.RequestAllocator(m_pFence->GetCompletedValue());
    }

    void CommandQueue::DiscardCommandAllocator(std::uint64_t fenceValueForReset, ID3D12CommandAllocator* allocator)
    {
        ASSERT(allocator != nullptr);

        m_CommandAllocatorPool.DiscardAllocator(fenceValueForReset, allocator);
    }
}
