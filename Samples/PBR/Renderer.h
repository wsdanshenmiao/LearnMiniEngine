#pragma once
#ifndef __RENDERER_H__
#define __RENDERER_H__

#include "Utilities/Singleton.h"
#include "Graphics/Resource/Texture.h"
#include "Graphics/DescriptorHeap.h"
#include "Graphics/RootSignature.h"
#include "Graphics/PipelineState.h"
#include "Graphics/ShaderCompiler.h"
#include "ConstantData.h"
#include "Core/Camera.h"


namespace DSM {
    class GraphicsCommandList;
    struct Mesh;
    
    class Renderer : public Singleton<Renderer>
    {
    public:
        // 跟参数的绑定槽
        enum RootBindings
        {
            kMeshConstants = 0,
            kMaterialConstants,
            kPassConstants,
            kMaterialSRVs,
            kNumRootBindings
        };

    public:
        void Create();
        void Shutdown();

        void OnResize(uint32_t width, uint32_t height);

        uint16_t GetPSO(uint16_t psoFlags);

    private:
        void CreateResource(uint32_t width, uint32_t height);

    private:
        friend class Singleton<Renderer>;
        Renderer();
        virtual ~Renderer() { Shutdown(); }

    public:
        bool m_Initialized = false;

        Texture m_DepthTex;
        DescriptorHandle m_DepthTexDSV;
        DescriptorHandle m_DepthTexSRV;

        Texture m_ColorTex;
        DescriptorHandle m_ColorTexRTV;
        DescriptorHandle m_ColorTexSRV;

        RootSignature m_CommonRootSig;
        GraphicsPSO m_DefaultPSO;
        std::vector<GraphicsPSO> m_PSOs;

        DescriptorHeap m_TextureHeap;
        // 测试封装的描述符堆
        //TODO:测试后删除
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_TestTextureHeap;
        uint64_t m_HeapOffset = 0;

        std::unique_ptr<ShaderByteCode> m_VS;
        std::unique_ptr<ShaderByteCode> m_VSUseTangent;
        std::unique_ptr<ShaderByteCode> m_PS;
        std::unique_ptr<ShaderByteCode> m_PSUseTangent;   
    };
#define g_Renderer (Renderer::GetInstance())

    class MeshRenderer
    {
    private:
        struct SortObject
        {
            const Mesh* m_Mesh;
            D3D12_GPU_VIRTUAL_ADDRESS m_MeshCBV;
            D3D12_GPU_VIRTUAL_ADDRESS m_MaterialCBV;
        };

        struct SortKey
        {
            union
            {
                uint64_t m_Value;
                struct
                {
                    uint64_t m_ObjIndex : 16;
                    uint64_t m_PSOIndex : 12;
                    uint64_t m_Key : 36;
                };
            }; 
            std::strong_ordering operator<=>(const SortKey& other) const
            {
                return m_Value <=> other.m_Value;
            }
        };

    public:
        void Render(GraphicsCommandList& cmdList, PassConstants& passConstants);

        Math::Matrix4 GetViewMatrix() const { return m_RenderCamera->GetViewMatrix(); }
        DirectX::BoundingFrustum GetViewFrustum() const;

        void AddRenderTarget(Texture& renderTarget, DescriptorHandle rtv);
        void SetDepthTexture(Texture& depthTex, DescriptorHandle dsv);

        void AddMesh(const Mesh& mesh, float distance, 
            D3D12_GPU_VIRTUAL_ADDRESS meshCBV, 
            D3D12_GPU_VIRTUAL_ADDRESS materialCBV);

        void SetCamera(const Camera& camera) { m_RenderCamera = &camera; }
        void SetScissor(const D3D12_RECT& scissor) { m_Scissor = scissor; }

    private:
        uint32_t m_NumRenderTargets = 0;
        std::array<Texture*, 8> m_RenderTarget;
        std::array<DescriptorHandle, 8> m_RenderTargetRTV;

        Texture* m_DepthTex;
        DescriptorHandle m_DepthTexDSV;
        
        std::map<SortKey, SortObject> m_SortObjects;

        const Camera* m_RenderCamera;
        D3D12_RECT m_Scissor{};
    };

} // namespace DSM 

#endif