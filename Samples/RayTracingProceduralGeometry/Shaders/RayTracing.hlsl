#include "RayTracingHLSLCompat.h"
#include "Common.hlsli"
#include "Light.hlsli"

// Global
// 输出图像
RWTexture2D<float4> gOutput : register(u0);
// 场景中的几何数据
RaytracingAccelerationStructure gScene : register(t0);
ConstantBuffer<RayTracing::SceneConstantBuffer> gSceneCB : register(b0);


// Local
ConstantBuffer<MaterialConstantBuffer> lMaterialCB : register(b1);

StructuredBuffer<uint3> lIndexBuffer : register(t1);
StructuredBuffer<float3> lNormalBuffer : register(t2);
StructuredBuffer<float2> lUVBuffer : register(t3);

Texture2D<float4> lBaseColorTex : register(t4);
Texture2D<float4> lDiffuseRoughnessTex : register(t5);
Texture2D<float4> lMetalnessTex : register(t6);
Texture2D<float> lOcclusionTex : register(t7);
Texture2D<float3> lEmissiveTex : register(t8);
Texture2D<float3> lNormalTex : register(t9);


RayTracing::Ray GenerateCameraRay(int2 index, float3 cameraPos, float3 viewportU, float3 viewportV, float focusDist)
{
    uint2 dimension = DispatchRaysDimensions().xy;
    float3 front = normalize(cross(viewportV, viewportU));

    float3 pixelDeltaU = viewportU / dimension.x;
    float3 pixelDeltaV = viewportV / dimension.y;
    
    float3 startPixelCenter = cameraPos + front * focusDist - (viewportU + viewportV) * 0.5f;
    startPixelCenter += (pixelDeltaU + pixelDeltaV) * 0.5f;

    float3 pixelSample = startPixelCenter + index.x * pixelDeltaU + index.y * pixelDeltaV;

    RayTracing::Ray ray;
    ray.origin = cameraPos;
    ray.direction = normalize(pixelSample - ray.origin);
    return ray;
}

float3 GetHitAttributes(float3 attributes[3], float2 barycentrics)
{
    float3 barycentrics3 = float3(1 - barycentrics.x - barycentrics.y, barycentrics.x, barycentrics.y);
    float3 attribute = attributes[0] * barycentrics3.x + attributes[1] * barycentrics3.y + attributes[2] * barycentrics3.z;
    return attribute;
}


float4 TraceRadianceRay(RayTracing::Ray ray, uint depth)
{
    [branch]
    if(depth >= MAX_TRACE_RECURSION_DEPTH) {
        return 0;
    }

    RayDesc rayDesc;
    rayDesc.Origin = ray.origin;
    rayDesc.Direction = ray.direction;
    rayDesc.TMin = 0.001f;
    rayDesc.TMax = 10000.0f;

    RayTracing::RayPayload payload;
    payload.color = float4(0, 0, 0, 1);
    payload.depth = depth + 1;
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
    RayTracing::Ray ray = GenerateCameraRay(
        DispatchRaysIndex().xy,
        gSceneCB.cameraPosAndFocusDist.xyz,
        gSceneCB.viewportU.xyz,
        gSceneCB.viewportV.xyz,
        gSceneCB.cameraPosAndFocusDist.w);

    float4 color = TraceRadianceRay(ray, 0);

    // 手动进行伽马映射
    color.rgb = LinearToSRGB(color.rgb);
    gOutput[DispatchRaysIndex().xy] = color;
}

[shader("closesthit")]
void ClosestHitShader(inout RayTracing::RayPayload payload, in BuiltInTriangleIntersectionAttributes attrs)
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
    baseCol *= lMaterialCB.baseColor;
    float roughness = lDiffuseRoughnessTex.SampleLevel(gAnisoWrapSampler, uv, 0).g;
    float metallic = lMetalnessTex.SampleLevel(gAnisoWrapSampler, uv, 0).b;
    float occlusion = lOcclusionTex.SampleLevel(gAnisoWrapSampler, uv, 0).r;
    float3 emissive = lEmissiveTex.SampleLevel(gAnisoWrapSampler, uv, 0).rgb;

    // 感知上的粗糙度
    float perceptualRoughness = roughness * lMaterialCB.roughnessFactor;

    Surface surface;
    surface.position = GetWorldPosition();
    surface.depth = RayTCurrent();
    surface.normal = normal;
    surface.roughness = perceptualRoughness * perceptualRoughness;
    surface.roughness = max(0.05, surface.roughness);
    surface.color = baseCol.rgb;
    surface.alpha = baseCol.a;
    surface.viewDir = -WorldRayDirection();
    surface.metallic = metallic * lMaterialCB.metallicFactor;

    // 计算光照
    float4 color = float4(ShadeLighting(surface), surface.alpha);
    color.rgb *= occlusion;
    color.rgb += emissive * lMaterialCB.emissiveColor.rgb;

    // 若表面很粗糙则不追踪反射光线
    [branch]
    if(surface.roughness <= 0.9f) {
        // 获得反射光线
        RayTracing::Ray reflectRay;
        reflectRay.origin = surface.position + 0.001 * surface.normal;
        reflectRay.direction = reflect(WorldRayDirection(), surface.normal);
        float4 refCol = TraceRadianceRay(reflectRay, payload.depth);

        // 计算反射系数
        float3 f0 = lerp(s_DielectricSpecular, surface.color, surface.metallic);
        float cos = max(0, dot(surface.normal, surface.viewDir));
        float3 F = F_Schlick(f0, 1.0, cos);

        refCol.rgb *= F * (1 - surface.roughness);
        color += refCol;
    }

    payload.color = color;
}

[shader("miss")]
void MissShader(inout RayTracing::RayPayload payload)
{
    payload.color = float4(0.529, 0.808, 0.922, 1);
}