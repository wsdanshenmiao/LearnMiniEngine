#include "CommandList.h"

#include "RenderContext.h"

namespace DSM {
    CommandList::CommandList(const std::wstring& id)
    {
        auto listName = id + L" CommandList";
        g_RenderContext.CreateCommandList(m_CmdListType, &m_CmdList, &m_CurrAllocator);
        m_CmdList->SetName(listName.c_str());
        m_ResourceBarriers.reserve(16);
    }

    CommandList::~CommandList()
    {
        // 归还当前的命令堆
        auto& cmdQueue = g_RenderContext.GetCommandQueue(m_CmdListType);
        cmdQueue.DiscardCommandAllocator(cmdQueue.GetNextFenceValue(), m_CurrAllocator);
    }

    void CommandList::Reset()
    {
        m_CmdList->Reset(m_CurrAllocator, nullptr);

        if (m_CurrComputeRootSignature != nullptr) {
            m_CmdList->SetComputeRootSignature(m_CurrComputeRootSignature);
        }
        if (m_CurrGraphicsRootSignature != nullptr) {
            m_CmdList->SetGraphicsRootSignature(m_CurrGraphicsRootSignature);
        }
        if (m_CurrPipelineState != nullptr) {
            m_CmdList->SetPipelineState(m_CurrPipelineState);
        }

        BindDescriptorHeaps();
    }

    void CommandList::FlushResourceBarriers()
    {
        if (!m_ResourceBarriers.empty()) {
            m_CmdList->ResourceBarrier(m_ResourceBarriers.size(), m_ResourceBarriers.data());
        }
        m_ResourceBarriers.clear();
    }

    void CommandList::CopyResource(GpuResource& dest, GpuResource& src)
    {
        TransitionResource(dest, D3D12_RESOURCE_STATE_COPY_DEST);
        TransitionResource(src, D3D12_RESOURCE_STATE_COPY_SOURCE);
        FlushResourceBarriers();
        m_CmdList->CopyResource(dest.GetResource(), src.GetResource());
    }

    void CommandList::CopyBufferRegion(
        GpuResource& dest,
        std::size_t destOffset,
        GpuResource& src,
        std::size_t srcOffset,
        std::size_t numBytes)
    {
        TransitionResource(dest, D3D12_RESOURCE_STATE_COPY_DEST);
        TransitionResource(src, D3D12_RESOURCE_STATE_COPY_SOURCE);
        FlushResourceBarriers();
        m_CmdList->CopyBufferRegion(dest.GetResource(), destOffset, src.GetResource(), srcOffset, numBytes);
    }

    void CommandList::CopySubresource(GpuResource& dest, std::uint32_t destIndex, GpuResource& src, std::uint32_t srcIndex)
    {
        TransitionResource(dest, D3D12_RESOURCE_STATE_COPY_DEST);
        TransitionResource(src, D3D12_RESOURCE_STATE_COPY_SOURCE);
        FlushResourceBarriers();
        
        D3D12_TEXTURE_COPY_LOCATION destLocation{};
        destLocation.pResource = dest.GetResource();
        destLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        destLocation.SubresourceIndex = destIndex;

        D3D12_TEXTURE_COPY_LOCATION srcLocation{};
        srcLocation.pResource = src.GetResource();
        srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        srcLocation.SubresourceIndex = srcIndex;

        m_CmdList->CopyTextureRegion(&destLocation, 0, 0, 0, &srcLocation, nullptr);
    }

    void CommandList::CopyTextureRegion(
        GpuResource& dest,
        std::uint32_t x,
        std::uint32_t y,
        std::uint32_t z,
        GpuResource& src,
        const RECT& rect)
    {
        TransitionResource(dest, D3D12_RESOURCE_STATE_COPY_DEST);
        TransitionResource(src, D3D12_RESOURCE_STATE_COPY_SOURCE);
        FlushResourceBarriers();

        D3D12_TEXTURE_COPY_LOCATION destLocation{};
        destLocation.pResource = dest.GetResource();
        destLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        destLocation.SubresourceIndex = 0;

        D3D12_TEXTURE_COPY_LOCATION srcLocation{};
        srcLocation.pResource = src.GetResource();
        srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        srcLocation.SubresourceIndex = 0;
        
        D3D12_BOX srcBox{};
        srcBox.left = rect.left;
        srcBox.top = rect.top;
        srcBox.right = rect.right;
        srcBox.bottom = rect.bottom;
        srcBox.front = 0;
        srcBox.back = 1;

        m_CmdList->CopyTextureRegion(&destLocation, x, y, z, &srcLocation, &srcBox);
    }

