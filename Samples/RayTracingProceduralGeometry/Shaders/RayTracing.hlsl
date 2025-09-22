#include "RayTracingHLSLCompat.h"


// // Global
// // 输出图像
// RWTexture2D<float4> gOutput : register(u0);
// // 场景中的几何数据
// RaytracingAccelerationStructure gScene : register(t0);
// ConstantBuffer<RayTracing::SceneConstantBuffer> gSceneCB : register(b0);


// // Local
// ConstantBuffer<RayTracing::Material> lMaterialCB : register(b1);

// StructuredBuffer<uint3> lIndexBuffer : register(t1);
// StructuredBuffer<float3> lNormalBuffer : register(t2);
// StructuredBuffer<float3> lUVBuffer : register(t3);

// Texture2D<float4> lBaseColorTex : register(t4);
// Texture2D<float4> lDiffuseRoughnessTex : register(t5);
// Texture2D<float> lMetalnessTex : register(t6);
// Texture2D<float> lOcclusionTex : register(t7);
// Texture2D<float3> lEmissiveTex : register(t8);
// Texture2D<float3> lNormalTex : register(t9);


struct RayPayload
{
    float4 color;
};

struct Vertex
{
    float3 position;
    float3 normal;
    float4 tangent;
    float3 biTangent;
    float2 texCoord;
};

// 场景中的几何数据
RaytracingAccelerationStructure gScene : register(t0);
StructuredBuffer<Vertex> gVertexBuffer : register(t1);
StructuredBuffer<uint3> gIndexBuffer : register(t2);

// 输出图像
RWTexture2D<float4> gOutput : register(u0);

// 全局常量缓冲区
ConstantBuffer<SceneConstantBuffer> gSceneCB : register(b0);
// 局部常量缓冲区
ConstantBuffer<CubeConstantBuffer> gCubeCB : register(b1);


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

Vertex GetHitAttributes(Vertex vertices[3], float2 barycentrics)
{
    float3 barycentrics3 = float3(1 - barycentrics.x - barycentrics.y, barycentrics.x, barycentrics.y);
    Vertex attributes;
    attributes.position = vertices[0].position * barycentrics3.x + vertices[1].position * barycentrics3.y + vertices[2].position * barycentrics3.z;
    attributes.normal = normalize(vertices[0].normal * barycentrics3.x + vertices[1].normal * barycentrics3.y + vertices[2].normal * barycentrics3.z);
    attributes.tangent = normalize(vertices[0].tangent * barycentrics3.x + vertices[1].tangent * barycentrics3.y + vertices[2].tangent * barycentrics3.z);
    attributes.biTangent = normalize(vertices[0].biTangent * barycentrics3.x + vertices[1].biTangent * barycentrics3.y + vertices[2].biTangent * barycentrics3.z);
    attributes.texCoord = vertices[0].texCoord * barycentrics3.x + vertices[1].texCoord * barycentrics3.y + vertices[2].texCoord * barycentrics3.z;
    return attributes;
}

[shader("raygeneration")]
void RaygenShader()
{
    RayDesc ray = GetRay(DispatchRaysIndex().xy);

    RayPayload payload;
    payload.color = float4(0, 0, 0, 1);
    TraceRay(gScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, ~0, 0, 1, 0, ray, payload);
    gOutput[DispatchRaysIndex().xy] = payload.color;
}

[shader("closesthit")]
void ClosestHitShader(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attrs)
{
    float3 lightDir = -normalize(gSceneCB.lightDirAndGloss.xyz);
    float3 lightColor = gSceneCB.lightColor.rgb;
    float3 halfDir = normalize(lightDir - normalize(WorldRayDirection()));
    float gloss = gSceneCB.lightDirAndGloss.w;

    float3 normal = float3(0, 1, 0);

    float4 albedo = gCubeCB.albedo;
    float3 diffuse = lightColor * max(0, dot(lightDir, normal));
    float3 specular = lightColor * pow(max(0, dot(halfDir, normal)), gloss);
    float3 col = saturate(diffuse + specular + 0.1);
    col *= albedo.rgb;

    // 获取重心坐标
    payload.color = float4(col, albedo.a);
}

[shader("miss")]
void MissShader(inout RayPayload payload)
{
    payload.color = float4(0, 0, 0, 1);
}