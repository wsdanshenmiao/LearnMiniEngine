#ifndef __RAYTRACING_HLSL_COMPAT_H__
#define __RAYTRACING_HLSL_COMPAT_H__

#if defined(__cplusplus)
using float2 = std::pair<float, float>;
using float3 = DSM::Math::Vector3;
using float4 = DSM::Math::Vector4;
using float3x3 = DSM::Math::Matrix3;
using float4x4 = DSM::Math::Matrix4;
using uint = uint32_t;
#endif


#ifndef MAX_TRACE_RECURSION_DEPTH
#define MAX_TRACE_RECURSION_DEPTH 3
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
        uint seed;
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
        float4 focusDistDefocusAngle;
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

}


#endif