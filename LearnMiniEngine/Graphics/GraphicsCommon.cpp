#include "GraphicsCommon.h"
#include "CommandSignature.h"

namespace DSM::Graphics {

    D3D12_SAMPLER_DESC SamplerLinearWrap;
    D3D12_SAMPLER_DESC SamplerLinearBorder;
    D3D12_SAMPLER_DESC SamplerLinearClamp;
    D3D12_SAMPLER_DESC SamplerPointClamp;
    D3D12_SAMPLER_DESC SamplerPointBorder;
    D3D12_SAMPLER_DESC SamplerAnisoWrap;
    D3D12_SAMPLER_DESC SamplerShadow;
    // 用于采样3D纹理
    D3D12_SAMPLER_DESC SamplerVolumeWrap;

    D3D12_RASTERIZER_DESC DefaultRasterizer;
    D3D12_RASTERIZER_DESC DefaultMsaaRasterizer;
    // 逆时针
    D3D12_RASTERIZER_DESC DefaultCCWRasterizer;
    D3D12_RASTERIZER_DESC DefaultCCWMsaaRasterizer;
    // 无背面剔除
    D3D12_RASTERIZER_DESC BothSidedRasterizer;
    D3D12_RASTERIZER_DESC BothSidedMsaaRasterizer;
    D3D12_RASTERIZER_DESC ShadowRasterizer;
    D3D12_RASTERIZER_DESC ShadowCCWRasterizer;
    D3D12_RASTERIZER_DESC ShadowBothSidedRasterizer;

    D3D12_BLEND_DESC NoColorWriteBlend;
    D3D12_BLEND_DESC DisableBlend;
    // 预乘混合
    D3D12_BLEND_DESC PreMultipliedBlend;
    // 加法混合
    D3D12_BLEND_DESC AdditiveBlend;
    D3D12_BLEND_DESC DefaultAlphaBlend;
    D3D12_BLEND_DESC AdditiveAlphaBlend;

    D3D12_DEPTH_STENCIL_DESC DisableDepthStencil;
    D3D12_DEPTH_STENCIL_DESC ReadWriteDepthStencil;
    D3D12_DEPTH_STENCIL_DESC ReadOnlyDepthStencil;
    D3D12_DEPTH_STENCIL_DESC ReadOnlyReversedDepthStencil;
    D3D12_DEPTH_STENCIL_DESC TestEqualDepthStencil;

    CommandSignature DrawCommandSignature{1};
    CommandSignature DrawIndexedCommandSignature{1};
    CommandSignature DispatchCommandSignature{1};

    std::array<Texture, kNumDefaultTexture> DefaultTextures;
    std::array<DescriptorHandle, kNumDefaultTexture> DefaultTextureHandles;
    
