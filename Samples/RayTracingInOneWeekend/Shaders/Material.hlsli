#ifndef __MATERIAL_HLSLI__
#define __MATERIAL_HLSLI__

#include "RayTracingHLSLCompat.h"
#include "Random.hlsli"

// 材质数据缓冲区
ByteAddressBuffer gMaterialBuffer : register(t0, space1);

RayTracing::MaterialType::LambertianMatData GetLambertianMaterialData(uint matDataOffset)
{
    RayTracing::MaterialType::LambertianMatData matData;
    uint3 data = gMaterialBuffer.Load3(matDataOffset);
    matData.albedo = asfloat(data);
    return matData;
}

RayTracing::MaterialType::MetalMatData GetMetalMaterialData(uint matDataOffset)
{
    RayTracing::MaterialType::MetalMatData matData;
    uint4 data = gMaterialBuffer.Load4(matDataOffset);
    matData.albedo = asfloat(data.xyz);
    matData.fuzz = asfloat(data.w);
    return matData;
}

RayTracing::MaterialType::DielectricMatData GetDielectricMaterialData(uint matDataOffset)
{
    RayTracing::MaterialType::DielectricMatData matData;
    uint data = gMaterialBuffer.Load(matDataOffset);
    matData.refractiveIndex = asfloat(data);
    return matData;
}

bool ScatterLambertian(Surface surface, uint matDataOffset, out float3 attenuation, out float3 scatterDir)
{
    RayTracing::MaterialType::LambertianMatData matData = GetLambertianMaterialData(matDataOffset);
    scatterDir = surface.normal + RandomUnitVector(surface.seed);
    if(NearZero(scatterDir)){
        scatterDir = surface.normal;
    }

    attenuation = surface.color * matData.albedo;
    return true; 
}

bool ScatterMetal(Surface surface, uint matDataOffset, out float3 attenuation, out float3 scatterDir)
{
    RayTracing::MaterialType::MetalMatData matData = GetMetalMaterialData(matDataOffset);
    // 反射光线
    float3 reflected = reflect(WorldRayDirection(), surface.normal);
    scatterDir = reflected + matData.fuzz * RandomUnitVector(surface.seed);
    attenuation = surface.color * matData.albedo;
    return (dot(scatterDir, surface.normal) > 0);
}

bool ScatterDielectric(Surface surface, uint matDataOffset, out float3 attenuation, out float3 scatterDir)
{
    RayTracing::MaterialType::DielectricMatData matData = GetDielectricMaterialData(matDataOffset);
    attenuation = float3(1.0f, 1.0f, 1.0f);

    // 根据表面方向决定折射率
    float refractionRatio = surface.frontFace ? (1.0f / matData.refractiveIndex) : matData.refractiveIndex;

    float3 unitDirection = WorldRayDirection();
    float cosTheta = min(dot(-unitDirection, surface.normal), 1.0f);
    float sinTheta = sqrt(1.0f - cosTheta * cosTheta);

    // 发生全反射
    bool cannotRefract = refractionRatio * sinTheta > 1.0f;
    
    // 计算基于菲涅尔反射的反射率
    // 反射系数
    float r0 = (1.0 - refractionRatio) / (1.0 + refractionRatio);   // 近似计算
    // 反射率
    r0 = r0 * r0;
    float condition = r0 + (1.0 - r0) * pow(1.0 - cosTheta, 5.0);

    // 根据蒙特卡洛方法决定反射或折射
    if(cannotRefract || condition > RandomFloat(surface.seed)){
        scatterDir = reflect(unitDirection, surface.normal);
    } else {
        scatterDir = refract(unitDirection, surface.normal, refractionRatio);
    }
    return true;
}

bool GetMaterialScatter(
    MaterialConstants materialCB, 
    Surface surface,
    out float3 attenuation, 
    out float3 scatterDir)
{
    switch(materialCB.type) {
    case RayTracing::MaterialType::Lambertian: {
        return ScatterLambertian(surface, materialCB.matDataOffset, attenuation, scatterDir);
    }
    case RayTracing::MaterialType::Metal: {
        return ScatterMetal(surface, materialCB.matDataOffset, attenuation, scatterDir);
    }
    case RayTracing::MaterialType::Dielectric: {
        return ScatterDielectric(surface, materialCB.matDataOffset, attenuation, scatterDir);
    }
    default:
        return false;
    }
    return false;
}

#endif // __MATERIAL_HLSLI__