#define STB_IMAGE_IMPLEMENTATION

#include "Texture.h"
#include "../RenderContext.h"
#include "../CommandList/CommandList.h"
#include "../../Utilities/DDSTextureLoader12.h"
#include "../../Utilities/FormatUtil.h"
#include "../../Utilities/stb_image.h"

using namespace DirectX;

namespace DSM{
    
    void Texture::Create(const std::wstring& name,
        const TextureDesc& texDesc,
        std::span<D3D12_SUBRESOURCE_DATA> subResources,
        D3D12_RESOURCE_STATES initialState,
        const D3D12_CLEAR_VALUE* clearValue,
        bool isCubeMap)
    {
        m_Desc = texDesc;
        m_IsCubeMap = isCubeMap;
        
        D3D12_RESOURCE_DESC resourceDesc{};
        resourceDesc.Dimension = texDesc.m_Dimension;
        resourceDesc.Flags = texDesc.m_Flags;
        resourceDesc.Format = texDesc.m_Format;
        resourceDesc.Width = texDesc.m_Width;
        resourceDesc.Height = texDesc.m_Height;
        resourceDesc.DepthOrArraySize = texDesc.m_DepthOrArraySize;
        resourceDesc.MipLevels = texDesc.m_MipLevels;
        resourceDesc.SampleDesc = texDesc.m_SampleDesc;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        GpuResourceDesc gpuResourceDesc{};
        gpuResourceDesc.m_HeapType = D3D12_HEAP_TYPE_DEFAULT;
        gpuResourceDesc.m_HeapFlags = D3D12_HEAP_FLAG_NONE;
        gpuResourceDesc.m_State = initialState;
        gpuResourceDesc.m_Desc = resourceDesc;
        
        GpuResource::Create(name, gpuResourceDesc);

        if (subResources.size() > 0) {
            CommandList::InitTexture(*this, subResources);
        }
    }

    void Texture::Create(const std::wstring& name, ID3D12Resource* resource, bool isCubeMap)
    {
        GpuResource::Create(name, resource);

        const D3D12_RESOURCE_DESC& resourceDesc = resource->GetDesc();
        m_IsCubeMap = isCubeMap;
        m_Desc.m_Dimension = resourceDesc.Dimension;
        m_Desc.m_Width = resourceDesc.Width;
        m_Desc.m_Height = resourceDesc.Height;
        m_Desc.m_DepthOrArraySize = resourceDesc.DepthOrArraySize;
        m_Desc.m_MipLevels = resourceDesc.MipLevels;
        m_Desc.m_SampleDesc = resourceDesc.SampleDesc;
        m_Desc.m_Flags = resourceDesc.Flags;
        m_Desc.m_Format = resourceDesc.Format;
    }


