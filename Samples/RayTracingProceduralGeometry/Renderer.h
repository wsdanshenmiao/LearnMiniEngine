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
#include "Graphics/CommandList/ComputeCommandList.h"
#include "Core/Camera.h"
#include "Shaders/RayTracingHLSLCompat.h"


namespace DSM {
    struct Model;

    namespace GeometryType {
        enum Enum {
            Triangle = 0,
            Count
        };
    }

    // 根签名的布局
    namespace GlobalRootSignature {
        enum Slot {
            RayTracingOutput = 0,
            AccelerationStructure,
            SceneConstantBuffer,
            Count
        };
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
                RayTracing::Material material;
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
            L"MissShader" 
        };
        inline static std::array<const wchar_t*, GeometryType::Count> s_ClosestHitShaderName_Triangle = { 
            L"ClosestHitShader_Triangle" 
        };
        inline static std::array<const wchar_t*, RayTracing::RayType::Count> s_HitGroupName_Triangle = { 
            L"HitGroup_Triangle"
        };

        static constexpr uint32_t s_MaxRecursionDepth = 5;

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
        void SetCamera(const Camera* camera) { m_Camera = camera; }
        void TraceRays(ComputeCommandList& cmdList);

        void AddModel(std::shared_ptr<Model> model);

    private:
        void CreateAccelerationStructure();
        void CreateShaderTable();

    private:
        const Camera* m_Camera;

        std::vector<std::shared_ptr<Model>> m_Models;

        std::vector<AccelerationStructureBuffers> m_BottomLevelASs;
        AccelerationStructureBuffers m_TopLevelAS;

        // 所有的 Shader Table
        GpuBuffer m_RayGenShaderTable;
        GpuBuffer m_HitGroupShaderTable;
        GpuBuffer m_MissShaderTable;

        GpuBuffer m_IndexBuffer{};
        GpuBuffer m_VertexBuffer{};
    };

} // namespace DSM 

#endif