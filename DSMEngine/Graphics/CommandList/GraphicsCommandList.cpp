#include "GraphicsCommandList.h"

#include "../DynamicDescriptorHeap.h"
#include "../RootSignature.h"


namespace DSM {
    void GraphicsCommandList::SetRootSignature(const RootSignature& rootSig)
    {
        if (m_CurrGraphicsRootSignature == rootSig.GetRootSignature()) return;

        m_CmdList->SetGraphicsRootSignature(rootSig.GetRootSignature());

        m_ViewDescriptorHeap->ParseGraphicsRootSignature(rootSig);
        m_SampleDescriptorHeap->ParseGraphicsRootSignature(rootSig);
    }

    void GraphicsCommandList::SetDescriptorTable(std::uint32_t rootIndex, D3D12_GPU_DESCRIPTOR_HANDLE firstHandle)
    {
        m_CmdList->SetGraphicsRootDescriptorTable(rootIndex, firstHandle);
    }

    void GraphicsCommandList::SetDynamicDescriptors(
        std::uint32_t rootIndex,
        std::uint32_t offset,
        std::uint32_t count,
        D3D12_CPU_DESCRIPTOR_HANDLE handles[])
    {
        m_ViewDescriptorHeap->SetGraphicsDescriptorHandle(rootIndex, offset, count , handles);
    }
}
