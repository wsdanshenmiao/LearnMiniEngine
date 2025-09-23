#ifndef __RAYTRACING_HLSL_COMPAT_H__
#define __RAYTRACING_HLSL_COMPAT_H__

#if defined(__cplusplus)
using float3 = DSM::Math::Vector3;
using float4 = DSM::Math::Vector4;
using float3x3 = DSM::Math::Matrix3;
using float4x4 = DSM::Math::Matrix4;
using uint = uint32_t;
#endif



struct MaterialConstantBuffer
{
    float4 baseColor;
    float4 emissiveColor;
    float normalTexScale;
    float metallicFactor;
    float roughnessFactor;
    float pad;
};

struct DirectionalLightData
{
    float4 color;
    float4 direction;
};

struct LightData
{
    uint dirLightCount;
};

namespace RayTracing {
    struct RayPayload
    {
        float4 color;
    };

    // 自定义图元的属性
    struct ProceduralPrimitiveAttributes
    {
        float3 normal;
    };

    // 场景的常量缓冲区
    struct SceneConstantBuffer
    {
        // 生成光线使用的数据
        float4 cameraPosAndFocusDist;
        float4 viewportU;
        float4 viewportV;
    };

    struct InstanceConstantBuffer
    {
        uint instanceIndex;
    };

    // 使用的光线种类
    enum RayType{
        Radiance = 0,
        Count
    };

    namespace TraceRayParameters {
        // 实例掩码
        static const uint InstanceMark = ~0;
        namespace HitGroup {
            static const uint Offset[RayType::Count] = {
                0   // 用于渲染的光线
            };
            static const uint GeometryStride = RayType::Count;
        }

        namespace MissShader {
            // Miss Shader 只需要使用索引
            static const uint Offset[RayType::Count] = {
                0   // 用于渲染的光线
            };
        }
    }

}


#endif