#ifndef __RESOURCEDATA_HLSLI__
#define __RESOURCEDATA_HLSLI__


#if defined(__cplusplus)
using float3 = DSM::Math::Vector3;
using float4 = DSM::Math::Vector4;
using float3x3 = DSM::Math::Matrix3;
using float4x4 = DSM::Math::Matrix4;
#endif

#define MAX_DIRECTIONAL_LIGHT_COUNT 4

struct DirectionalLightData
{
    float4 color;
    float4 direction;
};

#endif