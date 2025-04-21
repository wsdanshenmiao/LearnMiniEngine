#pragma once
#ifndef __GRAPHICSCOMMANDLIST_H__
#define __GRAPHICSCOMMANDLIST_H__

#include "CommandList.h"

namespace DSM {
    class RootSignature;

    
    class GraphicsCommandList : public CommandList
    {
    public:
        void SetRootSignature(const RootSignature& rootSig);
        
        void SetDescriptorTable(std::uint32_t rootIndex, D3D12_GPU_DESCRIPTOR_HANDLE firstHandle);
        void SetDynamicDescriptor(std::uint32_t rootIndex,
            std::uint32_t offset,
            D3D12_CPU_DESCRIPTOR_HANDLE handle){ SetDynamicDescriptors(rootIndex, offset, 1, &handle); }
        void SetDynamicDescriptors(std::uint32_t rootIndex,
            std::uint32_t offset,
            std::uint32_t count,
            D3D12_CPU_DESCRIPTOR_HANDLE handles[]);
    };
}


#endif


