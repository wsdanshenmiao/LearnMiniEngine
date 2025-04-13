#pragma once
#ifndef __COMMANDBUFFER_H__
#define __COMMANDBUFFER_H__

#include "GpuBuffer.h"

namespace DSM {
    class GraphicsCommandBuffer;
    class ComputeCommandBuffer;

    
    // 对命令列表的封装
    class CommandBuffer
    {
    public:
        CommandBuffer();
        ~CommandBuffer();
        DSM_NONCOPYABLE(CommandBuffer);

        GraphicsCommandBuffer& GetGraphicsCommandBuffer()
        {
            ASSERT(m_CmdListType != D3D12_COMMAND_LIST_TYPE_COMPUTE, "Cannot convert async compute buffer to graphics")
            return reinterpret_cast<GraphicsCommandBuffer&>(*this);
        }
        ComputeCommandBuffer& GetComputeCommandBuffer()
        {
            return reinterpret_cast<ComputeCommandBuffer&>(*this);
        }

        ID3D12GraphicsCommandList* GetCommandList() { return m_CmdList; }

    protected:
        void SetID(const std::wstring& id) { m_ID = id; }
        
    protected:
        D3D12_COMMAND_LIST_TYPE m_CmdListType{};
        ID3D12GraphicsCommandList* m_CmdList{};
        ID3D12CommandAllocator* m_CurrAllocator{};

        ID3D12RootSignature* m_CurrGraphicsRootSignature{};
        ID3D12RootSignature* m_CurrComputeRootSignature{};
        ID3D12PipelineState* m_CurrPipelineState{};

        std::array<ID3D12DescriptorHeap*, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> m_CurrDescriptorHeaps{};

        std::wstring m_ID{};
    };

    class GraphicsCommandBuffer : public CommandBuffer
    {
        
    };

    class ComputeCommandBuffer : public CommandBuffer
    {
        
    };

}

#endif