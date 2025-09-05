#include "RayTracingHLSLCompat.h"

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
ByteAddressBuffer gIndexBuffer : register(t2);

// 输出图像
RWTexture2D<float4> gOutput : register(u0);

ConstantBuffer<SceneConstantBuffer> gSceneCB : register(b0);
ConstantBuffer<CubeConstantBuffer> gCubeCB : register(b1);


RayDesc GetRay(int2 index)
{
    // 还原 NDC 坐标
    float2 pixelCenter = index + 0.5f;
    float2 ndc = pixelCenter / DispatchRaysDimensions().xy * 2.0f - 1.0f;
    ndc.y = -ndc.y;

    float4 posWS = mul(float4(ndc, 0, 1), gSceneCB.viewProjInv);
    posWS /= posWS.w;

    RayDesc ray;
    ray.Origin = gSceneCB.cameraPos.xyz;
    ray.Direction = normalize(posWS.xyz - ray.Origin);
    ray.TMin = 0.001f;
    ray.TMax = 10000.0f;
    return ray;
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
    // 获取重心坐标
    float3 barycentrics = float3(1 - attrs.barycentrics.x - attrs.barycentrics.y, attrs.barycentrics.x, attrs.barycentrics.y);
    payload.color = float4(barycentrics, 1);
}

[shader("miss")]
void MissShader(inout RayPayload payload)
{
    payload.color = float4(0, 0, 0, 1);
}