    void CommandList::WriteBuffer(GpuResource& dest, std::size_t destOffset, const void* data, std::size_t byteSize)
    {
        auto uploadBuffer = GetUploadBuffer(byteSize);
        memcpy(uploadBuffer.m_MappedAddress, data, byteSize);
        CopyBufferRegion(dest, destOffset, *uploadBuffer.m_Resource, uploadBuffer.m_Offset, byteSize);
    }

    void CommandList::FillBuffer(GpuResource& dest, std::size_t destOffset, DWParam value, std::size_t byteSize)
    {
        auto uploadBuffer = GetUploadBuffer(byteSize);
        std::vector<DWParam> data{};
        data.resize(Utility::AlignUp(byteSize, sizeof(DWParam)) / sizeof(DWParam), value);
        memcpy(uploadBuffer.m_MappedAddress, data.data(), byteSize);
        CopyBufferRegion(dest, destOffset, *uploadBuffer.m_Resource, uploadBuffer.m_Offset, byteSize);
    }

    void CommandList::InsertUAVBarrier(GpuResource& resource, bool flush)
    {
        D3D12_RESOURCE_BARRIER resourceBarrier = {};
        resourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        resourceBarrier.UAV.pResource = resource.GetResource();
        resourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        m_ResourceBarriers.push_back(std::move(resourceBarrier));

        if (flush) {
            FlushResourceBarriers();
        }
    }

    void CommandList::TransitionResource(GpuResource& resource, D3D12_RESOURCE_STATES newState, bool flush)
    {
        auto preState = resource.GetUsageState();
        if (m_CmdListType == D3D12_COMMAND_LIST_TYPE_COMPUTE) {
            auto valieComputeQueueResourceState =
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS |
                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
                D3D12_RESOURCE_STATE_COPY_DEST |
                D3D12_RESOURCE_STATE_COPY_SOURCE;
            ASSERT(preState & valieComputeQueueResourceState == preState);
            ASSERT(newState & valieComputeQueueResourceState == newState);
        }

        if (preState == newState) {
            D3D12_RESOURCE_BARRIER resourceBarrier = {};
            resourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            resourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            resourceBarrier.Transition.pResource = resource.GetResource();
            resourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            resourceBarrier.Transition.StateBefore = resource.GetUsageState();
            resourceBarrier.Transition.StateAfter = newState;
            m_ResourceBarriers.push_back(std::move(resourceBarrier));

            resource.SetUsageState(newState);
        }
        else if (newState == D3D12_RESOURCE_STATE_UNORDERED_ACCESS) {
            InsertUAVBarrier(resource, flush);
        }

        if (m_ResourceBarriers.size() >= 16 || flush) {
            FlushResourceBarriers();
        }
    }

    GpuResourceLocatioin CommandList::GetUploadBuffer(std::uint64_t bufferSize, std::uint32_t alignment)
    {
        return g_RenderContext.GetCpuBufferAllocator().Allocate(bufferSize, alignment);
    }

    void CommandList::SetDescriptorHeap(ID3D12DescriptorHeap* descriptorHeap)
    {
        SetDescriptorHeaps(1, &descriptorHeap);
    }

    void CommandList::SetDescriptorHeaps(std::uint32_t count, ID3D12DescriptorHeap** descriptorHeaps)
    {
        bool hasChanged = false;
        for (UINT i = 0; i < count; i++) {
            auto type = descriptorHeaps[i]->GetDesc().Type;
            if (m_CurrDescriptorHeaps[type] != descriptorHeaps[i]) {
                m_CurrDescriptorHeaps[type] = descriptorHeaps[i];
                hasChanged = true;
            }
        }
        if (hasChanged) {
            BindDescriptorHeaps();
        }
    }

    void CommandList::SetPipelineState(PSO& pso)
    {
        auto pipelineState = pso.GetPipelineStateObject();
        if (pipelineState != m_CurrPipelineState) {
            m_CmdList->SetPipelineState(m_CurrPipelineState);
            m_CurrPipelineState = pipelineState;
        }
    }


    

