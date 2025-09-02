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
#include "ConstantData.h"
#include "Core/Camera.h"
#include "Shaders/RayTracingHLSLCompat.h"


namespace DSM {
    class GraphicsCommandList;
    struct Mesh;
    
    class Renderer : public Singleton<Renderer>
    {
    public:
        enum RootBindings
        {
            RayTracingOutput,
            AccelerationStructure,
            Count
        };

        void Create();
        void Shutdown();

        void OnResize(uint32_t width, uint32_t height);

    private:
        void CreateResource(uint32_t width, uint32_t height);
        void CreateStateObject();
        void CreateAccelerationStructure();
        void CreateShaderTable();

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

        // 几何数据
        GpuBuffer m_VertexBuffer{};
        GpuBuffer m_IndexBuffer{};

        // 加速结构
        GpuBuffer m_BottomLevelAS{};
        GpuBuffer m_TopLevelAS{};

        // 着色器表
        GpuBuffer m_RayGenShaderTable{};
        GpuBuffer m_MissShaderTable{};
        GpuBuffer m_HitShaderTable{};

        RayGenConstantBuffer m_RayGenCB{};

        DescriptorHeap m_TextureHeap;
    };
#define g_Renderer (Renderer::GetInstance())

} // namespace DSM 

#endif