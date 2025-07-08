#define DEBUG
#include <iostream>
#include "Core/GameCore.h"
#include "Graphics/GraphicsCommon.h"
#include "Graphics/RenderContext.h"
#include "Graphics/ShaderCompiler.h"
#include "Graphics/CommandList/GraphicsCommandList.h"
#include "Graphics/Resource/GpuBuffer.h"
#include "Math/Matrix.h"
#include "Math/Random.h"
#include "Math/Transform.h"
#include "Utilities/Utility.h"
#include "ModelLoader.h"
#include "ConstantData.h"
#include "Geometry.h"
#include "Material.h"
#include "CameraController.h"
#include "ImguiManager.h"
#include "Renderer.h"

using namespace DSM;
using namespace DirectX;


class Sandbox : public GameCore::IGameApp
{
public:

    virtual void Startup()override
    {
        g_Renderer.Create();

		ASSERT(ImguiManager::GetInstance().InitImGui(
			g_RenderContext.GetDevice(),
			g_RenderContext.GetSwapChain().GetWindowHandle(),
			1,
			g_RenderContext.GetSwapChain().GetBackBuffer()->GetFormat()));
        
		auto& swapChain = g_RenderContext.GetSwapChain();

        uint64_t width = swapChain.GetWidth();
        uint32_t height = swapChain.GetHeight();

		m_Scissor = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
        m_Camera = std::make_unique<Camera>();
		m_Camera->SetViewPort(0, 0, static_cast<float>(width), static_cast<float>(height));
        float aspect = float(width) / height;
        m_Camera->SetFrustum(DirectX::XM_PIDIV4, aspect == 0 ? 1 : aspect, 0.1f, 1000.0f);
        m_Camera->SetPosition({ 100, 100, -100 });
        //m_Camera->SetPosition({ 0, 0, -10 });
        m_Camera->LookAt({ 0,0,0 }, { 0,1,0 });

        m_CameraController = std::make_unique<CameraController>();
        m_CameraController->InitCamera(m_Camera.get());

        m_SceneTrans.SetScale({ 0.05f, 0.05f, 0.05f });
        MeshConstants meshConstants{};
		meshConstants.m_World = Math::Matrix4::Transpose(m_SceneTrans.GetLocalToWorld());
        meshConstants.m_WorldIT = Math::Matrix4::InverseTranspose(m_SceneTrans.GetLocalToWorld());
		GpuBufferDesc meshConstantsDesc{};
        meshConstantsDesc.m_Size = sizeof(MeshConstants);
		meshConstantsDesc.m_Stride = sizeof(MeshConstants);
		meshConstantsDesc.m_HeapType = D3D12_HEAP_TYPE_UPLOAD;
		m_MeshConstants.Create(L"MeshConstants", meshConstantsDesc, &meshConstants);

        m_Model = LoadModel("Models//Sponza//sponza.gltf");
    }
    virtual void OnResize(std::uint32_t width, std::uint32_t height) override
    {
        // 更行颜色与深度深度缓冲区
		m_Scissor = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
        m_Camera->SetViewPort(0, 0, static_cast<float>(width), static_cast<float>(height));
        float aspect = float(width) / height;
        m_Camera->SetFrustum(DirectX::XM_PIDIV4, aspect == 0 ? 1 : aspect, 0.1f, 1000.0f);

        g_Renderer.OnResize(width, height);
    }
    virtual void Update(float deltaTime) override
    {
        deltaTime = 1.f / 60;

        ImguiManager::GetInstance().Update(deltaTime);

        m_PassConstants.m_ShadowTrans = Math::Matrix4::Identity;
		m_PassConstants.m_TotalTime = 0;
		m_PassConstants.m_DeltaTime = deltaTime;

        m_CameraController->Update(deltaTime);
	}
    virtual void RenderScene(RenderContext& renderContext) override
    {
        auto& swapChain = renderContext.GetSwapChain();


        GraphicsCommandList cmdList{ L"Render Scene" };

        MeshRenderer meshRenderer{};
        meshRenderer.AddRenderTarget(*swapChain.GetBackBuffer(), swapChain.GetBackBufferRTV());
        meshRenderer.SetDepthTexture(g_Renderer.m_DepthTex, g_Renderer.m_DepthTexDSV);
		meshRenderer.SetCamera(*m_Camera);
        meshRenderer.SetScissor(m_Scissor);
        m_Model->Render(meshRenderer, m_MeshConstants, m_SceneTrans);
        meshRenderer.Render(cmdList, m_PassConstants);

        ImguiManager::GetInstance().RenderImGui(cmdList.GetCommandList());

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
    std::unique_ptr<CameraController> m_CameraController{};

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