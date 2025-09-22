#include "RayTracingHLSLCompat.h"


// Global
// 输出图像
RWTexture2D<float4> gOutput : register(u0);
// 场景中的几何数据
RaytracingAccelerationStructure gScene : register(t0);
ConstantBuffer<RayTracing::SceneConstantBuffer> gSceneCB : register(b0);
SamplerState gAnisoWrapSampler : register(s0);


// Local
ConstantBuffer<RayTracing::Material> lMaterialCB : register(b1);

StructuredBuffer<uint3> lIndexBuffer : register(t1);
StructuredBuffer<float3> lNormalBuffer : register(t2);
StructuredBuffer<float2> lUVBuffer : register(t3);

Texture2D<float4> lBaseColorTex : register(t4);
Texture2D<float4> lDiffuseRoughnessTex : register(t5);
Texture2D<float> lMetalnessTex : register(t6);
Texture2D<float> lOcclusionTex : register(t7);
Texture2D<float3> lEmissiveTex : register(t8);
Texture2D<float3> lNormalTex : register(t9);


RayDesc GetRay(int2 index)
{
    uint2 dimension = DispatchRaysDimensions().xy;
    float3 viewportU = gSceneCB.viewportU.xyz;
    float3 viewportV = gSceneCB.viewportV.xyz;
    float3 front = normalize(cross(viewportV, viewportU));

    float3 pixelDeltaU = viewportU / dimension.x;
    float3 pixelDeltaV = viewportV / dimension.y;
    
    float3 cameraPos = gSceneCB.cameraPosAndFocusDist.xyz;
    float focusDist = gSceneCB.cameraPosAndFocusDist.w;
    float3 startPixelCenter = cameraPos + front * focusDist - (viewportU + viewportV) * 0.5f;
    startPixelCenter += (pixelDeltaU + pixelDeltaV) * 0.5f;

    float3 pixelSample = startPixelCenter + index.x * pixelDeltaU + index.y * pixelDeltaV;

    RayDesc ray;
    ray.Origin = cameraPos;
    ray.Direction = normalize(pixelSample - ray.Origin);
    ray.TMin = 0.001f;
    ray.TMax = 10000.0f;
    return ray;
}

float3 GetHitAttributes(float3 attributes[3], float2 barycentrics)
{
    float3 barycentrics3 = float3(1 - barycentrics.x - barycentrics.y, barycentrics.x, barycentrics.y);
    float3 attribute = attributes[0] * barycentrics3.x + attributes[1] * barycentrics3.y + attributes[2] * barycentrics3.z;
    return attribute;
}

[shader("raygeneration")]
void RaygenShader()
{
    RayDesc ray = GetRay(DispatchRaysIndex().xy);

    RayTracing::RayPayload payload;
    payload.color = float4(0, 0, 0, 1);
    TraceRay(gScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, ~0, 0, 1, 0, ray, payload);
    gOutput[DispatchRaysIndex().xy] = payload.color;
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
    uv -= int2(uv);
    uv *= sign(uv);
    
    float4 baseColor = lBaseColorTex.SampleLevel(gAnisoWrapSampler, uv, 0);

    float3 lightDir = -normalize(gSceneCB.lightDir.xyz);
    float3 lightColor = gSceneCB.lightColor.rgb;
    float3 halfDir = normalize(lightDir - normalize(WorldRayDirection()));

    float3 diffuse = lightColor * max(0, dot(lightDir, normal));
    float3 specular = lightColor * pow(max(0, dot(halfDir, normal)), 90);
    float3 col = saturate(diffuse + specular + 0.1);
    col *= baseColor.rgb;

    // 获取重心坐标
    payload.color = float4(col, baseColor.a);
}

[shader("miss")]
void MissShader(inout RayTracing::RayPayload payload)
{
    payload.color = float4(0.529, 0.808, 0.922, 1);
}