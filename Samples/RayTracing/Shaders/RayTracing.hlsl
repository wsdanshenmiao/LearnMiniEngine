#include "RayTracingHLSLCompat.h"

struct RayPayload
{
    float4 color;
};

RaytracingAccelerationStructure gScene : register(t0);
RWTexture2D<float4> gOutput : register(u0);
ConstantBuffer<RayGenConstantBuffer> gRayGenCB : register(b0);

bool IsInsideViewport(float2 p, Viewport viewport)
{
    return viewport.left <= p.x && p.x <= viewport.right &&
        viewport.top <= p.y && p.y <= viewport.bottom;
}

[shader("raygeneration")]
void RaygenShader()
{
    float2 rayIndex = DispatchRaysIndex().xy;
    float3 lerpVal = float3(DispatchRaysIndex()) / DispatchRaysDimensions();
    float3 dir = float3(0, 0, 1);
    float3 origin = float3(
        lerp(gRayGenCB.viewport.left, gRayGenCB.viewport.right, lerpVal.x),
        lerp(gRayGenCB.viewport.top, gRayGenCB.viewport.bottom, lerpVal.y),
        0.0f);

    if(IsInsideViewport(origin.xy, gRayGenCB.stencil)){
        RayDesc ray;
        ray.Origin = origin;
        ray.Direction = dir;
        ray.TMin = 0.001;
        ray.TMax = 10000;

        // 光线负载
        RayPayload payload = { float4(0, 0, 0, 0) };
        TraceRay(gScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, ~0, 0, 1, 0, ray, payload);

        gOutput[rayIndex] = payload.color;
    }
    else{
        gOutput[rayIndex] = float4(gRayGenCB.outSideColor, 1);
    }
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