#ifndef __PDF_HLSLI__
#define __PDF_HLSLI__

#include "Random.hlsli"
#include "RayTracingHLSLCompat.h"
#include "Common.hlsli"
#include "AnalyticPrimitives.hlsli"

StructuredBuffer<RayTracing::ImportanceSampling::ImportanceSamplingObject> gImportanceSamplingObjects : register(t0, space2);
ByteAddressBuffer gImportanceSamplingObjectDataBuffer : register(t1, space2);

// Importance Sampling Data Getters
RayTracing::ImportanceSampling::SphereData GetSphereData(uint offset)
{
    RayTracing::ImportanceSampling::SphereData sphere;
    // 获取球心和半径
    uint3 data = gImportanceSamplingObjectDataBuffer.Load3(offset);
    offset += 12;
    sphere.center = asfloat(data);

    data.x = gImportanceSamplingObjectDataBuffer.Load(offset);
    sphere.radius = asfloat(data.x);
    
    return sphere;
}

RayTracing::ImportanceSampling::QuadData GetQuadData(uint offset)
{
    RayTracing::ImportanceSampling::QuadData quad;
    // 变换矩阵 
    for(int i = 0; i < 4; i++)
    {
        uint4 data = gImportanceSamplingObjectDataBuffer.Load4(offset);
        offset += 16;
        quad.worldToObj[i] = asfloat(data);
    }

    // 获取四边形的顶点和边向量
    uint3 data = gImportanceSamplingObjectDataBuffer.Load3(offset);
    offset += 12;
    quad.q = asfloat(data);

    data = gImportanceSamplingObjectDataBuffer.Load3(offset);
    offset += 12;
    quad.u = asfloat(data);

    data = gImportanceSamplingObjectDataBuffer.Load3(offset);
    quad.v = asfloat(data);
    
    return quad;
}


enum PDFType
{
    SpherePDF,
    CosinePDF,
    ImportanceSamplingPDF,
    Count
};



// 正交基变换
float3 OrthonormalBasisTransform(float3 normal, float3 dir)
{
    normal = normalize(normal);
    float3 a = abs(normal.x) > 0.9 ? float3(0, 1, 0) : float3(1, 0, 0);
    float3 tangent = normalize(cross(normal, a));
    float3 bitangent = cross(normal, tangent);
    float3x3 TBN = float3x3(bitangent, tangent, normal);
    dir = dir[0] * TBN[0] + dir[1] * TBN[1] + dir[2] * TBN[2];
    return dir;
}




// Get PDF Value
// 球面均匀采样的PDF值
float GetSpherePDF()
{
    return 1.0f / (4.0f * sPI);
}

// 余弦加权半球采样的PDF值
float GetCosinePDF(float3 normal, float3 dir)
{
    float cosTheta = max(dot(normal, normalize(dir)), 0.0f);
    return cosTheta / sPI;
}

// 球体的重要性采样
float GetSphereImportanceSamplingPDF(float3 origin, float3 dir, uint primitiveDataOffset)
{
    RayTracing::ImportanceSampling::SphereData sphere = GetSphereData(primitiveDataOffset);

    // 需要变换到对象空间进行相交检测
    Ray ray = {origin, dir};

    float time = 0.0f;
    float3 oc = sphere.center - ray.origin;
    float a = dot(ray.direction, ray.direction);
    float h = dot(ray.direction, oc);
    float c = dot(oc, oc) - sphere.radius * sphere.radius;

    float discriminant = h * h -  a * c;
    if (discriminant < 0) { // 没有根则不相交
        return false;
    }
    float sqrtD = sqrt(discriminant);
    time = (h - sqrtD) / a;  // 计算方程的根
    if (time <= MIN_RAY_LENGTH || time >= MAX_RAY_LENGTH) { //不再范围内
        // 由于生成随机向量时只考虑了球外的情况，因此不需要检测是否球内相交
        return false;
    }

    // 计算立体角
    float3 rayToCenter = sphere.center - origin;
    float distSquared = dot(rayToCenter, rayToCenter);
    float cosThetaMax = sqrt(1 - sphere.radius * sphere.radius / distSquared);
    float solidAngle = 2 * s_PI * (1 - cosThetaMax);

    return 1.0f / solidAngle;
}

float GetQuadImportanceSamplingPDF(float3 origin, float3 dir, uint primitiveDataOffset)
{
    RayTracing::ImportanceSampling::QuadData quad = GetQuadData(primitiveDataOffset);
    
    // 获取着色点到四边形的距离
    Ray ray = { origin, dir };
    RayTracing::ProceduralPrimitiveAttributes attributes;
    float time = 0.0f;
    if(!RayQuadIntersectionTest(ray, float2(MIN_RAY_LENGTH, MAX_RAY_LENGTH), quad.q, quad.u, quad.v, attributes, time)) {
        return 0.0f;
    }

    // 从四边形上的面积投影到立体角转换到
    // dw  = (dA * cosθ) / (r^2)
    // dA = dw * (r^2) / cosθ
    float distanceSquared = time * time * dot(dir, dir);
    float cosine = abs(dot(dir, attributes.normal) / length(dir));
    float area = length(cross(quad.u, quad.v));

    return distanceSquared / (cosine * area);
}

