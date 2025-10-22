#ifndef __COMMON_HLSLI__
#define __COMMON_HLSLI__

// 默认采样器
SamplerState gAnisoWrapSampler : register(s0);

// Global
// 输出图像
RWTexture2D<float4> gOutput : register(u0);
// 场景中的几何数据
RaytracingAccelerationStructure gScene : register(t0);
ConstantBuffer<RayTracing::SceneConstantBuffer> gSceneCB : register(b0);


static const float s_PI = 3.14159265;

struct Surface
{
    float3 position;
    float3 normal;
    bool frontFace;
    float2 uv;
    float3 color;
    uint seed;
};


float3 GetWorldPosition()
{
    return WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
}

float3 GetObjectPosition()
{
    return ObjectRayOrigin() + RayTCurrent() * ObjectRayDirection();
}

float3 LinearToSRGB(float3 linearColor)
{
    return pow(linearColor, 1.0f / 2.2f);
}

bool InRange(float value, float min, float max)
{
    return min <= value && value <= max;
}

bool NearZero(float3 value)
{
    return dot(value, value) < 1e-6;
}

#endif // __COMMON_HLSLI__