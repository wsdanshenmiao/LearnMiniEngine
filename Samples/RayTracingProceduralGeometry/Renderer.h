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


namespace DSM {
    class GraphicsCommandList;
    class ComputeCommandList;
    struct Mesh;
    struct Model;



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
        enum RootBindings
        {
            RayTracingOutput,
            AccelerationStructure,
            SceneConstantBuffer,
            Count
        };

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
        inline static const wchar_t* s_MissShaderName = L"MissShader";
        inline static const wchar_t* s_ClosestHitShaderName = L"ClosestHitShader";
        inline static const wchar_t* s_HitGroupName = L"HitGroup";

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

        // 加速结构
        std::vector<AccelerationStructureBuffers> m_BottomLevelASs{};
        AccelerationStructureBuffers m_TopLevelAS{};

        // 着色器表
        GpuBuffer m_RayGenShaderTable{};
        GpuBuffer m_MissShaderTable{};
        GpuBuffer m_HitShaderTable{};
    };


} // namespace DSM 

#endif