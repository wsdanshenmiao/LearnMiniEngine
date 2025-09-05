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
    float4x4 viewProjInv;  // 投影矩阵的逆矩阵
    float4 cameraPos;
};

#endif