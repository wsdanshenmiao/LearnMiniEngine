#include "RayTracingHLSLCompat.h"
#include "Common.hlsli"
#include "AnalyticPrimitives.hlsli"
#include "Material.hlsli"

using namespace RayTracing;

// Common Local
Texture2D<float4> lBaseColorTex : register(t4);
ConstantBuffer<MaterialConstants> lMaterialCB : register(b1);

// Triangle Geometry Local
StructuredBuffer<uint3> lIndexBuffer : register(t1);
StructuredBuffer<float3> lNormalBuffer : register(t2);
StructuredBuffer<float2> lUVBuffer : register(t3);

// Procedural Geometry Local
ConstantBuffer<RayTracing::PrimitiveInstanceConstantBuffer> lPrimitiveInstanceCB : register(b2);

struct GenerateCameraRayParams
{
    uint2 index;
    uint2 subPixelIndex;
    float invSPP;
    float3 cameraPos;
    float3 viewportStart;
    float3 pixelDeltaU;
    float3 pixelDeltaV;
    float3 defocusU;
    float3 defocusV;
};

RayTracing::Ray GenerateCameraRay(GenerateCameraRayParams params, inout uint seed)
{
    // 在像素内随机取样
    float2 offset = RandomFloat2(seed) + params.subPixelIndex;
    offset *= params.invSPP;
    offset -= 0.5f;
    float2 pixelOffset = float2(params.index) + offset;
    float3 pixelSample = params.viewportStart + pixelOffset.x * params.pixelDeltaU + pixelOffset.y * params.pixelDeltaV;

    // 进行散焦模糊的随机偏移
    float2 defocusDisk = RandomInUnitDisk(seed);
    float3 center = params.cameraPos + defocusDisk.x * params.defocusU + defocusDisk.y * params.defocusV;

    RayTracing::Ray ray;
    ray.origin = center;
    ray.direction = normalize(pixelSample - ray.origin);
    return ray;
}

float3 GetHitAttributes(float3 attributes[3], float2 barycentrics)
{
    float3 barycentrics3 = float3(1 - barycentrics.x - barycentrics.y, barycentrics.x, barycentrics.y);
    float3 attribute = attributes[0] * barycentrics3.x + attributes[1] * barycentrics3.y + attributes[2] * barycentrics3.z;
    return attribute;
}


float4 TraceRadianceRay(RayTracing::Ray ray, uint depth, inout uint seed)
{
    [branch]
    if(depth >= MAX_TRACE_RECURSION_DEPTH) {
        return 0;
    }

    RayDesc rayDesc;
    rayDesc.Origin = ray.origin;
    rayDesc.Direction = ray.direction;
    rayDesc.TMin = MIN_RAY_LENGTH;
    rayDesc.TMax = MAX_RAY_LENGTH;

    RayTracing::RayPayload payload;
    payload.color = float4(0, 0, 0, 1);
    payload.depth = depth + 1;
    payload.seed = seed;
    TraceRay(gScene, 
        RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 
        RayTracing::TraceRayParameters::InstanceMark, 
        RayTracing::TraceRayParameters::HitGroup::Offset[RayTracing::RayType::Radiance], 
        RayTracing::TraceRayParameters::HitGroup::GeometryStride, 
        RayTracing::TraceRayParameters::MissShader::Offset[RayTracing::RayType::Radiance], 
        rayDesc, 
        payload);

    seed = payload.seed;
    
    return payload.color;
}

float4 GetColor(inout RayTracing::RayPayload payload, Surface surface)
{
    Ray incomingRay = {WorldRayOrigin(), WorldRayDirection()};

    ScatterRecord scatterRecord;
    float4 color = 0;
    [branch]
    if(!GetMaterialScatter(lMaterialCB, surface, scatterRecord, payload.seed)){
        return float4(scatterRecord.emission, 1);
    }

    // 是否需要根据 PDF 进行采样
    [branch]
    if(scatterRecord.skipPDF) {
        color = TraceRadianceRay(scatterRecord.scatterRay, payload.depth, payload.seed);
        color *= float4(scatterRecord.attenuation, 1);
    }
    else{
        // 使用混合 PDF 进行采样，一个是与物体表面 BRDF 相关的 PDF，一个是重要性采样 PDF
        PDFType pdfTypes[2] = {scatterRecord.pdfType, scatterRecord.pdfType};
        if(gSceneCB.numImportanceSamplingObjects > 0) {
            pdfTypes[1] = PDFType::ImportanceSamplingPDF;
        }

        // 根据 pdf 进行采样
        float3 pdfSampleDir = SampleMixturePDF(pdfTypes, surface, payload.seed);

        Ray scatterRay;
        scatterRay.origin = surface.position;
        scatterRay.direction = pdfSampleDir;
        // 获取材质的 PDF
        float pdfVal = GetMixturePDFValue(pdfTypes, incomingRay, scatterRay, surface);
        
        // 材质在特定方向进行散射的概率
        float scatterPDF = GetScatteringPDF(lMaterialCB.type, incomingRay, scatterRay, surface);
        
        color = TraceRadianceRay(scatterRay, payload.depth, payload.seed);
        color = (color * float4(scatterRecord.attenuation, 1) * scatterPDF) / pdfVal;
    }

    color.rgb += scatterRecord.emission;

    return color;
}


