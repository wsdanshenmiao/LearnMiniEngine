#pragma once
#ifndef __TEXTURE_H__
#define __TEXTURE_H__

#include <span>
#include "GpuResource.h"

namespace DSM {
    
    struct TextureDesc
    {
        D3D12_RESOURCE_DIMENSION m_Dimension = D3D12_RESOURCE_DIMENSION_UNKNOWN;
        std::uint64_t m_Width = 1;
        std::uint32_t m_Height = 1;
        std::uint16_t m_DepthOrArraySize = 1;
        std::uint16_t m_MipLevels = 1;
        DXGI_FORMAT m_Format = DXGI_FORMAT_UNKNOWN;
        DXGI_SAMPLE_DESC m_SampleDesc = { 1, 0 };
        D3D12_RESOURCE_FLAGS m_Flags = D3D12_RESOURCE_FLAG_NONE;
    };
    
    class Texture : public GpuResource
    {
    public:
        Texture() = default;
        Texture(const std::wstring& name, const TextureDesc& texDesc, std::span<D3D12_SUBRESOURCE_DATA> subResources = {})
        {
            Create(name, texDesc, subResources);
        }
        virtual ~Texture() = default;
        DSM_NONCOPYABLE(Texture);

        void Create(const std::wstring& name,
            const TextureDesc& texDesc,
            std::span<D3D12_SUBRESOURCE_DATA> subResources = {},
            bool isCubeMap = false);
        void Create(const std::wstring& name, ID3D12Resource* resource, bool isCubeMap = false);
        void Create(const std::wstring& name,
            const TextureDesc& texDesc,
            const D3D12_CLEAR_VALUE& clearValue,
            bool isCubeMap = false);
        
        const TextureDesc& GetDesc() const { return m_Desc; }
        D3D12_RESOURCE_DIMENSION GetDimension() const { return m_Desc.m_Dimension; }
        std::uint64_t GetWidth() const { return m_Desc.m_Width; }
        std::uint32_t GetHeight() const { return m_Desc.m_Height; }
        std::uint16_t GetDepthOrArraySize() const { return m_Desc.m_DepthOrArraySize; }
        const DXGI_FORMAT& GetFormat() const { return m_Desc.m_Format; }

        void CreateShaderResourceView(D3D12_CPU_DESCRIPTOR_HANDLE handle);
        void CreateDepthStencilView(D3D12_CPU_DESCRIPTOR_HANDLE handle, D3D12_DSV_FLAGS flags = D3D12_DSV_FLAG_NONE);
        void CreateRenderTargetView(D3D12_CPU_DESCRIPTOR_HANDLE handle);


        static bool CreateTextureFromFile(
            Texture& texture,
            const std::string& texName,
            const std::string& filename,
            bool forceSRGB = false);

    protected:
        DXGI_FORMAT GetDSVFormat(DXGI_FORMAT defaultFormat) const noexcept;
		DXGI_FORMAT GetSRVFormat(DXGI_FORMAT defaultFormat) const noexcept;
        
    protected:
        TextureDesc m_Desc{};
        bool m_IsCubeMap = false;
    };
}

#endif
