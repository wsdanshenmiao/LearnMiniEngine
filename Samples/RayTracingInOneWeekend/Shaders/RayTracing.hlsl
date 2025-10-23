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
    float2 offset = RandomFloat2(seed, -0.5f, 0.5f);
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
    rayDesc.TMin = 0.001f;
    rayDesc.TMax = 1000.0f;

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
    
    return payload.color;
}


[shader("raygeneration")]
void RaygenShader()
{
    uint2 dimension = DispatchRaysDimensions().xy;
    uint2 index = DispatchRaysIndex().xy;

    // 获取常量缓冲区中的数据
    float3 cameraPos = gSceneCB.cameraPos.xyz;
    float focusDist = gSceneCB.focusDistDefocusAngle.x;
    float defocusAngle = max(0, gSceneCB.focusDistDefocusAngle.y);
    float3 viewportU = gSceneCB.viewportUAndFrameIndex.xyz;
    float3 viewportV = gSceneCB.viewportVAndSamplePerPixel.xyz;
    float samplesPerPixel = gSceneCB.viewportVAndSamplePerPixel.w;
    float totalTime = gSceneCB.backgroundColorAndTotalTime.w;
    uint frameIndex = uint(gSceneCB.viewportUAndFrameIndex.w);

    // 初始化随机数状态
    uint pcgState = PCG_Init(index, asuint(frameIndex * totalTime));

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
    uint spp = uint(samplesPerPixel);
    for(uint sampleIndex = 0; sampleIndex < spp; ++sampleIndex){
        RayTracing::Ray ray = GenerateCameraRay(genParams, pcgState);
        color += TraceRadianceRay(ray, 0, pcgState);
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
    surface.seed = payload.seed;

    float3 attenuation;
    float3 rayDir;
    float3 emission;
    bool scatter = GetMaterialScatter(lMaterialCB, surface, attenuation, rayDir, emission);
    [branch]
    if(scatter) {
        Ray ray;
        ray.origin = surface.position;
        ray.direction = normalize(rayDir);
        float4 scatterCol = TraceRadianceRay(ray, payload.depth, surface.seed);

        scatterCol *= float4(attenuation, 1);
        payload.color = float4(scatterCol.rgb + emission, scatterCol.a);
    }
    else {
        payload.color = float4(emission, 1);
    }
    payload.seed = surface.seed;
}

[shader("closesthit")]
void ClosestHitShader_AABB(inout RayTracing::RayPayload payload, in RayTracing::ProceduralPrimitiveAttributes attrs)
{
    float2 uv = attrs.uv;
    float3 normal = normalize(attrs.normal);
    float4 baseCol = lBaseColorTex.SampleLevel(gAnisoWrapSampler, uv, 0);
    
    Surface surface;
    surface.position = GetWorldPosition();
    surface.normal = normal;
    surface.frontFace = attrs.frontFace;
    surface.uv = uv;
    surface.color = baseCol.rgb;
    surface.seed = payload.seed;

    float3 attenuation;
    float3 rayDir;
    float3 emission;
    bool scatter = GetMaterialScatter(lMaterialCB, surface, attenuation, rayDir, emission);
    [branch]
    if(scatter) {
        Ray ray;
        ray.origin = surface.position;
        ray.direction = normalize(rayDir);
        float4 scatterCol = TraceRadianceRay(ray, payload.depth, surface.seed);

        scatterCol *= float4(attenuation, 1);
        payload.color = float4(scatterCol.rgb + emission, scatterCol.a);
    }
    else {
        payload.color = float4(emission, 1);
    }
    payload.seed = surface.seed;
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
    if(RayAnalyticPrimitiveIntersectionTest(ray, primType, attrs, time)){
        ReportHit(time, 0, attrs);
    }
}