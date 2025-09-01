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
#include "ConstantData.h"
#include "Geometry.h"
#include "CameraController.h"
#include "ImguiManager.h"
#include "Renderer.h"

using namespace DSM;
using namespace DirectX;


class RayTracing : public GameCore::IGameApp
{
public:

    virtual void Startup()override
    {
        g_Renderer.Create();

		ASSERT(ImguiManager::GetInstance().InitImGui(
			g_RenderContext.GetDevice(),
			g_RenderContext.GetSwapChain().GetWindowHandle(),
			2,
			g_RenderContext.GetSwapChain().GetBackBuffer()->GetFormat()));
        
		auto& swapChain = g_RenderContext.GetSwapChain();

        uint64_t width = swapChain.GetWidth();
        uint32_t height = swapChain.GetHeight();

		m_Scissor = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
        m_Camera = std::make_unique<Camera>();
		m_Camera->SetViewPort(0, 0, static_cast<float>(width), static_cast<float>(height));
        float aspect = float(width) / height;
        m_Camera->SetFrustum(DirectX::XM_PIDIV4, aspect == 0 ? 1 : aspect, 0.1f, 1000.0f);
        // m_Camera->SetPosition({ 100, 100, -100 });
        //m_Camera->SetPosition({ 0, 0, -10 });
        // m_Camera->LookAt({ 0,0,0 }, { 0,1,0 });

        m_CameraController = std::make_unique<CameraController>();
        m_CameraController->InitCamera(m_Camera.get());
        m_CameraController->SetMoveSpeed(15);
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

        m_CameraController->Update(deltaTime);
	}
    virtual void RenderScene(RenderContext& renderContext) override
    {
        auto& swapChain = renderContext.GetSwapChain();


        GraphicsCommandList cmdList{ L"Render Scene" };

        auto rect = RECT{0, 0, static_cast<long>(swapChain.GetWidth()), static_cast<long>(swapChain.GetHeight())};
        cmdList.CopyTextureRegion(*swapChain.GetBackBuffer(), 0, 0, 0, g_Renderer.m_RayTracingOutput, rect);
        cmdList.TransitionResource(*swapChain.GetBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET);
        cmdList.TransitionResource(g_Renderer.m_RayTracingOutput, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        cmdList.FlushResourceBarriers();

        cmdList.SetRenderTarget(swapChain.GetBackBufferRTV());
        ImguiManager::GetInstance().RenderImGui(cmdList.GetCommandList());

        cmdList.TransitionResource(*swapChain.GetBackBuffer(), D3D12_RESOURCE_STATE_PRESENT);
        cmdList.ExecuteCommandList();

        swapChain.Present();
    }
    virtual void Cleanup() override
    {
        g_Renderer.Shutdown();
    };

    virtual bool RequiresRaytracingSupport() const override {return true;}

private:
    std::unique_ptr<Camera> m_Camera{};
    std::unique_ptr<CameraController> m_CameraController{};
    D3D12_RECT m_Scissor{};
};

int WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nShowCmd)
{
    RayTracing sandbox{};
    return GameCore::RunApplication(sandbox, 1024, 768, L"DSMEngine", hInstance, nShowCmd);
}