#pragma once
#ifndef __MATERIAL_H__
#define __MATERIAL_H__

#include <array>


namespace DSM {
    enum MaterialTex
    {
        kBaseColor, kDiffuseRoughness, kMetalness, kOcclusion, kEmissive, kNormal, kNumTextures
    };

    struct Material
    {
        float m_BaseColor[4] = {1,1,1,1};
        float m_EmissiveColor[3] = {0,0,0};
        float m_NormalTexScale = 1;
        float m_MetallicFactor = 1;
        float m_RoughnessFactor = 1;
		std::array<std::uint32_t, kNumTextures> m_TextureIndices;
    };
}

#endif
