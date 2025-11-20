#ifndef __RAYTRACING_HLSL_COMPAT_H__
#define __RAYTRACING_HLSL_COMPAT_H__

#if defined(__cplusplus)
using float2 = std::pair<float, float>;
using float3 = DSM::Math::Vector3;
using float4 = DSM::Math::Vector4;
using float3x3 = DSM::Math::Matrix3;
using float4x4 = DSM::Math::Matrix4;
using uint = uint32_t;

struct PCGState
{
    uint state;
};

#else
#include "Random.hlsli"
#endif


#ifndef MAX_TRACE_RECURSION_DEPTH
#define MAX_TRACE_RECURSION_DEPTH 3
#endif


#ifndef MAX_RAY_LENGTH
#define MAX_RAY_LENGTH 10000.0f
#endif

#ifndef MIN_RAY_LENGTH
#define MIN_RAY_LENGTH 0.001f
#endif

namespace RayTracing {
    struct Ray
    {
        float3 origin;
        float3 direction;
    };

    struct RayPayload
    {
        float4 color;
        uint depth;
        PCGState rng;
    };

    // 自定义图元的属性
    struct ProceduralPrimitiveAttributes
    {
        float3 normal;
        float2 uv;
        bool frontFace;
    };

    // 场景的常量缓冲区
    struct SceneConstantBuffer
    {
        // 生成光线使用的数据
        float4 cameraPos;
        float4 viewportUAndFrameIndex;
        float4 viewportVAndSamplePerPixel;
        float4 backgroundColorAndTotalTime;
        float2 focusDistAndDefocusAngle;
        float numImportanceSamplingObjects;
        float pad;
    };

    struct PrimitiveInstanceConstantBuffer
    {
        uint primitiveType; // 图元类型
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
                0,   // 用于渲染的光线
            };
            static const uint GeometryStride = RayType::Count;
        }

        namespace MissShader {
            // Miss Shader 只需要使用索引
            static const uint Offset[RayType::Count] = {
                0,   // 用于渲染的光线
            };
        }
    }

    // 解析几何的类型
    namespace AnalyticPrimitive{
        enum PrimitiveType{
            Sphere = 0,
            Quad,
            Cube,
            Count
        };
    }

    namespace MaterialType{
        enum Type{
            Lambertian = 0,
            Metal,
            Dielectric,
            DiffuseLight,
            Count
        };

        struct LambertianMatData{
            float3 albedo;
        };

        struct MetalMatData{
            float3 albedo;
            float fuzz;
        };

        struct DielectricMatData{
            float refractiveIndex;
        };

        struct DiffuseLightMatData{
            float3 emitColor;
        };

        static const uint MaterialDataSize[MaterialType::Count] = {
            12, // Lambertian
            16, // Metal
            4,  // Dielectric
            12  // Light
        };
    }

    // 材质的类型及数据偏移
    struct MaterialConstants
    {
        MaterialType::Type type;
        uint matDataOffset;
    };


    namespace ImportanceSampling {
        enum ImportanceSamplingPrimitiveType
        {
            Sphere = 0,
            Quad,
            Count
        };

        // 重要性采样对象
        struct ImportanceSamplingObject
        {
            ImportanceSamplingPrimitiveType type;
            uint primitiveDataOffset;
        };

        static const uint ImportanceSamplingDataSize[ImportanceSamplingPrimitiveType::Count] = {
            16, // Sphere
            100 // Quad
        };

        struct SphereData
        {
            float3 center;
            float radius;
        };

        struct QuadData
        {
            float4x4 worldToObj;
            float3 q;
            float3 u;
            float3 v;
        };
    }
}

#endif