float GetImportanceSamplingPDF(float3 origin, float3 dir)
{
    float pdfVal = 0;
    const float numObjects = gSceneCB.numImportanceSamplingObjects;
    if(numObjects <= 0)
        return pdfVal;
    
    float weight = 1.0f / numObjects;
    for(uint i = 0; i < (uint)numObjects; i++)
    {
        float val = 0;
        RayTracing::ImportanceSampling::ImportanceSamplingObject obj = gImportanceSamplingObjects[i];
        switch(obj.type){
        case RayTracing::ImportanceSampling::ImportanceSamplingPrimitiveType::Sphere: {
            val = GetSphereImportanceSamplingPDF(origin, dir, obj.primitiveDataOffset);
            break;
        }
        case RayTracing::ImportanceSampling::ImportanceSamplingPrimitiveType::Quad: {
            val = GetQuadImportanceSamplingPDF(origin, dir, obj.primitiveDataOffset);
            break;
        }
        default:
            val = 0.0f;
            break;
        }
        pdfVal += val * weight;
    }
    return pdfVal;
}

float GetPDFValue(PDFType pdfType, Ray incomingRay, Ray scatterRay, Surface surface)
{
    switch(pdfType) {
    case PDFType::SpherePDF: {
        return GetSpherePDF();
    }
    case PDFType::CosinePDF: {
        return GetCosinePDF(surface.normal, scatterRay.direction);
    }
    case PDFType::ImportanceSamplingPDF: {
        return GetImportanceSamplingPDF(scatterRay.origin, scatterRay.direction);
    }
    default:
        return 0.0f;
    }
}

// Mixture PDF
float GetMixturePDFValue(PDFType pdfTypes[2], Ray incomingRay, Ray scatterRay, Surface surface)
{
    float pdf1 = GetPDFValue(pdfTypes[0], incomingRay, scatterRay, surface);
    float pdf2 = GetPDFValue(pdfTypes[1], incomingRay, scatterRay, surface);
    return 0.5f * (pdf1 + pdf2);
}




// Sample PDF
float3 SampleSpherePDF(inout uint state)
{
    return RandomUnitVector(state);
}

float3 SampleCosinePDF(float3 normal, inout uint state)
{
    return OrthonormalBasisTransform(normal, RandomCosineDirection(state));
}

// 球体的重要性采样
float3 SampleSphereImportanceSamplingPDF(float3 origin, uint primitiveDataOffset, inout uint state)
{
    RayTracing::ImportanceSampling::SphereData sphere = GetSphereData(primitiveDataOffset);

    float3 direction = sphere.center - origin;
    float distanceSquared = dot(direction, direction);
    
    float r1 = RandomFloat(state);
    float r2 = RandomFloat(state);
    float cosThetaMax = sqrt(1 - sphere.radius * sphere.radius / distanceSquared);
    float z = 1 + r2 * (cosThetaMax - 1);
    float sinTheta = sqrt(1 - z * z);

    float phi = 2 * s_PI * r1;
    float x = cos(phi) * sinTheta;
    float y = sin(phi) * sinTheta;
    float3 sampleDir = float3(x, y, z);

    return OrthonormalBasisTransform(direction, sampleDir);
}

float3 SampleQuadImportanceSamplingPDF(float3 origin, uint primitiveDataOffset, inout uint state)
{
    RayTracing::ImportanceSampling::QuadData quad = GetQuadData(primitiveDataOffset);
    // 在四边形上随机取一个点
    float3 p = quad.q + (RandomFloat(state) * quad.u) + (RandomFloat(state) * quad.v);
    return p - origin;
}

float3 SampleImportanceSamplingPDF(float3 origin, inout uint state)
{
    uint size = gSceneCB.numImportanceSamplingObjects;
    if(size <= 0)
        return 0;

    uint index = RandomUint(state, 0, size - 1);
    RayTracing::ImportanceSampling::ImportanceSamplingObject obj = gImportanceSamplingObjects[index];
    
    switch(obj.type){
    case RayTracing::ImportanceSampling::ImportanceSamplingPrimitiveType::Sphere: {
        return SampleSphereImportanceSamplingPDF(origin, obj.primitiveDataOffset, state);
    }
    case RayTracing::ImportanceSampling::ImportanceSamplingPrimitiveType::Quad: {
        return SampleQuadImportanceSamplingPDF(origin, obj.primitiveDataOffset, state);
    }
    default:
        return 0;
    }
}

float3 SamplePDF(PDFType pdfType, Surface surface, inout uint state)
{
    switch(pdfType) {
    case PDFType::SpherePDF: {
        return SampleSpherePDF(state);
    }
    case PDFType::CosinePDF: {
        return SampleCosinePDF(surface.normal, state);
    }
    case PDFType::ImportanceSamplingPDF: {
        return SampleImportanceSamplingPDF(surface.position, state);
    }
    default:
        return 0;
    }
}


float3 SampleMixturePDF(PDFType pdfTypes[2], Surface surface, inout uint state)
{
    PDFType selectedPDF = RandomFloat(state) < 0.5f ? pdfTypes[0] : pdfTypes[1];
    return SamplePDF(selectedPDF, surface, state);
}




float GetScatteringPDF(RayTracing::MaterialType::Type matType, Ray incomingRay, Ray scatterRay, Surface surface)
{
    switch(matType){
    case RayTracing::MaterialType::Lambertian: {
        float3 normal = surface.normal;
        float cosTheta = max(dot(normal, normalize(scatterRay.direction)), 0.0f);
        return cosTheta / sPI;
    }
    default:
        return 0.0f;
    }
}


#endif // __PDF_HLSLI__