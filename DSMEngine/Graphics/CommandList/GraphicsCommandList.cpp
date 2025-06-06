#include "GraphicsCommandList.h"
#include "../DynamicDescriptorHeap.h"
#include "../RootSignature.h"
#include "../Resource/GpuResource.h"
#include "../Resource/DynamicBufferAllocator.h"
#include "Graphics/CommandSignature.h"

namespace DSM {
    void GraphicsCommandList::ClearRenderTarget(
        D3D12_CPU_DESCRIPTOR_HANDLE rtv,
        const float* clearColor,
        const D3D12_RECT* clearRect)
    {
        FlushResourceBarriers();

        const float defaultColor[4] = {};
        clearColor = clearColor == nullptr ? defaultColor : clearColor;
        m_CmdList->ClearRenderTargetView(rtv, clearColor, clearRect == nullptr ? 0 : 1, clearRect);
    }

    void GraphicsCommandList::SetRootSignature(const RootSignature& rootSig)
    {
        if (m_CurrGraphicsRootSignature == rootSig.GetRootSignature()) return;

        m_CmdList->SetGraphicsRootSignature(rootSig.GetRootSignature());

        //TODO:取消注释后会报错
        //m_ViewDescriptorHeap->ParseGraphicsRootSignature(rootSig);
        //m_SampleDescriptorHeap->ParseGraphicsRootSignature(rootSig);

        m_CurrGraphicsRootSignature = rootSig.GetRootSignature();
    }

    void GraphicsCommandList::SetConstants(std::uint32_t rootIndex, DWParam x, DWParam y, DWParam z)
    {
        m_CmdList->SetGraphicsRoot32BitConstant(rootIndex, x.Uint, 0);
        m_CmdList->SetGraphicsRoot32BitConstant(rootIndex, y.Uint, 1);
        m_CmdList->SetGraphicsRoot32BitConstant(rootIndex, z.Uint, 2);
    }

    void GraphicsCommandList::SetConstants(std::uint32_t rootIndex, DWParam x, DWParam y, DWParam z, DWParam w)
    {
        m_CmdList->SetGraphicsRoot32BitConstant(rootIndex, x.Uint, 0);
        m_CmdList->SetGraphicsRoot32BitConstant(rootIndex, y.Uint, 1);
        m_CmdList->SetGraphicsRoot32BitConstant(rootIndex, z.Uint, 2);
        m_CmdList->SetGraphicsRoot32BitConstant(rootIndex, w.Uint, 3);
    }