[shader("raygeneration")]
void RaygenShader()
{
    uint2 dimension = DispatchRaysDimensions().xy;
    uint2 index = DispatchRaysIndex().xy;

    // 获取常量缓冲区中的数据
    float3 cameraPos = gSceneCB.cameraPos.xyz;
    float focusDist = gSceneCB.focusDistAndDefocusAngle.x;
    float defocusAngle = max(0, gSceneCB.focusDistAndDefocusAngle.y);
    float3 viewportU = gSceneCB.viewportUAndFrameIndex.xyz;
    float3 viewportV = gSceneCB.viewportVAndSamplePerPixel.xyz;
    float samplesPerPixel = gSceneCB.viewportVAndSamplePerPixel.w;
    float totalTime = gSceneCB.backgroundColorAndTotalTime.w;
    uint frameIndex = uint(gSceneCB.viewportUAndFrameIndex.w);

    // 初始化随机数状态
    uint pcgState = PCG_Init(index, frameIndex);

    // 计算摄像机光线参数
    float3 front = normalize(cross(viewportV, viewportU));

    float3 pixelDeltaU = viewportU / dimension.x;
    float3 pixelDeltaV = viewportV / dimension.y;
    
    float3 viewportStart = cameraPos + front * focusDist - (viewportU + viewportV) * 0.5f;
    viewportStart += (pixelDeltaU + pixelDeltaV) * 0.5f;
    
    // 计算散焦模糊参数
    float defocusRadius = focusDist * tan(defocusAngle * 0.5f);
    float3 defocusU = normalize(viewportU) * defocusRadius;
    float3 defocusV = normalize(viewportV) * defocusRadius;

    GenerateCameraRayParams genParams;
    genParams.index = index;
    genParams.cameraPos = cameraPos;
    genParams.viewportStart = viewportStart;
    genParams.pixelDeltaU = pixelDeltaU;
    genParams.pixelDeltaV = pixelDeltaV;
    genParams.defocusU = defocusU;
    genParams.defocusV = defocusV;

    float4 color = float4(0,0,0,1);
    uint sqrtSPP = uint(sqrt(samplesPerPixel));
    uint spp = sqrtSPP * sqrtSPP;
    if(spp == 0) {
        gOutput[index] = float4(0, 0, 0, 1);
        return;
    }
    genParams.invSPP = 1.0f / float(spp);
    for(uint i = 0; i < sqrtSPP; ++i){
        for(uint j = 0; j < sqrtSPP; ++j){
            genParams.subPixelIndex = uint2(i, j);
            RayTracing::Ray ray = GenerateCameraRay(genParams, pcgState);
            float4 sampleColor = TraceRadianceRay(ray, 0, pcgState);
            color += sampleColor;
        }
    }
    color /= spp;

    // 手动进行伽马映射
    color.rgb = LinearToSRGB(color.rgb);
    gOutput[index] = select(isnan(color), float4(0, 0, 0, 1), color);
}

[shader("closesthit")]
void ClosestHitShader_Triangle(inout RayTracing::RayPayload payload, in BuiltInTriangleIntersectionAttributes attrs)
{
    uint3 indices = lIndexBuffer[PrimitiveIndex()];
    float3 normals[3] = {
            lNormalBuffer[indices[0]],
            lNormalBuffer[indices[1]],
            lNormalBuffer[indices[2]]};
    float3 uvs[3] = {
            lUVBuffer[indices[0]].xyy,
            lUVBuffer[indices[1]].xyy,
            lUVBuffer[indices[2]].xyy};
    float3 normal = normalize(GetHitAttributes(normals, attrs.barycentrics));
    float2 uv = GetHitAttributes(uvs, attrs.barycentrics).xy;

    float4 baseCol = lBaseColorTex.SampleLevel(gAnisoWrapSampler, uv, 0);

    Surface surface;
    surface.position = GetWorldPosition();
    surface.normal = normal;
    surface.frontFace = true;
    surface.uv = uv;
    surface.color = baseCol.rgb;

    payload.color = GetColor(payload, surface);
}

[shader("closesthit")]
void ClosestHitShader_AABB(inout RayTracing::RayPayload payload, in RayTracing::ProceduralPrimitiveAttributes attrs)
{
    float2 uv = attrs.uv;
    float3 normal = attrs.normal;
    float4 baseCol = lBaseColorTex.SampleLevel(gAnisoWrapSampler, uv, 0);
    
    Surface surface;
    surface.position = GetWorldPosition();
    surface.normal = normal;
    surface.frontFace = attrs.frontFace;
    surface.uv = uv;
    surface.color = baseCol.rgb;

    payload.color = GetColor(payload, surface);
}

[shader("miss")]
void MissShader(inout RayTracing::RayPayload payload)
{
    payload.color = float4(gSceneCB.backgroundColorAndTotalTime.rgb, 1);
}

[shader("intersection")]
void IntersectionShader_AnalyticPrimitive()
{
    Ray ray = {ObjectRayOrigin(), ObjectRayDirection()};
    RayTracing::AnalyticPrimitive::PrimitiveType primType = (RayTracing::AnalyticPrimitive::PrimitiveType)lPrimitiveInstanceCB.primitiveType;
    
    float time;
    RayTracing::ProceduralPrimitiveAttributes attrs = (RayTracing::ProceduralPrimitiveAttributes)0;
    if(RayAnalyticPrimitiveIntersectionTest(ray, float2(RayTMin(), RayTCurrent()), primType, attrs, time)){
        ReportHit(time, 0, attrs);
    }
}
