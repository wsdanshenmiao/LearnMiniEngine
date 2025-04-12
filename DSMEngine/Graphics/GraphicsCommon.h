#pragma once
#ifndef __GRAPHICSCOMMON_H__
#define __GRAPHICSCOMMON_H__

#include <d3d12.h>
#include <memory>

namespace DSM {
    inline bool IsDirectXRaytracingSupported(ID3D12Device* device)
    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS5 featureSupport{};
        if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &featureSupport, sizeof(featureSupport))))
            return false;

        return featureSupport.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED;
    }

    enum class DSMSubresourceType : std::uint8_t
    {
        CBV, SRV, UAV, RTV, DSV, Invalid
    };

    enum class DSMResourceUsage : std::uint8_t
    {
        Default, Upload, Readback
    };

    enum class DSMBindFlag : std::uint32_t
    {
        None = 0,
        ConstantBuffer = 1 << 0,
        ShaderResource = 1 << 1,
        RenderTarget = 1 << 2,
        DepthStencil = 1 << 3,
        UnorderedAccess = 1 << 4,
    };

    enum class DSMBufferFlag : std::uint32_t
    {
        None,
        IndirectArgsBuffer = 1 << 0,
        RowBuffer = 1 << 1,
        StructuredBuffer = 1 << 2,
        ConstantBuffer = 1 << 3,
        VertexBuffer = 1 << 4,
        IndexBuffer = 1 << 5,
        AccelStruct = 1 << 6
    };
}

#endif