    void GraphicsCommandList::SetDynamicConstantBuffer(std::uint32_t rootIndex, std::size_t bufferSize, const void* pData)
    {
        ASSERT(pData != nullptr && bufferSize > 0);
        auto uploadBuffer = GetUploadBuffer(bufferSize, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
        memcpy(uploadBuffer.m_MappedAddress, pData, bufferSize);
        m_CmdList->SetGraphicsRootConstantBufferView(rootIndex, uploadBuffer.m_GpuAddress);
    }

    void GraphicsCommandList::SetShaderResource(std::uint32_t rootIndex, const GpuResource& resource, std::uint64_t offset)
    {
        auto state = (D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        ASSERT(resource.GetUsageState() & state != 0);
        m_CmdList->SetGraphicsRootShaderResourceView(rootIndex, resource.GetGpuVirtualAddress() + offset);
    }

    void GraphicsCommandList::SetUnorderedAccess(std::uint32_t rootIndex, const GpuResource& resource,std::uint64_t offset)
    {
        ASSERT(resource.GetUsageState() & D3D12_RESOURCE_STATE_UNORDERED_ACCESS != 0);
        m_CmdList->SetGraphicsRootUnorderedAccessView(rootIndex, resource.GetGpuVirtualAddress() + offset);
    }

    void GraphicsCommandList::SetDynamicSRV(std::uint32_t rootIndex, std::size_t bufferSize, const void* pData)
    {
        ASSERT(bufferSize > 0 && pData != nullptr);

        auto uploadBuffer = GetUploadBuffer(bufferSize);
        memcpy(uploadBuffer.m_MappedAddress, pData, bufferSize);

        m_CmdList->SetGraphicsRootShaderResourceView(rootIndex, uploadBuffer.m_GpuAddress);
    }
    
    void GraphicsCommandList::SetDynamicDescriptors(
        std::uint32_t rootIndex,
        std::uint32_t offset,
        std::span<D3D12_CPU_DESCRIPTOR_HANDLE> handles)
    {
        ASSERT(handles.size() > 0);
        m_ViewDescriptorHeap->SetGraphicsDescriptorHandle(rootIndex, offset, handles.size(), handles.data());
    }

    void GraphicsCommandList::SetDynamicSamples(
        std::uint32_t rootIndex,
        std::uint32_t offset,
        std::span<D3D12_CPU_DESCRIPTOR_HANDLE> handles)
    {
        ASSERT(handles.size() > 0);
        m_SampleDescriptorHeap->SetGraphicsDescriptorHandle(rootIndex, offset, handles.size(), handles.data());
    }

    void GraphicsCommandList::SetDynamicIB(std::size_t indexCount, const std::uint16_t* indexData)
    {
        ASSERT(indexData != nullptr && indexCount > 0);
        
        auto bufferSize = indexCount * sizeof(std::uint16_t);
        auto uploadBuffer = GetUploadBuffer(bufferSize);
        memcpy(uploadBuffer.m_MappedAddress, indexData, bufferSize);

        D3D12_INDEX_BUFFER_VIEW indexView = {};
        indexView.Format = DXGI_FORMAT_R16_UINT;
        indexView.BufferLocation = uploadBuffer.m_GpuAddress;
        indexView.SizeInBytes = bufferSize;
        m_CmdList->IASetIndexBuffer(&indexView);
    }

    void GraphicsCommandList::SetDynamicIB(std::size_t indexCount, const std::uint32_t* indexData)
    {
        ASSERT(indexData != nullptr && indexCount > 0);
        
        auto bufferSize = indexCount * sizeof(std::uint32_t);
        auto uploadBuffer = GetUploadBuffer(bufferSize);
        memcpy(uploadBuffer.m_MappedAddress, indexData, bufferSize);

        D3D12_INDEX_BUFFER_VIEW indexView = {};
        indexView.Format = DXGI_FORMAT_R32_UINT;
        indexView.BufferLocation = uploadBuffer.m_GpuAddress;
        indexView.SizeInBytes = bufferSize;
        m_CmdList->IASetIndexBuffer(&indexView);
    }

    void GraphicsCommandList::DrawInstanced(std::uint32_t vertexCountPerInstance, std::uint32_t instanceCount,
                                            std::uint32_t startVertexLocation, std::uint32_t startInstanceLocation)
    {
        FlushResourceBarriers();
        
        m_ViewDescriptorHeap->CommitGraphicsRootDescriptorTables();
        m_SampleDescriptorHeap->CommitGraphicsRootDescriptorTables();
        m_CmdList->DrawInstanced(vertexCountPerInstance, instanceCount, startVertexLocation, startInstanceLocation);
    }

    void GraphicsCommandList::DrawIndexedInstanced(
        std::uint32_t indexCountPerInstance,
        std::uint32_t instanceCount,
        std::uint32_t startIndexLocation,
        int baseVertexLocation,
        std::uint32_t startInstanceLocation)
    {
        FlushResourceBarriers();

        m_ViewDescriptorHeap->CommitGraphicsRootDescriptorTables();
        m_SampleDescriptorHeap->CommitGraphicsRootDescriptorTables();
        m_CmdList->DrawIndexedInstanced(
            indexCountPerInstance, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
    }

    void GraphicsCommandList::ExecuteIndirect(
        CommandSignature& cmdSig,
        GpuResource& argumentBuffer,
        std::uint64_t argumentOffset,
        std::uint32_t maxCommands,
        GpuResource* counterBuffer,
        std::uint64_t counterOffset)
    {
        FlushResourceBarriers();
        m_ViewDescriptorHeap->CommitGraphicsRootDescriptorTables();
        m_SampleDescriptorHeap->CommitGraphicsRootDescriptorTables();
        m_CmdList->ExecuteIndirect(
            cmdSig.GetCommandSignature(), maxCommands, argumentBuffer.GetResource(), argumentOffset,
            counterBuffer == nullptr ? nullptr : counterBuffer->GetResource(), counterOffset);
    }
}
