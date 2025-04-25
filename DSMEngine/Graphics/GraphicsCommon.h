#pragma once
#ifndef __GRAPHICSCOMMON_H__
#define __GRAPHICSCOMMON_H__

#include <d3d12.h>
#include <memory>

namespace DSM::Graphics {
    bool IsDirectXRaytracingSupported(ID3D12Device* device);
    
    D3D12_BLEND_DESC GetDefaultBlendState();
    D3D12_RASTERIZER_DESC GetDefaultRasterizerState();
    D3D12_DEPTH_STENCIL_DESC GetDefaultDepthStencilState();

}

#endif
