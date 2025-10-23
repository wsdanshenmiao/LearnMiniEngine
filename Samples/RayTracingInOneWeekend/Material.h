#pragma once
#ifndef __MATERIAL_H__
#define __MATERIAL_H__

#include <array>
#include "Math/Vector.h"
#include "Shaders/RayTracingHLSLCompat.h"

namespace DSM {
    enum MaterialTex
    {
        kBaseColor, kDiffuseRoughness, kMetalness, kOcclusion, kEmissive, kNormal, kNumTextures
    };

    struct Material
    {
        Math::Vector4 baseColor = {1,1,1,1};
        Math::Vector4 emissiveColor = {0,0,0,0};
        float normalTexScale = 1;
        float metallicFactor = 1;
        float roughnessFactor = 1;
    };

    struct RTMaterial 
    {
        RayTracing::MaterialType::Type materialType;
        virtual ~RTMaterial() = default;
        virtual uint32_t GetDataSize() const = 0;
        virtual void CopyData(void* dest) const = 0;
    };

    struct LambertianMaterial : public RTMaterial
    {
        RayTracing::MaterialType::LambertianMatData matData{};

        LambertianMaterial()
        {
            materialType = RayTracing::MaterialType::Lambertian;
        }

        uint32_t GetDataSize() const override
        {
            return RayTracing::MaterialType::MaterialDataSize[RayTracing::MaterialType::Lambertian];
        }
        void CopyData(void* dest) const override
        {
            assert(dest != nullptr);
            auto dataSize = RayTracing::MaterialType::MaterialDataSize[RayTracing::MaterialType::Lambertian];
            memcpy(dest, &matData.albedo, dataSize);
        }
    };

    struct MetalMaterial : public RTMaterial
    {
        RayTracing::MaterialType::MetalMatData matData{};

        MetalMaterial()
        {
            materialType = RayTracing::MaterialType::Metal;
        }

        uint32_t GetDataSize() const override
        {
            return RayTracing::MaterialType::MaterialDataSize[RayTracing::MaterialType::Metal];
        }
        void CopyData(void* dest) const override
        {
            assert(dest != nullptr);
            memcpy(dest, &matData.albedo, 3 * sizeof(float));
            memcpy(static_cast<uint8_t*>(dest) + 3 * sizeof(float), &matData.fuzz, sizeof(matData.fuzz));
        }
    };

    struct DielectricMaterial : public RTMaterial
    {
        RayTracing::MaterialType::DielectricMatData matData{};

        DielectricMaterial()
        {
            materialType = RayTracing::MaterialType::Dielectric;
        }

        uint32_t GetDataSize() const override
        {
            return RayTracing::MaterialType::MaterialDataSize[RayTracing::MaterialType::Dielectric];
        }

        void CopyData(void* dest) const override
        {
            assert(dest != nullptr);
            memcpy(dest, &matData.refractiveIndex, GetDataSize());
        }
    };


    struct DiffuseLightMaterial : public RTMaterial
    {
        RayTracing::MaterialType::DiffuseLightMatData matData{};

        DiffuseLightMaterial()
        {
            materialType = RayTracing::MaterialType::DiffuseLight;
        }

        uint32_t GetDataSize() const override
        {
            return RayTracing::MaterialType::MaterialDataSize[RayTracing::MaterialType::DiffuseLight];
        }

        void CopyData(void* dest) const override
        {
            assert(dest != nullptr);
            memcpy(dest, &matData.emitColor, GetDataSize());
        }
    };
}


#endif
