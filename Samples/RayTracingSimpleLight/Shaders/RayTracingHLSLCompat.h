#ifndef __RAYTRACING_HLSL_COMPAT_H__
#define __RAYTRACING_HLSL_COMPAT_H__

#if defined(__cplusplus)
using float3 = DSM::Math::Vector3;
using float4 = DSM::Math::Vector4;
using float3x3 = DSM::Math::Matrix3;
using float4x4 = DSM::Math::Matrix4;
#endif

struct CubeConstantBuffer
{
    float4 albedo;
};

struct SceneConstantBuffer
{
    // 生成光线使用的数据
    float4 cameraPosAndFocusDist;
    float4 viewportU;
    float4 viewportV;
    float4 lightDir;
    float4 lightColor;
};

#endif