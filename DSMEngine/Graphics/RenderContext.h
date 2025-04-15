#pragma once
#ifndef __RENDERCONTEXT_H__
#define __RENDERCONTEXT_H__

#include "../pch.h"
#include "../Utilities/Singleton.h"
#include "CommandQueue.h"
#include "GpuResourceAllocator.h"

namespace DSM {
    class RenderContext : public Singleton<RenderContext>
    {
    public:
        RenderContext();
        ~RenderContext()
        {
            Shutdown();
        }

        void Create(bool requireDXRSupport);
        void Shutdown();
        
        ID3D12Device5* GetDevice() const{return m_pDevice.Get();}
        IDXGIFactory7* GetFactory() const{return m_pFactory.Get();}

        CommandQueue& GetCommandQueue(D3D12_COMMAND_LIST_TYPE listType = D3D12_COMMAND_LIST_TYPE_DIRECT) noexcept;
        CommandQueue& GetGraphicsQueue() noexcept { return m_GraphicsQueue; }
        CommandQueue& GetComputeQueue() noexcept { return m_ComputeQueue; }
        CommandQueue& GetCopyQueue() noexcept { return m_CopyQueue; }

        void CreateCommandList(
            D3D12_COMMAND_LIST_TYPE listType,
            ID3D12GraphicsCommandList** ppList,
            ID3D12CommandAllocator** ppAllocator);

        bool IsFenceComplete(std::uint64_t fenceValue) noexcept
        {
            return GetCommandQueue(D3D12_COMMAND_LIST_TYPE(fenceValue >> QUEUE_TYPE_MOVEBITS)).IsFenceComplete(fenceValue);
        }


    public:
        inline static bool sm_bTypedUAVLoadSupport_R11G11B10_FLOAT = false;
        inline static bool sm_bTypedUAVLoadSupport_R16G16B16A16_FLOAT = false;
        
    private:
        Microsoft::WRL::ComPtr<ID3D12Device5> m_pDevice{};
        Microsoft::WRL::ComPtr<IDXGIFactory7> m_pFactory{};

        CommandQueue m_GraphicsQueue;
        CommandQueue m_ComputeQueue;
        CommandQueue m_CopyQueue;

    };

    

#define g_RenderContext	RenderContext::GetInstance()
}

#endif

