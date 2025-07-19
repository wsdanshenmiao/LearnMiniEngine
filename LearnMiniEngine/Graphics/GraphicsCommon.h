#pragma once
#ifndef __GRAPHICSCOMMON_H__
#define __GRAPHICSCOMMON_H__

#include <d3d12.h>

namespace DSM {
    class CommandSignature;
}

namespace DSM::Graphics {

    enum eDefaultTexture
    {
        kMagenta2D,
        kBlackOpaque2D,
        kWhiteOpaque2D,
        kBlackTransparent2D,
        kWhiteTransparent2D,
        kDefaultNormalTex,
        kBlackCubeTex,

        kNumDefaultTexture
    };

    void InitializeCommon();
    void DestroyCommon();
    
    bool IsDirectXRaytracingSupported(ID3D12Device* device);

    D3D12_CPU_DESCRIPTOR_HANDLE GetDefaultTexture(eDefaultTexture texID);

    extern D3D12_SAMPLER_DESC SamplerLinearWrap;
    extern D3D12_SAMPLER_DESC SamplerLinearBorder;
    extern D3D12_SAMPLER_DESC SamplerLinearClamp;
    extern D3D12_SAMPLER_DESC SamplerPointClamp;
    extern D3D12_SAMPLER_DESC SamplerPointBorder;
    extern D3D12_SAMPLER_DESC SamplerAnisoWrap;
    extern D3D12_SAMPLER_DESC SamplerShadow;
    // 用于采样3D纹理
    extern D3D12_SAMPLER_DESC SamplerVolumeWrap;

    extern D3D12_RASTERIZER_DESC DefaultRasterizer;
    extern D3D12_RASTERIZER_DESC DefaultMsaaRasterizer;
    // 逆时针
    extern D3D12_RASTERIZER_DESC DefaultCCWRasterizer;
    extern D3D12_RASTERIZER_DESC DefaultCCWMsaaRasterizer;
    // 无背面剔除
    extern D3D12_RASTERIZER_DESC BothSidedRasterizer;
    extern D3D12_RASTERIZER_DESC BothSidedMsaaRasterizer;
    extern D3D12_RASTERIZER_DESC ShadowRasterizer;
    extern D3D12_RASTERIZER_DESC ShadowCCWRasterizer;
    extern D3D12_RASTERIZER_DESC ShadowBothSidedRasterizer;

    extern D3D12_BLEND_DESC NoColorWriteBlend;
    extern D3D12_BLEND_DESC DisableBlend;
    // 预乘混合
    extern D3D12_BLEND_DESC PreMultipliedBlend;
    // 加法混合
    extern D3D12_BLEND_DESC AdditiveBlend;
    extern D3D12_BLEND_DESC DefaultAlphaBlend;
    extern D3D12_BLEND_DESC AdditiveAlphaBlend;

    extern D3D12_DEPTH_STENCIL_DESC DisableDepthStencil;
    extern D3D12_DEPTH_STENCIL_DESC ReadWriteDepthStencil;
    extern D3D12_DEPTH_STENCIL_DESC ReadOnlyDepthStencil;
    extern D3D12_DEPTH_STENCIL_DESC ReadOnlyReversedDepthStencil;
    extern D3D12_DEPTH_STENCIL_DESC TestEqualDepthStencil;

    extern CommandSignature DrawCommandSignature;
    extern CommandSignature DrawIndexedCommandSignature;
    extern CommandSignature DispatchCommandSignature;
    
}

#endif
