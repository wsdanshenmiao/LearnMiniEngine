#pragma once
#ifndef __RENDERER_H__
#define __RENDERER_H__

#include "Mesh.h"
#include "Core/Camera.h"
#include "Graphics/DescriptorHeap.h"
#include "Graphics/RootSignature.h"
#include "Graphics/PipelineState.h"
#include "Graphics/Resource/Texture.h"
#include "Utilities/Singleton.h"
#include "Graphics/ShaderCompiler.h"

namespace DSM {
    struct PassConstants;
}

namespace DSM {
    class GraphicsCommandList;
}

namespace DSM {
    class Camera;
    class Texture;
    struct Mesh;
    
    class Renderer : public Singleton<Renderer>
    {
    public:
        enum RootBindings
        {
            kMeshConstants,
            kMaterialConstants,
            kPassConstants,
            kMaterialSRVs,
            kCommonSRVs,

            kNumRootBindings
        };
        
    public:
        void Create();
        void Shutdown();

        std::uint16_t GetPSO(std::uint16_t psoFlags);
        void OnResize(std::uint32_t width, std::uint32_t height);
        void ResizeShadowMap(std::uint32_t width, std::uint32_t height);

    private:
        friend class Singleton<Renderer>;
        Renderer();
        ~Renderer() { Shutdown(); }

    public:
        // 是否使用PreZPass
        bool m_SeparateZPass = true;
        bool m_Initialized = false;

        Texture m_SceneColorTexture{};
        DescriptorHandle m_SceneColorSRV{};
        DescriptorHandle m_SceneColorRTV{};
        
        Texture m_SceneDepthTexture{};
        DescriptorHandle m_SceneDepthSRV{};
        DescriptorHandle m_SceneDepthDSV{};
        DescriptorHandle m_SceneDepthDSVReadOnly{};
        
        Texture m_ShadowMap{};
        DescriptorHandle m_ShadowMapSRV{};
        DescriptorHandle m_ShadowMapDSV{};
        DescriptorHandle m_ShadowMapDSVReadOnly{};
        
        DescriptorHandle m_CommonTexture{};
        
        DescriptorHeap m_TextureHeap;
        RootSignature m_RootSignature;

        std::unique_ptr<ShaderByteCode> m_LitVS;
        std::unique_ptr<ShaderByteCode> m_LitPS;
        std::unique_ptr<ShaderByteCode>m_LitNoTangentVS;
		std::unique_ptr<ShaderByteCode> m_LitNoTangentPS;

        
        std::vector<GraphicsPSO> m_PSOs;
        
    private:
        static constexpr std::uint32_t sm_MaxTextureSize = 4096;
        static constexpr std::uint32_t sm_MaxSamplerSize = 2048;

        GraphicsPSO m_DefaultPSO;
        GraphicsPSO m_SkyboxPSO;
    };
#define g_Renderer (Renderer::GetInstance())


    // 根据网格的属性来排序并渲染
    class MeshSorter
    {
    public:
        enum BatchType{ kDefault, kShadows, kNumBatchTypes };
        enum DrawPass{ kZPass, kOpaque, kTransparent, kNumDrawPasses };

    public:
        MeshSorter(BatchType batchType) : m_BatchType(batchType) {}
        
        Math::Matrix4 GetViewMatrix() const noexcept { return m_Camera->GetViewMatrix(); }
        const DirectX::BoundingFrustum& GetViewFrustum() const noexcept { return m_Frustum; }
        const DirectX::BoundingFrustum GetWorldFrustum() const noexcept
        {
            auto ret = m_Frustum;
            m_Frustum.Transform(ret, m_Camera->GetTransform().GetLocalToWorld());
            return ret;
        }
        
        void SetCamera(const Camera& camera) noexcept
        {
            m_Camera = &camera;
            DirectX::BoundingFrustum::CreateFromMatrix(m_Frustum, m_Camera->GetProjMatrix());
        }
        void SetScissor(const D3D12_RECT& rect) noexcept { m_Scissor = rect; }
        void SetDepthStencilTarget(Texture& depthTarget, DescriptorHandle dsv, DescriptorHandle dsvReadOnly) noexcept
        {
            m_DepthTex = &depthTarget;
            m_DSV = dsv;
            m_DSVReadOnly = dsvReadOnly;
        }

        void AddRenderTarget(Texture& renderTarget)
        {
            ASSERT(m_NumRTVs <= 8);
            m_RTVs[m_NumRTVs++] = &renderTarget;
        }

        void AddMesh(const Mesh& mesh,
            float distance,
            D3D12_GPU_VIRTUAL_ADDRESS meshCBV,
            D3D12_GPU_VIRTUAL_ADDRESS matCBV);
        void Sort();
        void Render(DrawPass pass, GraphicsCommandList& cmdList, PassConstants& passConstants);

    private:
        // 用于排序的键值
        struct SortKey
        {
            union
            {
                std::uint64_t m_Value;
                // 优先将同一趟 Pass 的放在一起，然后是距离，再是同一个 PSO
                struct
                {
                    std::uint64_t m_ObjIndex : 16;
                    std::uint64_t m_PSOIndex : 12;
                    std::uint64_t m_Key : 32;
                    std::uint64_t m_PassId : 4;
                };
            };
        };
        
        struct SortObject
        {
            const Mesh* m_Mesh;
            D3D12_GPU_VIRTUAL_ADDRESS m_MeshCBV;
            D3D12_GPU_VIRTUAL_ADDRESS m_MaterialCBV;
        };

        std::vector<SortObject> m_SortObjects{};
        std::vector<std::uint64_t> m_SortKey{};
        BatchType m_BatchType{};
        std::array<std::uint32_t, kNumDrawPasses> m_PassCounts{};
        DrawPass m_CurrPass = kZPass;
        std::uint32_t m_CurrDraw = 0;

        const Camera* m_Camera = nullptr;
        D3D12_RECT m_Scissor{};
        std::uint32_t m_NumRTVs = 0;
        std::array<Texture*, 8> m_RTVs{};
        Texture* m_DepthTex = nullptr;
        DescriptorHandle m_DSV{};
        DescriptorHandle m_DSVReadOnly{};
        DirectX::BoundingFrustum m_Frustum{};
    };

}

#endif
