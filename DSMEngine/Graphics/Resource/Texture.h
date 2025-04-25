#pragma once
#ifndef __TEXTURE_H__
#define __TEXTURE_H__

#include <span>
#include "GpuResource.h"

namespace DSM {
    
    struct TextureDesc
    {
        D3D12_RESOURCE_DIMENSION m_Dimension;
        std::uint64_t m_Width;
        std::uint32_t m_Height;
        std::uint16_t m_DepthOrArraySize;
        std::uint16_t m_MipLevels;
        DXGI_FORMAT m_Format;
        DXGI_SAMPLE_DESC m_SampleDesc;
        D3D12_RESOURCE_FLAGS m_Flags;
    };
    
    class Texture : public GpuResource
    {
    public:
        Texture() = default;
        Texture(const std::wstring& name, const TextureDesc& texDesc, std::span<D3D12_SUBRESOURCE_DATA> subResources = {})
        {
            Create(name, texDesc, subResources);
        }
        ~Texture() = default;
        DSM_NONCOPYABLE(Texture);

        void Create(const std::wstring& name, const TextureDesc& texDesc, std::span<D3D12_SUBRESOURCE_DATA> subResources = {});
        void Create(const std::wstring& name, ID3D12Resource* resource, bool isCubeMap = false);
        void Create(const std::wstring& name, const TextureDesc& texDesc, const D3D12_CLEAR_VALUE& clearValue);
        
        const TextureDesc& GetDesc() const { return m_Desc; }
        const D3D12_RESOURCE_DIMENSION GetDimension() const { return m_Desc.m_Dimension; }
        const std::uint64_t GetWidth() const { return m_Desc.m_Width; }
        const std::uint32_t GetHeight() const { return m_Desc.m_Height; }
        const std::uint16_t GetDepthOrArraySize() const { return m_Desc.m_DepthOrArraySize; }
        const DXGI_FORMAT GetFormat() const { return m_Desc.m_Format; }


        static void CreateTextureFromFile(
            Texture& texture,
            const std::string& texName,
            const std::string& fileName,
            bool forceSRGB = false);
        
    private:
        TextureDesc m_Desc{};
        bool m_IsCubeMap = false;
    };
}

#endif
