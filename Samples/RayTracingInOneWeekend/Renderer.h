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
#include "RayTracingHelper.h"
#include "ProceduralGeometry.h"
#include "Shaders/RayTracingHLSLCompat.h"


namespace DSM {
    class GraphicsCommandList;
    class ComputeCommandList;
    struct Mesh;
    struct Model;



    
    class Renderer : public Singleton<Renderer>
    {
    public:
        void Create();
        void Shutdown();

        void OnResize(uint32_t width, uint32_t height);
        void CreateStateObject();

    private:
        void CreateResource(uint32_t width, uint32_t height);

    private:
        friend class Singleton<Renderer>;
        Renderer();
        virtual ~Renderer() { Shutdown(); }

    public:
        inline static const wchar_t* s_RayGenShaderName = L"RaygenShader";
        inline static std::array<const wchar_t*, RayTracing::RayType::Count> s_MissShaderName = { 
            L"MissShader"
        };
        inline static std::array<const wchar_t*, GeometryType::Count> s_ClosestHitShaderName = { 
            L"ClosestHitShader_Triangle",
            L"ClosestHitShader_AABB"
        };
        inline static std::array<const wchar_t*, RayTracing::RayType::Count> s_HitGroupName_Triangle = { 
            L"HitGroup_Triangle"
        };
        inline static std::array<const wchar_t*, RayTracing::RayType::Count> s_HitGroupName_AABB = { 
            L"HitGroup_AABB"
        };
        inline static std::array<const wchar_t*, IntersectionShaderType::Count> s_IntersectionShaderName = { 
            L"IntersectionShader_AnalyticPrimitive"
        };

        bool m_Initialized = false;

        Texture m_RayTracingOutput{};
        DescriptorHandle m_OutputUAV{};

        Microsoft::WRL::ComPtr<ID3D12StateObject> m_RayTracingStateObject{};
        // 生成光线时使用的根签名
        std::array<RootSignature, LocalRootSignature::Type::Count> m_LocalRootSigs;
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
        void AddProceduralGeometry(const ProceduralGeometryDesc& desc);
        void AddProceduralGeometries(std::span<ProceduralGeometryDesc> descs);

    private:
        void CreateAccelerationStructure();
        void CreateShaderTable();

    public:
        static constexpr size_t sm_MaxDirLightCount = 4;

    private:
        const Camera* m_Camera;
        bool m_HasChanged = false;

        std::vector<std::shared_ptr<Model>> m_Models;
        std::unique_ptr<ProceduralGeometryManager> m_ProceduralGeometryManager;

        // 加速结构
        std::vector<AccelerationStructureBuffers> m_BottomLevelASs{};
        AccelerationStructureBuffers m_TopLevelAS{};

        // 着色器表
        GpuBuffer m_RayGenShaderTable{};
        GpuBuffer m_MissShaderTable{};
        GpuBuffer m_HitShaderTable{};

        GpuBuffer m_MaterialBuffer{};
    };


} // namespace DSM 

#endif