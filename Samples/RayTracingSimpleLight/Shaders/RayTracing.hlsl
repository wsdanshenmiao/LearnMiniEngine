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
StructuredBuffer<uint3> gIndexBuffer : register(t2);

// 输出图像
RWTexture2D<float4> gOutput : register(u0);

ConstantBuffer<SceneConstantBuffer> gSceneCB : register(b0);
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
    uint3 indices = gIndexBuffer[PrimitiveIndex()];
    Vertex vertices[3] = {
        gVertexBuffer[indices[0]],
        gVertexBuffer[indices[1]],
        gVertexBuffer[indices[2]]
    };

    Vertex attributes = GetHitAttributes(vertices, attrs.barycentrics);
    float3 normals[3] = {
        gVertexBuffer[indices[0]].normal,
        gVertexBuffer[indices[1]].normal,
        gVertexBuffer[indices[2]].normal
    };
    float3 normal = normals[0] +
        attrs.barycentrics.x * (normals[1] - normals[0]) +
        attrs.barycentrics.y * (normals[2] - normals[0]);

    float3 lightDir = -normalize(gSceneCB.lightDir.xyz);
    float3 diffuse = gSceneCB.lightColor.rgb * gCubeCB.albedo.rgb * max(0, dot(lightDir, normal));

    // 获取重心坐标
    payload.color = float4(diffuse, gCubeCB.albedo.a);
}

[shader("miss")]
void MissShader(inout RayPayload payload)
{
    payload.color = float4(0, 0, 0, 1);
}