#pragma once
#ifndef __COMPUTECOMMANDLIST_H__
#define __COMPUTECOMMANDLIST_H__

#include "CommandList.h"
#include "../../Math/MathCommon.h"

namespace DSM {
    class RootSignature;
    class CommandSignature;
    
    class ComputeCommandList : public CommandList
    {
    public:
        ComputeCommandList(const std::wstring& id, bool async = false)
            :CommandList(id, async ? D3D12_COMMAND_LIST_TYPE_COMPUTE : D3D12_COMMAND_LIST_TYPE_DIRECT){}
        
        void SetRootSignature(const RootSignature& rootSig);

        void SetConstantArray(std::uint32_t rootIndex, std::uint32_t numConstants, const void * pConstants)
        {
            m_CmdList->SetComputeRoot32BitConstants(rootIndex, numConstants, pConstants, 0);
        }
        void SetConstant(std::uint32_t rootIndex, std::uint32_t offset, DWParam val)
        {
            m_CmdList->SetComputeRoot32BitConstant(rootIndex, val.Uint, offset);
        }
        void SetConstants(std::uint32_t rootIndex, DWParam x){ m_CmdList->SetComputeRoot32BitConstant(rootIndex, x.Uint, 0); }
        void SetConstants(std::uint32_t rootIndex, DWParam x, DWParam y)
        {
            m_CmdList->SetComputeRoot32BitConstant(rootIndex, x.Uint, 0);
            m_CmdList->SetComputeRoot32BitConstant(rootIndex, y.Uint, 1);
        }
        void SetConstants(std::uint32_t rootIndex, DWParam x, DWParam y, DWParam z);
        void SetConstants(std::uint32_t rootIndex, DWParam x, DWParam y, DWParam z, DWParam w);
        void SetDescriptorTable(std::uint32_t rootIndex, D3D12_GPU_DESCRIPTOR_HANDLE firstHandle)
        {
            m_CmdList->SetComputeRootDescriptorTable(rootIndex, firstHandle);
        }
        void SetConstantBuffer(std::uint32_t rootIndex, D3D12_GPU_VIRTUAL_ADDRESS cbv)
        {
            m_CmdList->SetComputeRootConstantBufferView(rootIndex, cbv);
        }
        void SetDynamicConstantBuffer(std::uint32_t rootIndex, std::size_t bufferSize, const void* pData);
        void SetShaderResource(std::uint32_t rootIndex, const GpuResource& resource, std::uint64_t offset = 0);
        void SetUnorderedAccess(std::uint32_t rootIndex, const GpuResource& resource, std::uint64_t offset = 0);
        void SetDynamicSRV(std::uint32_t rootIndex, std::size_t bufferSize, const void* pData);

        void SetDynamicDescriptor(std::uint32_t rootIndex,
            std::uint32_t offset,
            D3D12_CPU_DESCRIPTOR_HANDLE handle){ SetDynamicDescriptors(rootIndex, offset, 1, &handle); }
        void SetDynamicDescriptors(std::uint32_t rootIndex,
            std::uint32_t offset,
            std::uint32_t count,
            D3D12_CPU_DESCRIPTOR_HANDLE handles[]);
        void SetDynamicSample(std::uint32_t rootIndex,
            std::uint32_t offset,
            D3D12_CPU_DESCRIPTOR_HANDLE handle){ SetDynamicSamples(rootIndex, offset, 1, &handle); }
        void SetDynamicSamples(std::uint32_t rootIndex,
            std::uint32_t offset,
            std::uint32_t count,
            D3D12_CPU_DESCRIPTOR_HANDLE handles[]);

        void Dispatch(std::size_t groupCountX = 1, std::size_t groupCountY = 1, std::size_t groupCountZ = 1 );
        void Dispatch1D(std::size_t threadCountX, std::size_t groupSizeX = 64)
        {
            Dispatch(Math::DivideByMultiple(threadCountX, groupSizeX), 1, 1);
        }
        void Dispatch2D(std::size_t threadCountX, std::size_t threadCountY, std::size_t groupSizeX = 8, std::size_t groupSizeY = 8)
        {
            Dispatch(Math::DivideByMultiple(threadCountX, groupSizeX), Math::DivideByMultiple(threadCountY, groupSizeY), 1);
        }
        void Dispatch3D(std::size_t threadCountX, std::size_t threadCountY, std::size_t threadCountZ,
            std::size_t groupSizeX, std::size_t groupSizeY, std::size_t groupSizeZ )
        {
            Dispatch(Math::DivideByMultiple(threadCountX, groupSizeX),
                Math::DivideByMultiple(threadCountY, groupSizeY),
                Math::DivideByMultiple(threadCountZ, groupSizeZ));
        }
        void ExecuteIndirect(
            CommandSignature& cmdSig,
            GpuResource& argumentBuffer,
            std::uint64_t argumentOffset,
            std::uint32_t maxCommands = 1,
            GpuResource* counterBuffer = nullptr,
            std::uint64_t counterOffset = 0);
    };
}


#endif
