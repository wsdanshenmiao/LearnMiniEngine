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

}

#endif
