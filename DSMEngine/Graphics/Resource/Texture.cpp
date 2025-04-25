#define STB_IMAGE_IMPLEMENTATION

#include "Texture.h"
#include "../RenderContext.h"
#include "../CommandList/CommandList.h"
#include "../../Utilities/DDSTextureLoader12.h"
#include "../../Utilities/FormatUtil.h"
#include "../../Utilities/stb_image.h"

using namespace DirectX;

namespace DSM{
    
    void Texture::Create(const std::wstring& name,const TextureDesc& texDesc,std::span<D3D12_SUBRESOURCE_DATA> subResources)
    {
        m_Desc = texDesc;
        
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
        gpuResourceDesc.m_State = D3D12_RESOURCE_STATE_COPY_DEST;
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

    void Texture::Create(const std::wstring& name, const TextureDesc& texDesc, const D3D12_CLEAR_VALUE& clearValue)
    {
        m_Desc = texDesc;
        
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
        gpuResourceDesc.m_State = D3D12_RESOURCE_STATE_COMMON;
        gpuResourceDesc.m_Desc = resourceDesc;
        
        GpuResource::Create(name, gpuResourceDesc, clearValue);
    }

    void Texture::CreateTextureFromFile(
        Texture& texture,
        const std::string& texName,
        const std::string& fileName,
        bool forceSRGB)
    {
		stbi_uc* imgData = nullptr;
        D3D12_RESOURCE_DESC texDesc{};
        std::unique_ptr<std::uint8_t[]> ddsData{};
        std::vector<D3D12_SUBRESOURCE_DATA> subResources{};
        DDS_LOADER_FLAGS loadFlags = forceSRGB ? DDS_LOADER_FORCE_SRGB : DDS_LOADER_DEFAULT;
        std::wstring wTexName = Utility::UTF8ToWString(texName);
        
        if (FAILED(LoadDDSTextureFromFileEx(
            g_RenderContext.GetDevice(),
            wTexName.c_str(),
            0,
            D3D12_RESOURCE_FLAG_NONE,
            loadFlags,
            texDesc,
            ddsData,
            subResources,
            nullptr,
            &texture.m_IsCubeMap))) {
            int width, height, components;
            imgData = stbi_load(texName.c_str(), &width, &height, &components, 4);

            bool isHDR = stbi_is_hdr(texName.c_str());
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
        texture.Create(wTexName, textureDesc, subResources);

        if (imgData != nullptr) {
            stbi_image_free(imgData);
        }
    }
}
