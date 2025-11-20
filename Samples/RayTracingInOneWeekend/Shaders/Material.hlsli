#ifndef __MATERIAL_HLSLI__
#define __MATERIAL_HLSLI__

#include "RayTracingHLSLCompat.h"
#include "Random.hlsli"
#include "PDF.hlsli"

struct ScatterRecord
{
    Ray scatterRay;
    float3 attenuation;
    float3 emission;
    bool skipPDF;
    PDFType pdfType;
};

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

RayTracing::MaterialType::DiffuseLightMatData GetDiffuseLightMaterialData(uint matDataOffset)
{
    RayTracing::MaterialType::DiffuseLightMatData matData;
    uint3 data = gMaterialBuffer.Load3(matDataOffset);
    matData.emitColor = asfloat(data);
    return matData;
}

bool ScatterLambertian(
    Surface surface, 
    uint matDataOffset, 
    out ScatterRecord scatterRecord,
    inout PCGState rng)
{
    RayTracing::MaterialType::LambertianMatData matData = GetLambertianMaterialData(matDataOffset);

    scatterRecord.emission = float3(0,0,0);
    scatterRecord.attenuation = surface.color * matData.albedo;
    scatterRecord.skipPDF = false;
    scatterRecord.pdfType = PDFType::CosinePDF;
    return true; 
}

bool ScatterMetal(
    Surface surface, 
    uint matDataOffset,  
    out ScatterRecord scatterRecord,
    inout PCGState rng)
{
    scatterRecord.emission = float3(0,0,0);
    
    RayTracing::MaterialType::MetalMatData matData = GetMetalMaterialData(matDataOffset);
    // 反射光线
    float3 reflected = reflect(WorldRayDirection(), surface.normal);
    float3 scatterDir = reflected + matData.fuzz * RandomUnitVector(rng);
    scatterRecord.attenuation = surface.color * matData.albedo;
    scatterRecord.scatterRay.origin = surface.position;
    scatterRecord.scatterRay.direction = scatterDir;
    // 不需要进行 PDF 采样
    scatterRecord.skipPDF = true;
    return (dot(scatterDir, surface.normal) > 0);
}

bool ScatterDielectric(
    Surface surface, 
    uint matDataOffset, 
    out ScatterRecord scatterRecord,
    inout PCGState rng)
{
    scatterRecord.emission = float3(0,0,0);

    RayTracing::MaterialType::DielectricMatData matData = GetDielectricMaterialData(matDataOffset);
    scatterRecord.attenuation = surface.color;

    // 根据表面方向决定折射率
    float refractionRatio = surface.frontFace ? (1.0f / matData.refractiveIndex) : matData.refractiveIndex;

    float3 unitDirection = WorldRayDirection();
    float cosTheta   = min(dot(-unitDirection, surface.normal), 1.0f);
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
    if(cannotRefract || condition > RandomFloat(rng)){
        scatterRecord.scatterRay.direction = reflect(unitDirection, surface.normal);
    } else {
        scatterRecord.scatterRay.direction = refract(unitDirection, surface.normal, refractionRatio);
    }
    scatterRecord.scatterRay.origin = surface.position;
    scatterRecord.skipPDF = true;
    return true;
}

bool ScatterDiffuseLight(
    Surface surface, 
    uint matDataOffset,
    out ScatterRecord scatterRecord,
    inout PCGState rng)
{
    // 发光材质不散射光线
    scatterRecord.attenuation = float3(0,0,0);
    scatterRecord.scatterRay = (Ray)0;
    if(surface.frontFace) { // 单面光源
        RayTracing::MaterialType::DiffuseLightMatData matData = GetDiffuseLightMaterialData(matDataOffset);
        scatterRecord.emission = matData.emitColor * surface.color;
    }
    return false;
}

bool GetMaterialScatter(
    MaterialConstants materialCB, 
    Surface surface,
    out ScatterRecord scatterRecord,
    inout PCGState rng)
{
    switch(materialCB.type) {
    case RayTracing::MaterialType::Lambertian: {
        return ScatterLambertian(surface, materialCB.matDataOffset, scatterRecord, rng);
    }
    case RayTracing::MaterialType::Metal: {
        return ScatterMetal(surface, materialCB.matDataOffset, scatterRecord, rng);
    }
    case RayTracing::MaterialType::Dielectric: {
        return ScatterDielectric(surface, materialCB.matDataOffset, scatterRecord, rng);
    }
    case RayTracing::MaterialType::DiffuseLight: {
        return ScatterDiffuseLight(surface, materialCB.matDataOffset, scatterRecord, rng);
    }
    default:
        return false;
    }
    return false;
}

#endif // __MATERIAL_HLSLI__