    void Texture::CreateShaderResourceView(D3D12_CPU_DESCRIPTOR_HANDLE handle)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = GetSRVFormat(m_Desc.m_Format);
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        switch (m_Desc.m_Dimension) {
            case D3D12_RESOURCE_DIMENSION_TEXTURE1D: {
                if (m_Desc.m_DepthOrArraySize > 1) {
                    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
                    srvDesc.Texture1DArray.ArraySize = m_Desc.m_DepthOrArraySize;
                    srvDesc.Texture1DArray.MipLevels = m_Desc.m_MipLevels;
                }
                else {
                    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
                    srvDesc.Texture1D.MipLevels = m_Desc.m_MipLevels;
                }
                break;
            }
            case D3D12_RESOURCE_DIMENSION_TEXTURE2D: {
                if (m_IsCubeMap) {
                    if (m_Desc.m_DepthOrArraySize > 6) {
                        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
                        srvDesc.TextureCubeArray.MipLevels = m_Desc.m_MipLevels;
                        srvDesc.TextureCubeArray.NumCubes = m_Desc.m_DepthOrArraySize / 6;
                    }
                    else {
                        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
                        srvDesc.TextureCube.MipLevels = m_Desc.m_MipLevels;
                    }
                }
                else if (m_Desc.m_DepthOrArraySize > 1){
                    if (m_Desc.m_SampleDesc.Count > 1) {
                        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
                    }
                    else {
                        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
                        srvDesc.Texture2DArray.MipLevels = m_Desc.m_MipLevels;
                        srvDesc.Texture2DArray.ArraySize = m_Desc.m_DepthOrArraySize;
                    }
                }
                else {
                    if (m_Desc.m_SampleDesc.Count > 1) {
                        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
                    }
                    else{
                        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                        srvDesc.Texture2D.MipLevels = m_Desc.m_MipLevels;
                    }
                }
                break;
            }
            case D3D12_RESOURCE_DIMENSION_TEXTURE3D: {
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
                srvDesc.Texture3D.MipLevels = m_Desc.m_MipLevels;
            }
        }
        g_RenderContext.GetDevice()->CreateShaderResourceView(GetResource(), &srvDesc, handle);
    }

    void Texture::CreateDepthStencilView(D3D12_CPU_DESCRIPTOR_HANDLE handle, D3D12_DSV_FLAGS flags)
    {
        ASSERT(m_IsCubeMap == false && m_Desc.m_Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE3D);
        
        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Format = GetDSVFormat(m_Desc.m_Format);
        dsvDesc.Flags = flags;
        switch (m_Desc.m_Dimension) {
            case D3D12_RESOURCE_DIMENSION_TEXTURE1D: {
                if (m_Desc.m_DepthOrArraySize > 1) {
                    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1DARRAY;
                    dsvDesc.Texture1DArray.ArraySize = m_Desc.m_DepthOrArraySize;
                }
                else {
                    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1D;
                }
                break;
            }
            case D3D12_RESOURCE_DIMENSION_TEXTURE2D: {
                if (m_Desc.m_DepthOrArraySize > 1){
                    if (m_Desc.m_SampleDesc.Count > 1) {
                        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
                        dsvDesc.Texture2DMSArray.ArraySize = m_Desc.m_DepthOrArraySize;
                    }
                    else {
                        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
                        dsvDesc.Texture2DArray.ArraySize = m_Desc.m_DepthOrArraySize;
                    }
                }
                else {
                    if (m_Desc.m_SampleDesc.Count > 1) {
                        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
                    }
                    else{
                        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
                    }
                }
                break;
            }
        }
        g_RenderContext.GetDevice()->CreateDepthStencilView(GetResource(), &dsvDesc, handle);
    }

    void Texture::CreateRenderTargetView(D3D12_CPU_DESCRIPTOR_HANDLE handle)
    {
        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.Format = m_Desc.m_Format;
        switch (m_Desc.m_Dimension) {
            case D3D12_RESOURCE_DIMENSION_TEXTURE1D: {
                if (m_Desc.m_DepthOrArraySize > 1) {
                    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
                    rtvDesc.Texture1DArray.ArraySize = m_Desc.m_DepthOrArraySize;
                }
                else {
                    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1D;
                }
                break;
            }
            case D3D12_RESOURCE_DIMENSION_TEXTURE2D: {
                if (m_Desc.m_DepthOrArraySize > 1){
                    if (m_Desc.m_SampleDesc.Count > 1) {
                        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
                        rtvDesc.Texture2DMSArray.ArraySize = m_Desc.m_DepthOrArraySize;
                    }
                    else {
                        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
                        rtvDesc.Texture2DArray.ArraySize = m_Desc.m_DepthOrArraySize;
                    }
                }
                else {
                    if (m_Desc.m_SampleDesc.Count > 1) {
                        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
                    }
                    else{
                        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
                    }
                }
                break;
            }
            case D3D12_RESOURCE_DIMENSION_TEXTURE3D: {
                rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
                rtvDesc.Texture3D.WSize = m_Desc.m_DepthOrArraySize;
            }
        }
        g_RenderContext.GetDevice()->CreateRenderTargetView(GetResource(), &rtvDesc, handle);
    }

    void Texture::CreateUnorderedAccessView(D3D12_CPU_DESCRIPTOR_HANDLE handle)
    {
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.Format = m_Desc.m_Format;
        switch (m_Desc.m_Dimension) {
            case D3D12_RESOURCE_DIMENSION_TEXTURE1D: {
                if (m_Desc.m_DepthOrArraySize > 1) {
                    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
                    uavDesc.Texture1DArray.ArraySize = m_Desc.m_DepthOrArraySize;
                }
                else {
                    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
                }
                break;
            }
            case D3D12_RESOURCE_DIMENSION_TEXTURE2D: {
                if (m_Desc.m_DepthOrArraySize > 1) {
                    if (m_Desc.m_SampleDesc.Count > 1) {
                        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DMSARRAY;
                        uavDesc.Texture2DMSArray.ArraySize = m_Desc.m_DepthOrArraySize;
                    }
                    else {
                        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
                        uavDesc.Texture2DArray.ArraySize = m_Desc.m_DepthOrArraySize;
                    }
                }
                else {
                    if (m_Desc.m_SampleDesc.Count > 1) {
                        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DMS;
                    }
                    else {
                        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
                    }
                }
                break;
            }
            case D3D12_RESOURCE_DIMENSION_TEXTURE3D: {
                uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
                uavDesc.Texture3D.WSize = m_Desc.m_DepthOrArraySize;
            }
        }
        g_RenderContext.GetDevice()->CreateUnorderedAccessView(GetResource(), nullptr, &uavDesc, handle);
    }

    bool Texture::CreateTextureFromFile(
        Texture& texture,
        const std::string& texName,
        const std::string& filename,
        bool forceSRGB)
    {
		stbi_uc* imgData = nullptr;
        D3D12_RESOURCE_DESC texDesc{};
        std::unique_ptr<std::uint8_t[]> ddsData{};
        std::vector<D3D12_SUBRESOURCE_DATA> subResources{};
        DDS_LOADER_FLAGS loadFlags = forceSRGB ? DDS_LOADER_FORCE_SRGB : DDS_LOADER_DEFAULT;
        std::wstring wFilename = Utility::UTF8ToWString(filename);
        
        if (FAILED(LoadDDSTextureFromFileEx(
            g_RenderContext.GetDevice(),
            wFilename.c_str(),
            0,
            D3D12_RESOURCE_FLAG_NONE,
            loadFlags,
            texDesc,
            ddsData,
            subResources,
            nullptr,
            &texture.m_IsCubeMap))) {
            int width, height, components;
            imgData = stbi_load(filename.c_str(), &width, &height, &components, 4);
            if (imgData == nullptr) return false;
            
            bool isHDR = stbi_is_hdr(filename.c_str());
            texDesc.Format = isHDR ? DXGI_FORMAT_R32G32B32A32_FLOAT : DXGI_FORMAT_R8G8B8A8_UNORM;
            texDesc.Width = static_cast<std::uint64_t>(width);
            texDesc.Height = static_cast<std::uint32_t>(height);
            texDesc.DepthOrArraySize = 1;
            texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            texDesc.MipLevels = 1;
            texDesc.SampleDesc = {1,0};
            texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
            texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

            D3D12_SUBRESOURCE_DATA subResourceData{};
            subResourceData.pData = imgData;
            subResourceData.RowPitch = Utility::GetRowPitch(texDesc.Format, texDesc.Width);
            subResourceData.SlicePitch = Utility::GetSlicePitch(texDesc.Format, texDesc.Width, texDesc.Height);
            subResources.clear();
            subResources.emplace_back(std::move(subResourceData));
        }

        TextureDesc textureDesc{};
        textureDesc.m_Dimension = texDesc.Dimension;
        textureDesc.m_MipLevels = texDesc.MipLevels;
        textureDesc.m_SampleDesc = texDesc.SampleDesc;
        textureDesc.m_Format = texDesc.Format;
        textureDesc.m_Flags = texDesc.Flags;
        textureDesc.m_Height = texDesc.Height;
        textureDesc.m_Width = texDesc.Width;
        textureDesc.m_DepthOrArraySize = texDesc.DepthOrArraySize;
        texture.Create(wFilename, textureDesc, subResources);

        std::wstring wTexName = Utility::UTF8ToWString(texName);
        texture->SetName(wTexName.c_str());

        if (imgData != nullptr) {
            stbi_image_free(imgData);
        }

        return true;
    }

    DXGI_FORMAT Texture::GetDSVFormat(DXGI_FORMAT defaultFormat) const noexcept
    {
        switch (defaultFormat)
        {
            // 32-bit Z w/ Stencil
        case DXGI_FORMAT_R32G8X24_TYPELESS:
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
        case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
            return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;

            // No Stencil
        case DXGI_FORMAT_R32_TYPELESS:
        case DXGI_FORMAT_D32_FLOAT:
        case DXGI_FORMAT_R32_FLOAT:
            return DXGI_FORMAT_D32_FLOAT;

            // 24-bit Z
        case DXGI_FORMAT_R24G8_TYPELESS:
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
        case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
        case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
            return DXGI_FORMAT_D24_UNORM_S8_UINT;

            // 16-bit Z w/o Stencil
        case DXGI_FORMAT_R16_TYPELESS:
        case DXGI_FORMAT_D16_UNORM:
        case DXGI_FORMAT_R16_UNORM:
            return DXGI_FORMAT_D16_UNORM;

        default:
            return defaultFormat;
        }
        
    }

    DXGI_FORMAT Texture::GetSRVFormat(DXGI_FORMAT defaultFormat) const noexcept
    {
        switch (defaultFormat)
        {
            // 32-bit Z w/ Stencil
        case DXGI_FORMAT_R32G8X24_TYPELESS:
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
        case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
            return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;

            // No Stencil
        case DXGI_FORMAT_R32_TYPELESS:
        case DXGI_FORMAT_D32_FLOAT:
        case DXGI_FORMAT_R32_FLOAT:
            return DXGI_FORMAT_R32_FLOAT;

            // 24-bit Z
        case DXGI_FORMAT_R24G8_TYPELESS:
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
        case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
        case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
            return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;

            // 16-bit Z w/o Stencil
        case DXGI_FORMAT_R16_TYPELESS:
        case DXGI_FORMAT_D16_UNORM:
        case DXGI_FORMAT_R16_UNORM:
            return DXGI_FORMAT_R16_UNORM;

        default:
            return defaultFormat;
        }
    }


    
}