    void CommandList::InitTexture(GpuResource& dest, std::uint32_t numSubResource, D3D12_SUBRESOURCE_DATA subResources[])
    {
        // 获取拷贝信息
        Microsoft::WRL::ComPtr<ID3D12Device> pDevice;
        dest->GetDevice(IID_PPV_ARGS(pDevice.GetAddressOf()));
        std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> footprint(numSubResource);	// 子资源的宽高偏移等信息
        std::vector<std::uint32_t> numRows(numSubResource);	// 子资源的行数
        std::vector<std::uint64_t> rowByteSize(numSubResource);	// 子资源每一行的字节大小
        std::uint64_t uploadBufferSize{};	// 整个纹理数据的大小
        const auto& texDesc = dest->GetDesc();
        pDevice->GetCopyableFootprints(
            &texDesc, 0,
            numSubResource, 0,
            footprint.data(), numRows.data(),
            rowByteSize.data(), &uploadBufferSize);
        
        CommandList cmdList{L"InitTexture"};
        
        auto uploadBuffer = cmdList.GetUploadBuffer(uploadBufferSize);

        // 拷贝纹理资源
        BYTE* mappedData = reinterpret_cast<BYTE*>(uploadBuffer.m_MappedAddress);
        // 每一个子资源
        for (std::uint32_t i = 0; i < numSubResource; i++) {
            BYTE* destData = mappedData + footprint[i].Offset;
            auto srcData = reinterpret_cast<const BYTE*>(subResources[i].pData);
            // 每一个深度
            for (std::uint32_t z = 0; z < footprint[i].Footprint.Depth; z++) {
                auto destDepthOffsetData = destData + footprint[i].Footprint.RowPitch * numRows[i] * z;
                auto srcDepthOffsetData = srcData + subResources[i].SlicePitch * z;
                // 每一行
                for (std::uint32_t y = 0; y < numRows[i]; y++) {
                    auto destRowOffsetData = destDepthOffsetData + footprint[i].Footprint.RowPitch * y;
                    auto srcRowOffsetData = srcDepthOffsetData + subResources[i].RowPitch * y;
                    memcpy(destRowOffsetData, srcRowOffsetData, rowByteSize[i]);
                }
            }
        }

        cmdList.TransitionResource(dest, D3D12_RESOURCE_STATE_COPY_DEST);
        cmdList.FlushResourceBarriers();
        
        // 拷贝所有子资源
        for (std::size_t i = 0; i < numSubResource; i++) {
            D3D12_TEXTURE_COPY_LOCATION destLocation{};
            destLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            destLocation.SubresourceIndex = i;
            destLocation.pResource = dest.GetResource();

            D3D12_TEXTURE_COPY_LOCATION src{};
            src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
            src.PlacedFootprint = footprint[i];
            src.PlacedFootprint.Offset += uploadBuffer.m_Offset;
            src.pResource = uploadBuffer.m_Resource->GetResource();
            cmdList.m_CmdList->CopyTextureRegion(&destLocation,0,0,0,&src,nullptr);
        }
        
        cmdList.TransitionResource(dest, D3D12_RESOURCE_STATE_GENERIC_READ);

        // 等待GPU完成拷贝工作
        g_RenderContext.ExecuteCommandList(&cmdList, true);
    }

    void CommandList::InitBuffer(GpuResource& dest, const void* data, std::size_t byteSize, std::size_t destOffset)
    {
        CommandList cmdList{L"InitBuffer"};
        cmdList.WriteBuffer(dest, destOffset, data, byteSize);
        g_RenderContext.ExecuteCommandList(&cmdList, true);
    }

    void CommandList::InitTextureArraySlice(GpuResource& dest, std::uint32_t sliceIndex, GpuResource& src)
    {
        CommandList cmdList{L"InitTextureArraySlice"};

        const auto& destDesc = dest->GetDesc();
        const auto& srcDesc = src->GetDesc();

        ASSERT(sliceIndex < srcDesc.DepthOrArraySize && srcDesc.DepthOrArraySize == 1 &&
            destDesc.Width == srcDesc.Width && destDesc.Height == srcDesc.Height &&
            destDesc.MipLevels <= srcDesc.MipLevels);

        cmdList.TransitionResource(dest, D3D12_RESOURCE_STATE_COPY_DEST);
        cmdList.TransitionResource(src, D3D12_RESOURCE_STATE_COPY_SOURCE);
        cmdList.FlushResourceBarriers();

        // 将所有的mipmap拷贝到纹理中
        auto subResourceIndex = sliceIndex + destDesc.MipLevels;
        for (std::uint32_t i = 0; i < destDesc.MipLevels; i++) {
            cmdList.CopySubresource(dest, subResourceIndex + i, src, i);
        }
        
        cmdList.TransitionResource(src, D3D12_RESOURCE_STATE_GENERIC_READ);

        g_RenderContext.ExecuteCommandList(&cmdList, true);
    }


    

    void CommandList::BindDescriptorHeaps()
    {
        // 绑定描述符堆
        UINT size = 0;
        std::array<ID3D12DescriptorHeap*, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> heaps;
        for (auto& heap : m_CurrDescriptorHeaps) {
            if (heap != nullptr) {
                heaps[size++] = heap;
            }
        }
        m_CmdList->SetDescriptorHeaps(size, heaps.data());
    }
}
