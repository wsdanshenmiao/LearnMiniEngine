#pragma once
#ifndef __TEXTURE_H__
#define __TEXTURE_H__
#include "GpuResource.h"

namespace DSM {
    struct TextureDesc
    {
        D3D12_RESOURCE_DESC m_ResourceDescs;
        D3D12_RESOURCE_STATES m_ResourceState{};
    };

    
    class Texture : public GpuResource
    {
    public:
        Texture();
        ~Texture();
        DSM_NONCOPYABLE(Texture);

    private:
    };
}

#endif
