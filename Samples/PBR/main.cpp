#define DEBUG
#include "Core/GameCore.h"
#include "Graphics/GraphicsCommon.h"
#include "Graphics/PipelineState.h"
#include "Graphics/RenderContext.h"
#include "Graphics/RootSignature.h"
#include "Graphics/ShaderCompiler.h"
#include "Graphics/CommandList/GraphicsCommandList.h"
#include "Graphics/Resource/GpuBuffer.h"
#include "Math/Matrix.h"
#include "Math/Random.h"
#include "Math/Transform.h"
#include "Utilities/Utility.h"
#include "Graphics/CommandSignature.h"
#include <iostream>
#include "Renderer/ModelLoader.h"
#include "Renderer/Renderer.h"
#include "Renderer/ConstantData.h"

using namespace DSM;
using namespace DirectX;


class Sandbox : public GameCore::IGameApp
{
public:
    struct VertexPosColor
    {
        static const auto& GetInputLayout()
        {
            static const std::array<D3D12_INPUT_ELEMENT_DESC, 2> inputLayout = {
                D3D12_INPUT_ELEMENT_DESC{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
                D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                D3D12_INPUT_ELEMENT_DESC{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12,
                D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
            };
            return inputLayout;
        };

        DirectX::XMFLOAT3 m_Pos;
        DirectX::XMFLOAT4 m_Color;
    };
    
    struct SubmeshData
    {
        UINT m_IndexCount = 0;
        UINT m_StarIndexLocation = 0;
        INT m_BaseVertexLocation = 0;

        // 单个物体的包围盒
        DirectX::BoundingBox m_Bound;
    };
    
    virtual void Startup()override
    {
        g_Renderer.Create();
        std::uint64_t width = g_Renderer.m_SceneColorTexture.GetWidth();
        std::uint32_t height = g_Renderer.m_SceneColorTexture.GetHeight();

		m_Scissor = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
        m_Camera = std::make_unique<Camera>();
		m_Camera->SetViewPort(0, 0, static_cast<float>(width), static_cast<float>(height));
		m_Camera->SetFrustum(DirectX::XM_PIDIV2, width / height, 0.1f, 1000.0f);

        MeshConstants meshConstants{};
		meshConstants.m_World = m_SceneTrans.GetLocalToWorld();
        meshConstants.m_WorldIT = Math::Matrix4::InverseTranspose(meshConstants.m_World);
		GpuBufferDesc meshConstantsDesc{};
        meshConstantsDesc.m_Size = sizeof(MeshConstants);
		meshConstantsDesc.m_Stride = sizeof(MeshConstants);
		meshConstantsDesc.m_HeapType = D3D12_HEAP_TYPE_DEFAULT;
		m_MeshConstants.Create(L"MeshConstants", meshConstantsDesc, &meshConstants);

        m_Model = LoadModel("Models//Sponza//sponza.gltf");
    }
    virtual void OnResize(std::uint32_t width, std::uint32_t height) override
    {
        // 更行颜色与深度深度缓冲区
        g_Renderer.OnResize(width, height);
		m_Scissor = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
        m_Camera->SetViewPort(0, 0, static_cast<float>(width), static_cast<float>(height));
        m_Camera->SetFrustum(DirectX::XM_PIDIV2, width / height, 0.1f, 1000.0f);
    }
    virtual void Update(float deltaTime) override
    {
        
        m_PassConstants.m_View = m_Camera->GetViewMatrix();
        m_PassConstants.m_ViewInv = Math::Matrix4::Inverse(m_PassConstants.m_View);
        m_PassConstants.m_Proj = m_Camera->GetProjMatrix();
        m_PassConstants.m_ProjInv = Math::Matrix4::Inverse(m_PassConstants.m_Proj);
        m_PassConstants.m_ShadowTrans = Math::Matrix4::Identity;
		Math::Vector3 cameraPos = m_Camera->GetTransform().GetPosition();
		m_PassConstants.m_CameraPos[0] = cameraPos.GetX();
		m_PassConstants.m_CameraPos[1] = cameraPos.GetY();
		m_PassConstants.m_CameraPos[2] = cameraPos.GetZ();
		m_PassConstants.m_TotalTime = 0;
		m_PassConstants.m_DeltaTime = deltaTime;
	}
    virtual void RenderScene(RenderContext& renderContext) override
    {
        auto& swapChain = renderContext.GetSwapChain();
        
        GraphicsCommandList cmdList{L"Render Scene"};

        cmdList.TransitionResource(g_Renderer.m_SceneDepthTexture, D3D12_RESOURCE_STATE_DEPTH_WRITE);
        cmdList.ClearDepth(g_Renderer.m_SceneDepthDSV);

        MeshSorter sorter{ MeshSorter::kDefault };
        sorter.SetCamera(*m_Camera);
        sorter.SetScissor(m_Scissor);
        sorter.SetDepthStencilTarget(
            g_Renderer.m_SceneDepthTexture, 
            g_Renderer.m_SceneDepthDSV, 
            g_Renderer.m_SceneDepthDSVReadOnly);
        sorter.AddRenderTarget(g_Renderer.m_SceneColorTexture);

        m_Model->Render(sorter, m_MeshConstants, m_SceneTrans);

        sorter.Sort();
		//sorter.Render(MeshSorter::kZPass, cmdList, m_PassConstants);


        //for (const auto& mesh : m_Model->m_Meshes) {
        //    D3D12_VERTEX_BUFFER_VIEW vbView[] = {
        //        mesh->m_PositionStream,
        //        mesh->m_UVStream
        //    };
        //    cmdList.SetVertexBuffers(0, vbView);
        //    cmdList.SetIndexBuffer(mesh->m_IndexBufferViews);

        //    for (const auto& [name, submesh] : mesh->m_SubMeshes) {
        //        cmdList.DrawIndexed(submesh.m_IndexCount, submesh.m_IndexOffset, submesh.m_VertexOffset);
        //    }
        //}

		cmdList.CopyResource(*swapChain.GetBackBuffer(), g_Renderer.m_SceneColorTexture);

        cmdList.TransitionResource(*swapChain.GetBackBuffer(), D3D12_RESOURCE_STATE_PRESENT);

        cmdList.ExecuteCommandList();

        swapChain.Present();
    }
    virtual void Cleanup() override
    {
        g_Renderer.Shutdown();
    };

private:
    std::unique_ptr<Camera> m_Camera{};

    D3D12_RECT m_Scissor{};

    Transform m_SceneTrans{};
    GpuBuffer m_MeshConstants{};

    PassConstants m_PassConstants{};

    std::shared_ptr<Model> m_Model{};
};

int WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nShowCmd)
{
    Sandbox sandbox{};
    return GameCore::RunApplication(sandbox, 1024, 768, L"DSMEngine", hInstance, nShowCmd);
}