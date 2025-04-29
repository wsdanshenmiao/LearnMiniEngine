#include "ComputeCommandList.h"
#include "../DynamicDescriptorHeap.h"
#include "../RootSignature.h"
#include "../Resource/DynamicBufferAllocator.h"


namespace DSM {

    void ComputeCommandList::SetRootSignature(const RootSignature& rootSig)
    {
        if (m_CurrComputeRootSignature == rootSig.GetRootSignature()) return;

        m_CmdList->SetComputeRootSignature(rootSig.GetRootSignature());
        
        m_ViewDescriptorHeap->ParseComputeRootSignature(rootSig);
        m_SampleDescriptorHeap->ParseComputeRootSignature(rootSig);

        m_CurrComputeRootSignature = rootSig.GetRootSignature();
    }

    
    void ComputeCommandList::SetConstants(std::uint32_t rootIndex, DWParam x, DWParam y, DWParam z)
    {
        m_CmdList->SetComputeRoot32BitConstant(rootIndex, x.Uint, 0);
        m_CmdList->SetComputeRoot32BitConstant(rootIndex, y.Uint, 1);
        m_CmdList->SetComputeRoot32BitConstant(rootIndex, z.Uint, 2);
    }

    void ComputeCommandList::SetConstants(std::uint32_t rootIndex, DWParam x, DWParam y, DWParam z, DWParam w)
    {
        m_CmdList->SetComputeRoot32BitConstant(rootIndex, x.Uint, 0);
        m_CmdList->SetComputeRoot32BitConstant(rootIndex, y.Uint, 1);
        m_CmdList->SetComputeRoot32BitConstant(rootIndex, z.Uint, 2);
        m_CmdList->SetComputeRoot32BitConstant(rootIndex, w.Uint, 3);
    }

    void ComputeCommandList::SetDynamicConstantBuffer(std::uint32_t rootIndex, std::size_t bufferSize, const void* pData)
    {
        ASSERT(pData != nullptr && bufferSize > 0);
        auto uploadBuffer = GetUploadBuffer(bufferSize, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
        memcpy(uploadBuffer.m_MappedAddress, pData, bufferSize);
        m_CmdList->SetComputeRootConstantBufferView(rootIndex, uploadBuffer.m_GpuAddress);
    }

    void ComputeCommandList::SetShaderResource(std::uint32_t rootIndex, const GpuResource& resource, std::uint64_t offset)
    {
        auto state = (D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        ASSERT(resource.GetUsageState() & state != 0);
        m_CmdList->SetComputeRootShaderResourceView(rootIndex, resource.GetGpuVirtualAddress() + offset);
    }

    void ComputeCommandList::SetUnorderedAccess(std::uint32_t rootIndex, const GpuResource& resource,std::uint64_t offset)
    {
        ASSERT(resource.GetUsageState() & D3D12_RESOURCE_STATE_UNORDERED_ACCESS != 0);
        m_CmdList->SetComputeRootUnorderedAccessView(rootIndex, resource.GetGpuVirtualAddress() + offset);
    }

    void ComputeCommandList::SetDynamicSRV(std::uint32_t rootIndex, std::size_t bufferSize, const void* pData)
    {
        ASSERT(bufferSize > 0 && pData != nullptr);

        auto uploadBuffer = GetUploadBuffer(bufferSize);
        memcpy(uploadBuffer.m_MappedAddress, pData, bufferSize);

        m_CmdList->SetComputeRootShaderResourceView(rootIndex, uploadBuffer.m_GpuAddress);
    }
    
    void ComputeCommandList::SetDynamicDescriptors(
        std::uint32_t rootIndex,
        std::uint32_t offset,
        std::uint32_t count,
        D3D12_CPU_DESCRIPTOR_HANDLE handles[])
    {
        m_ViewDescriptorHeap->SetComputeDescriptorHandle(rootIndex, offset, count, handles);
    }

    void ComputeCommandList::SetDynamicSamples(
        std::uint32_t rootIndex,
        std::uint32_t offset,
        std::uint32_t count,
        D3D12_CPU_DESCRIPTOR_HANDLE handles[])
    {
        m_SampleDescriptorHeap->SetComputeDescriptorHandle(rootIndex, offset, count, handles);
    }

    void ComputeCommandList::Dispatch(std::size_t groupCountX, std::size_t groupCountY, std::size_t groupCountZ)
    {
        FlushResourceBarriers();
        m_ViewDescriptorHeap->CommitComputeRootDescriptorTables();
        m_SampleDescriptorHeap->CommitComputeRootDescriptorTables();
        m_CmdList->Dispatch(groupCountX, groupCountY, groupCountZ);
    }
}