    bool IsDirectXRaytracingSupported(ID3D12Device* device)
    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS5 featureSupport{};
        if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &featureSupport, sizeof(featureSupport))))
            return false;

        return featureSupport.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE GetDefaultTexture(eDefaultTexture texID)
    {
        ASSERT(texID < kNumDefaultTexture);
        return DefaultTextureHandles[texID];
    }

    void InitializeCommon()
    {
        D3D12_SAMPLER_DESC samplerDesc{};
        samplerDesc.Filter = D3D12_FILTER_ANISOTROPIC;
        samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        samplerDesc.MipLODBias = 0.0f;
        samplerDesc.MaxAnisotropy = 16;
        samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
        samplerDesc.BorderColor[0] = 1.0f;
        samplerDesc.BorderColor[1] = 1.0f;
        samplerDesc.BorderColor[2] = 1.0f;
        samplerDesc.BorderColor[3] = 1.0f;
        samplerDesc.MinLOD = 0.0f;
        samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;

        SamplerLinearWrap = samplerDesc;
        SamplerLinearWrap.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;

        SamplerLinearBorder = SamplerLinearWrap;
        SamplerLinearBorder.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        SamplerLinearBorder.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        SamplerLinearBorder.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        SamplerLinearBorder.BorderColor[0] = 0.0f;
        SamplerLinearBorder.BorderColor[1] = 0.0f;
        SamplerLinearBorder.BorderColor[2] = 0.0f;
        SamplerLinearBorder.BorderColor[3] = 0.0f;
        
        SamplerLinearClamp = samplerDesc;
        SamplerLinearClamp.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        SamplerLinearClamp.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        SamplerLinearClamp.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        SamplerLinearClamp.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;

        SamplerPointClamp = samplerDesc;
        SamplerPointClamp.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
        SamplerPointClamp.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        SamplerPointClamp.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        SamplerPointClamp.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;

        SamplerPointBorder = SamplerLinearBorder;
        SamplerPointBorder.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;

        SamplerAnisoWrap = samplerDesc;
        SamplerAnisoWrap.MaxAnisotropy = 4;

        SamplerShadow = samplerDesc;
        SamplerShadow.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
        SamplerShadow.ComparisonFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
        SamplerShadow.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        SamplerShadow.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        SamplerShadow.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;

        SamplerVolumeWrap.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
        

        
        D3D12_RASTERIZER_DESC rasterizerDesc{};
        rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
        rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
        rasterizerDesc.FrontCounterClockwise = false;
        rasterizerDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
        rasterizerDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
        rasterizerDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
        rasterizerDesc.DepthClipEnable = true;
        rasterizerDesc.MultisampleEnable = false;
        rasterizerDesc.AntialiasedLineEnable = false;
        rasterizerDesc.ForcedSampleCount = 0;
        rasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

        DefaultRasterizer = rasterizerDesc;

        DefaultMsaaRasterizer = rasterizerDesc;
        DefaultMsaaRasterizer.MultisampleEnable = true;

        DefaultCCWRasterizer = rasterizerDesc;
        DefaultCCWRasterizer.ForcedSampleCount = true;

        DefaultCCWMsaaRasterizer = DefaultCCWRasterizer;
        DefaultCCWMsaaRasterizer.MultisampleEnable = true;

        BothSidedRasterizer = rasterizerDesc;
        BothSidedRasterizer.CullMode = D3D12_CULL_MODE_NONE;

        BothSidedMsaaRasterizer = BothSidedRasterizer;
        BothSidedMsaaRasterizer.MultisampleEnable = true;

        ShadowRasterizer = rasterizerDesc;
        ShadowRasterizer.SlopeScaledDepthBias = -1.5;
        ShadowRasterizer.DepthBias =-100;

        ShadowCCWRasterizer = ShadowRasterizer;
        ShadowCCWRasterizer.FrontCounterClockwise = true;

        ShadowBothSidedRasterizer = ShadowRasterizer;
        ShadowBothSidedRasterizer.CullMode = D3D12_CULL_MODE_NONE;
        
        
        D3D12_RENDER_TARGET_BLEND_DESC rtBlendDesc{};
        rtBlendDesc.BlendEnable = false;
        rtBlendDesc.LogicOpEnable = false;
        rtBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
        rtBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        rtBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
        rtBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
        rtBlendDesc.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
        rtBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
        rtBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
        rtBlendDesc.RenderTargetWriteMask = 0;
        D3D12_BLEND_DESC blendDesc{};
        blendDesc.AlphaToCoverageEnable = false;
        blendDesc.IndependentBlendEnable = false;
        blendDesc.RenderTarget[0] = rtBlendDesc;

        NoColorWriteBlend = blendDesc;

        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        DisableBlend = blendDesc;

        blendDesc.RenderTarget[0].BlendEnable = true;
        DefaultAlphaBlend = blendDesc;

        blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
        PreMultipliedBlend = blendDesc;

        blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
        AdditiveBlend = blendDesc;

        blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        AdditiveAlphaBlend = blendDesc;
        
        
        D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
        depthStencilDesc.DepthEnable = TRUE;
        depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
        depthStencilDesc.StencilEnable = FALSE;
        depthStencilDesc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
        depthStencilDesc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
        const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp ={
            D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
        depthStencilDesc.FrontFace = defaultStencilOp;
        depthStencilDesc.BackFace = defaultStencilOp;

        ReadWriteDepthStencil = depthStencilDesc;

        ReadOnlyDepthStencil = depthStencilDesc;
        ReadOnlyDepthStencil.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

        ReadOnlyReversedDepthStencil = ReadOnlyDepthStencil;
        ReadOnlyReversedDepthStencil.DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;

        TestEqualDepthStencil = ReadOnlyDepthStencil;
        TestEqualDepthStencil.DepthFunc = D3D12_COMPARISON_FUNC_EQUAL;

        DisableDepthStencil = ReadOnlyDepthStencil;
        DisableDepthStencil.DepthEnable = false;
        DisableDepthStencil.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;


        DrawCommandSignature[0].Draw();
        DrawCommandSignature.Finalize();

        DrawIndexedCommandSignature[0].DrawIndex();
        DrawIndexedCommandSignature.Finalize();

        DispatchCommandSignature[0].Dispatch();
        DispatchCommandSignature.Finalize();


        // 创建默认纹理
        uint32_t MagentaPixel = 0xFFFF00FF;
        TextureDesc texDesc{};
        texDesc.m_Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        texDesc.m_Flags = D3D12_RESOURCE_FLAG_NONE;
        texDesc.m_Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        texDesc.m_Height = 1;
        texDesc.m_Width = 1;
        texDesc.m_MipLevels = 1;
        texDesc.m_SampleDesc = { 1, 0};
        texDesc.m_DepthOrArraySize = 1;
        D3D12_SUBRESOURCE_DATA subresourceData{};
        subresourceData.pData = &MagentaPixel;
        subresourceData.RowPitch = 4;
        subresourceData.SlicePitch = subresourceData.RowPitch * 4;
        DefaultTextures[kMagenta2D].Create(L"Magenta Tex", texDesc, {&subresourceData, 1});
        uint32_t BlackOpaqueTexel = 0xFF000000;
        subresourceData.pData = &BlackOpaqueTexel;
        DefaultTextures[kBlackOpaque2D].Create(L"BlackOpaque Tex", texDesc, {&subresourceData, 1});
        uint32_t BlackTransparentTexel = 0x00000000;
        subresourceData.pData = &BlackTransparentTexel;
        DefaultTextures[kBlackTransparent2D].Create(L"BlackTransparent Tex", texDesc, {&subresourceData, 1});
        uint32_t WhiteOpaqueTexel = 0xFFFFFFFF;
        subresourceData.pData = &WhiteOpaqueTexel;
        DefaultTextures[kWhiteOpaque2D].Create(L"WhiteOpaque Tex", texDesc, {&subresourceData, 1});
        uint32_t WhiteTransparentTexel = 0x00FFFFFF;
        subresourceData.pData = &WhiteTransparentTexel;
        DefaultTextures[kWhiteTransparent2D].Create(L"WhiteTransparent Tex", texDesc, {&subresourceData, 1});
        uint32_t FlatNormalTexel = 0x00FF8080;
        subresourceData.pData = &FlatNormalTexel;
        DefaultTextures[kDefaultNormalTex].Create(L"DefaultNormal Tex", texDesc, {&subresourceData, 1});
        uint32_t BlackCubeTexels[6] = {};
        subresourceData.pData = BlackCubeTexels;
        texDesc.m_DepthOrArraySize = 6;
        DefaultTextures[kBlackCubeTex].Create(L"BlackCube Tex", texDesc, {&subresourceData, 1}, D3D12_RESOURCE_STATE_COMMON, nullptr, true);

        // 创建默认纹理的描述符
        for (int i = 0; i < kNumDefaultTexture; ++i) {
            DefaultTextureHandles[i] = g_RenderContext.AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            if (i != kBlackCubeTex) {
                g_RenderContext.GetDevice()->CreateShaderResourceView(DefaultTextures[i].GetResource(), nullptr, DefaultTextureHandles[i]);
            }
        }
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format = DefaultTextures[kBlackCubeTex].GetFormat();
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.TextureCube.MipLevels = 1;
        g_RenderContext.GetDevice()->CreateShaderResourceView(
            DefaultTextures[kBlackCubeTex].GetResource(), &srvDesc, DefaultTextureHandles[kBlackCubeTex]);
    }

    void DestroyCommon()
    {
        DrawCommandSignature.Destroy();
        DrawIndexedCommandSignature.Destroy();
        DispatchCommandSignature.Destroy();

        for (int i = 0; i < kNumDefaultTexture; ++i) {
            g_RenderContext.FreeDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, DefaultTextureHandles[i]);
        }
    }
}
