#ifndef __COMMON_HLSLI__
#define __COMMON_HLSLI__

// 默认采样器
SamplerState gAnisoWrapSampler : register(s0);



float3 GetWorldPosition()
{
    return WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
}

float3 LinearToSRGB(float3 linearColor)
{
    return pow(linearColor, 1.0 / 2.2f);
}

#endif // __COMMON_HLSLI__