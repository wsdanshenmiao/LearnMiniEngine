#pragma once
#ifndef __RENDERER_H__
#define __RENDERER_H__

#include "Utilities/Singleton.h"
#include "Graphics/Resource/Texture.h"
#include "Graphics/Resource/GpuBuffer.h"
#include "Graphics/DescriptorHeap.h"
#include "Graphics/RootSignature.h"
#include "Graphics/PipelineState.h"
#include "Graphics/ShaderCompiler.h"
#include "Core/Camera.h"
#include "Shaders/RayTracingHLSLCompat.h"
#include "Light.h"


namespace DSM {
    class GraphicsCommandList;
    class ComputeCommandList;
    struct Mesh;
    struct Model;


    namespace GeometryType {
        enum Enum {
            Triangle = 0,
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

        namespace Light{
            enum Slot {
                LightData = RayTracing::Slot::Count,
                DirectionalLightDatas,
                Count
            };
        }

        static constexpr uint32_t GlobalRootSignatureCount = Light::Slot::Count;

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

        inline uint32_t MaxRootArgumentsSize()
        {
            return sizeof(Triangle::RootArguments);
        }
    }

    struct AccelerationStructureBuffers
    {
        GpuBuffer scratch;
        GpuBuffer accelerationStructure;
        GpuBuffer instanceDesc;    // Used only for top-level AS
        uint64_t resultDataMaxSizeInBytes;
    };

    
    class Renderer : public Singleton<Renderer>
    {
    public:
        void Create();
        void Shutdown();

        void OnResize(uint32_t width, uint32_t height);

    private:
        void CreateResource(uint32_t width, uint32_t height);
        void CreateStateObject();

    private:
        friend class Singleton<Renderer>;
        Renderer();
        virtual ~Renderer() { Shutdown(); }

    public:
        inline static const wchar_t* s_RayGenShaderName = L"RaygenShader";
        inline static std::array<const wchar_t*, RayTracing::RayType::Count> s_MissShaderName = { 
            L"MissShader",
            L"MissShader_Shadow"
        };
        inline static std::array<const wchar_t*, GeometryType::Count> s_ClosestHitShaderName = { 
            L"ClosestHitShader_Triangle"
        };
        inline static std::array<const wchar_t*, RayTracing::RayType::Count> s_HitGroupName_Triangle = { 
            L"HitGroup_Triangle",
            L"HitGroup_Triangle_Shadow" 
        };

        static constexpr uint32_t s_MaxTraceRecursionDepth = 3;

        bool m_Initialized = false;

        Texture m_RayTracingOutput{};
        DescriptorHandle m_OutputUAV{};

        Microsoft::WRL::ComPtr<ID3D12StateObject> m_RayTracingStateObject{};
        // 生成光线时使用的根签名
        RootSignature m_LocalRootSig;
        // 全局根签名
        RootSignature m_GlobalRootSig;
        
        DescriptorHeap m_TextureHeap;
    };
#define g_Renderer (Renderer::GetInstance())


    class RayTracer
    {
    public:
        RayTracer();

        void SetCamera(const Camera* camera) { m_Camera = camera; }
        void TraceRays(ComputeCommandList& cmdList);

        void AddModel(std::shared_ptr<Model> model);
        void AddLight(const Light& light);

    private:
        void CreateAccelerationStructure();
        void CreateShaderTable();

    public:
        static constexpr size_t sm_MaxDirLightCount = 4;

    private:
        const Camera* m_Camera;

        std::vector<std::shared_ptr<Model>> m_Models;

        // 加速结构
        std::vector<AccelerationStructureBuffers> m_BottomLevelASs{};
        AccelerationStructureBuffers m_TopLevelAS{};

        // 着色器表
        GpuBuffer m_RayGenShaderTable{};
        GpuBuffer m_MissShaderTable{};
        GpuBuffer m_HitShaderTable{};


        // 光照信息
        GpuBuffer m_LightDataBuffer;
        GpuBuffer m_DirLightDataBuffer;
        std::vector<DirectionalLightData> m_DirLights{};
    };


} // namespace DSM 

#endif