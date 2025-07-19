#pragma once
#ifndef __GRAPHICSCOMMANDLIST_H__
#define __GRAPHICSCOMMANDLIST_H__

#include <algorithm>

#include "CommandList.h"

namespace DSM {
    class CommandSignature;
    class RootSignature;

    
    class GraphicsCommandList : public CommandList
    {
    public:
        GraphicsCommandList(const std::wstring& id)
            : CommandList(id){}
        void ClearRenderTarget(
            D3D12_CPU_DESCRIPTOR_HANDLE rtv,
            const float* clearColor = nullptr,
            const D3D12_RECT* clearRect = nullptr);
        void ClearDepth(D3D12_CPU_DESCRIPTOR_HANDLE dsv, float depth = 1)
        {
            FlushResourceBarriers();
            m_CmdList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, depth, 0, 0, nullptr);
        }
        void ClearStencil(D3D12_CPU_DESCRIPTOR_HANDLE dsv, float stencil = 0)
        {
            FlushResourceBarriers();
            m_CmdList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_STENCIL, 1, stencil, 0, nullptr);
        }
        void ClearDepthStencil(D3D12_CPU_DESCRIPTOR_HANDLE dsv, float depth = 1, std::uint8_t stencil = 0)
        {
            FlushResourceBarriers();
            m_CmdList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, depth, stencil, 0, nullptr);
        }
        
        void SetRootSignature(const RootSignature& rootSig);

        void SetRenderTargets(std::span<const D3D12_CPU_DESCRIPTOR_HANDLE> RTVs)
        {
            m_CmdList->OMSetRenderTargets(RTVs.size(), RTVs.empty() ? nullptr : RTVs.data(), false, nullptr);
        }
        void SetRenderTargets(
            std::span<const D3D12_CPU_DESCRIPTOR_HANDLE> RTVs,
            const D3D12_CPU_DESCRIPTOR_HANDLE dsv)
        {
            m_CmdList->OMSetRenderTargets(RTVs.size(), RTVs.empty() ? nullptr : RTVs.data(), false, &dsv);
        }
        void SetRenderTarget(const D3D12_CPU_DESCRIPTOR_HANDLE rtv){SetRenderTargets({&rtv, 1});}
        void SetRenderTarget(const D3D12_CPU_DESCRIPTOR_HANDLE rtv, const D3D12_CPU_DESCRIPTOR_HANDLE dsv){SetRenderTargets({&rtv, 1}, dsv);}
        void SetDepthStencilTarget(const D3D12_CPU_DESCRIPTOR_HANDLE dsv) { SetRenderTargets({}, dsv); }

        void SetViewport(const D3D12_VIEWPORT& viewport){ m_CmdList->RSSetViewports(1, &viewport);}
        void SetViewport(float x, float y, float width, float height, float minDepth = 0, float maxDepth = 1)
        {
            D3D12_VIEWPORT viewport{.TopLeftX = x,.TopLeftY = y,
                .Width = width, .Height = height, .MinDepth = minDepth, .MaxDepth = maxDepth};
            m_CmdList->RSSetViewports(1, &viewport);
        }
        void SetScissor(const D3D12_RECT& rect)
        {
            ASSERT(rect.left < rect.right && rect.top < rect.bottom);
            m_CmdList->RSSetScissorRects(1, &rect);
        }
        void SetScissor(long left, long top, long right, long botton)
        {
            D3D12_RECT rect{.left = left, .top = top, .right = right, .bottom = botton};
            SetScissor(rect);
        }
        void SetViewportAndScissor(const D3D12_VIEWPORT& viewport, const D3D12_RECT& rect)
        {
            SetViewport(viewport);
            SetScissor(rect);
        }
        void SetViewportAndScissor(long x, long y, long width, long height)
        {
            SetViewport(x, y, width, height);
            SetScissor(x, y, x + width, y + height);
        }
        void SetStencilRef(std::uint32_t ref){ m_CmdList->OMSetStencilRef(ref);}
        void SetBlendFactor(const float factor[4]){m_CmdList->OMSetBlendFactor(factor);}
        void SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY topology){m_CmdList->IASetPrimitiveTopology(topology);}

        template <typename T>
        void SetConstantArray(std::uint32_t rootIndex, std::uint32_t numConstants, const void * pConstants)
        {
            m_CmdList->SetGraphicsRoot32BitConstants(rootIndex, numConstants, pConstants, 0);
        }
        void SetConstant(std::uint32_t rootIndex, std::uint32_t offset, DWParam val)
        {
            m_CmdList->SetGraphicsRoot32BitConstant(rootIndex, val.Uint, offset);
        }
        void SetConstants(std::uint32_t rootIndex, DWParam x){ m_CmdList->SetGraphicsRoot32BitConstant(rootIndex, x.Uint, 0); }
        void SetConstants(std::uint32_t rootIndex, DWParam x, DWParam y)
        {
            m_CmdList->SetGraphicsRoot32BitConstant(rootIndex, x.Uint, 0);
            m_CmdList->SetGraphicsRoot32BitConstant(rootIndex, y.Uint, 1);
        }
        void SetConstants(std::uint32_t rootIndex, DWParam x, DWParam y, DWParam z);
        void SetConstants(std::uint32_t rootIndex, DWParam x, DWParam y, DWParam z, DWParam w);
        void SetDescriptorTable(std::uint32_t rootIndex, D3D12_GPU_DESCRIPTOR_HANDLE firstHandle)
        {
            m_CmdList->SetGraphicsRootDescriptorTable(rootIndex, firstHandle);
        }
        void SetConstantBuffer(std::uint32_t rootIndex, D3D12_GPU_VIRTUAL_ADDRESS cbv)
        {
            m_CmdList->SetGraphicsRootConstantBufferView(rootIndex, cbv);
        }
        void SetDynamicConstantBuffer(std::uint32_t rootIndex, std::size_t bufferSize, const void* pData);
        void SetShaderResource(std::uint32_t rootIndex, const GpuResource& resource, std::uint64_t offset = 0);
        void SetUnorderedAccess(std::uint32_t rootIndex, const GpuResource& resource, std::uint64_t offset = 0);
        void SetDynamicSRV(std::uint32_t rootIndex, std::size_t bufferSize, const void* pData);
        
        void SetDynamicDescriptor(std::uint32_t rootIndex,
            std::uint32_t offset,
            D3D12_CPU_DESCRIPTOR_HANDLE handle){ SetDynamicDescriptors(rootIndex, offset, {&handle, 1}); }
        void SetDynamicDescriptors(std::uint32_t rootIndex,
            std::uint32_t offset,
            std::span<D3D12_CPU_DESCRIPTOR_HANDLE> handles);
        void SetDynamicSample(std::uint32_t rootIndex,
            std::uint32_t offset,
            D3D12_CPU_DESCRIPTOR_HANDLE handle){ SetDynamicSamples(rootIndex, offset, {&handle, 1}); }
        void SetDynamicSamples(std::uint32_t rootIndex,
            std::uint32_t offset,
            std::span<D3D12_CPU_DESCRIPTOR_HANDLE> handles);

        void SetIndexBuffer(const D3D12_INDEX_BUFFER_VIEW& ibv){m_CmdList->IASetIndexBuffer(&ibv);}
        void SetVertexBuffer(std::uint32_t slot, const D3D12_VERTEX_BUFFER_VIEW& vbv) { SetVertexBuffers(slot, {&vbv, 1}); }
        void SetVertexBuffers(std::uint32_t startSlot, std::span<const D3D12_VERTEX_BUFFER_VIEW> VBVs)
        {
            ASSERT(VBVs.size() > 0);
            m_CmdList->IASetVertexBuffers(startSlot, VBVs.size(), VBVs.data());
        }
        void SetDynamicIB(std::size_t indexCount, const std::uint16_t* indexData);
        void SetDynamicIB(std::size_t indexCount, const std::uint32_t* indexData);
        template <typename T>
        void SetDynamicVB(std::uint32_t slot, std::size_t numVertex, const T* pData);

        void Draw(std::uint32_t vertexCount, std::uint32_t vertexStartOffset = 0) { DrawInstanced(vertexCount, 1, vertexStartOffset, 0); }
        void DrawIndexed(std::uint32_t indexCount,
            std::uint32_t startIndexLocation = 0,
            int baseVertexLocation = 0)
        {
            DrawIndexedInstanced(indexCount, 1, startIndexLocation, baseVertexLocation, 0);
        }
        void DrawInstanced(
            std::uint32_t vertexCountPerInstance,
            std::uint32_t instanceCount,
            std::uint32_t startVertexLocation = 0,
            std::uint32_t startInstanceLocation = 0);
        void DrawIndexedInstanced(
            std::uint32_t indexCountPerInstance,
            std::uint32_t instanceCount,
            std::uint32_t startIndexLocation,
            int baseVertexLocation,
            std::uint32_t startInstanceLocation);
        void ExecuteIndirect(
            CommandSignature& cmdSig,
            GpuResource& argumentBuffer,
            std::uint64_t argumentOffset,
            std::uint32_t maxCommands = 1,
            GpuResource* counterBuffer = nullptr,
            std::uint64_t counterOffset = 0);
    };

    template <typename T>
    void GraphicsCommandList::SetDynamicVB(std::uint32_t slot, std::size_t numVertex, const T* pData)
    {
        ASSERT(pData != nullptr && numVertex > 0);

        auto bufferSize = sizeof(T) * numVertex;
        auto uploadBuffer = GetUploadBuffer(bufferSize);
        memcpy(uploadBuffer.m_MappedAddress, pData, bufferSize);

        D3D12_VERTEX_BUFFER_VIEW vbv = {};
        vbv.BufferLocation = uploadBuffer.m_GpuAddress;
        vbv.SizeInBytes = bufferSize;
        vbv.StrideInBytes = sizeof(T);
        m_CmdList->IASetVertexBuffers(slot, 1, &vbv);
    }
}


#endif


