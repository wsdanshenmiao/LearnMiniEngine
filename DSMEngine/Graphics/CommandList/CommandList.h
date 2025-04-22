#pragma once
#ifndef __COMMANDBUFFER_H__
#define __COMMANDBUFFER_H__

#include <d3d12.h>
#include <wrl/client.h>
#include <vector>
#include <array>
#include <span>
#include "../../Utilities/Macros.h"


namespace DSM {
    class GraphicsCommandList;
    class ComputeCommandList;
    class PipelineState;
    class PSO;
    class DynamicDescriptorHeap;
    class GpuResource;
    struct GpuResourceLocatioin;

    struct DWParam
    {
        DWParam( FLOAT f ) : Float(f) {}
        DWParam( UINT u ) : Uint(u) {}
        DWParam( INT i ) : Int(i) {}

        void operator= ( FLOAT f ) { Float = f; }
        void operator= ( UINT u ) { Uint = u; }
        void operator= ( INT i ) { Int = i; }

        union
        {
            FLOAT Float;
            UINT Uint;
            INT Int;
        };
    };

    
    // 对命令列表的封装
    class CommandList
    {
        friend class RenderContext;
    public:
        CommandList(const std::wstring& id = L"");
        ~CommandList();
        DSM_NONCOPYABLE(CommandList);

        GraphicsCommandList& GetGraphicsCommandList()
        {
            ASSERT(m_CmdListType != D3D12_COMMAND_LIST_TYPE_COMPUTE, "Cannot convert async compute list to graphics")
            return reinterpret_cast<GraphicsCommandList&>(*this);
        }
        ComputeCommandList& GetComputeCommandList()
        {
            return reinterpret_cast<ComputeCommandList&>(*this);
        }

        ID3D12GraphicsCommandList* GetCommandList() { return m_CmdList.Get(); }

        void Reset();
        void FlushResourceBarriers();
        
        void ClearUAV(GpuResource& resource, D3D12_CPU_DESCRIPTOR_HANDLE uav, const float* clearColor = nullptr);
        void ClearUAv(GpuResource& resource, D3D12_CPU_DESCRIPTOR_HANDLE uav, const std::uint32_t* clearColor = nullptr);
        
        void CopyResource(GpuResource& dest, GpuResource& src);
        void CopyBufferRegion(GpuResource& dest,
            std::size_t destOffset,
            GpuResource& src,
            std::size_t srcOffset,
            std::size_t numBytes);
        void CopySubresource(GpuResource& dest, std::uint32_t destIndex, GpuResource& src, std::uint32_t srcIndex);
        void CopyTextureRegion(GpuResource& dest,
            std::uint32_t x,
            std::uint32_t y,
            std::uint32_t z,
            GpuResource& src,
            const RECT& rect);
        void WriteBuffer(GpuResource& dest, std::size_t destOffset, const void* data, std::size_t byteSize);
        void FillBuffer(GpuResource& dest, std::size_t destOffset, DWParam value, std::size_t byteSize);

        void InsertUAVBarrier(GpuResource& resource, bool flush = false);
        void TransitionResource(GpuResource& resource, D3D12_RESOURCE_STATES newState, bool flush = false);

        GpuResourceLocatioin GetUploadBuffer(std::uint64_t bufferSize, std::uint32_t alignment = 0);

        void SetDescriptorHeap(ID3D12DescriptorHeap* descriptorHeap);
        void SetDescriptorHeaps(std::uint32_t count , ID3D12DescriptorHeap** descriptorHeaps);
        void SetPipelineState(PSO& pso);


        static void InitTexture(GpuResource& dest, std::span<D3D12_SUBRESOURCE_DATA> subResources);
        static void InitBuffer(GpuResource& dest, const void* data, std::size_t byteSize, std::size_t destOffset = 0);
        static void InitTextureArraySlice(GpuResource& dest, std::uint32_t sliceIndex, GpuResource& src);

        
    protected:
        void BindDescriptorHeaps();
        
    protected:
        D3D12_COMMAND_LIST_TYPE m_CmdListType{};
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_CmdList{};
        ID3D12CommandAllocator* m_CurrAllocator{};

        ID3D12RootSignature* m_CurrGraphicsRootSignature{};
        ID3D12RootSignature* m_CurrComputeRootSignature{};
        ID3D12PipelineState* m_CurrPipelineState{};

        DynamicDescriptorHeap* m_ViewDescriptorHeap{};
        DynamicDescriptorHeap* m_SampleDescriptorHeap{};

        std::vector<D3D12_RESOURCE_BARRIER> m_ResourceBarriers{};
        std::array<ID3D12DescriptorHeap*, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> m_CurrDescriptorHeaps{};
    };

}

#endif