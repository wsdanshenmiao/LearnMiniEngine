#pragma once
#ifndef __RAYTRACING_HELPER_H__
#define __RAYTRACING_HELPER_H__

#include "Shaders/RayTracingHLSLCompat.h"
#include "Graphics/Resource/GpuBuffer.h"

namespace DSM {
    namespace GeometryType {
        enum Enum {
            Triangle = 0,
            AABB,
            Count
        };
    }

    // 根签名的布局
    namespace GlobalRootSignature {
        namespace RayTracing{
            enum Slot {
                RayTracingOutput = 0,
                AccelerationStructure,
                SceneConstantBuffer,
                Count
            };
        }

        static constexpr uint32_t GlobalRootSignatureCount = RayTracing::Slot::Count;

        namespace StaticSampler {
            enum Slot {
                AnisoWrap = 0,
                Count
            };
        }
    }

    namespace LocalRootSignature {
        namespace Type {
            enum Enum {
                Triangle = 0,
                AABB,
                Count
            };
        }

        namespace Triangle {
            enum Slot {
                Material = 0,
                IndexBuffer,
                NormalBuffer,
                UVBuffer,
                Textures,
                Count
            };
            struct RootArguments {
                MaterialConstantBuffer material;
                D3D12_GPU_VIRTUAL_ADDRESS indexBuffer;
                D3D12_GPU_VIRTUAL_ADDRESS normalBuffer;
                D3D12_GPU_VIRTUAL_ADDRESS uvBuffer;
                D3D12_GPU_DESCRIPTOR_HANDLE textures;   // 6 个 PBR 纹理
            };
        };

        namespace AABB{
            enum Slot{
                Material = 0,
                Textures,
                PrimitiveInstance,
                Count
            };
            // 16 字节对齐
            struct RootArguments{
                MaterialConstantBuffer material;
                D3D12_GPU_DESCRIPTOR_HANDLE textures;   // 6 个 PBR 纹理
                RayTracing::PrimitiveInstanceConstantBuffer primitiveInstance;
            };
        }

        inline uint32_t MaxRootArgumentsSize()
        {
            return (std::max)(sizeof(Triangle::RootArguments), sizeof(AABB::RootArguments));
        }
    }

    namespace IntersectionShaderType{
        enum Enum {
            AnalyticPrimitive = 0,
            Count
        };
    }

    struct AccelerationStructureBuffers
    {
        GpuBuffer scratch;
        GpuBuffer accelerationStructure;
        GpuBuffer instanceDesc;    // Used only for top-level AS
        uint64_t resultDataMaxSizeInBytes;
    };

} // namespace DSM


#endif