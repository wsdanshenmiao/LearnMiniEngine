#define DEBUG
#include <iostream>
#include "Core/GameCore.h"
#include "Graphics/GraphicsCommon.h"
#include "Graphics/RenderContext.h"
#include "Graphics/ShaderCompiler.h"
#include "Graphics/CommandList/GraphicsCommandList.h"
#include "Graphics/CommandList/ComputeCommandList.h"
#include "Graphics/Resource/GpuBuffer.h"
#include "Math/Matrix.h"
#include "Math/Random.h"
#include "Math/Transform.h"
#include "Utilities/Utility.h"
#include "Geometry.h"
#include "CameraController.h"
#include "ImguiManager.h"
#include "Renderer.h"
#include "ModelLoader.h"
#include "ProceduralGeometry.h"

using namespace DSM;
using namespace DirectX;


class RayTracingApp : public GameCore::IGameApp
{
public:

    virtual void Startup()override
    {
        g_Renderer.Create();
        
		auto& swapChain = g_RenderContext.GetSwapChain();

		ASSERT(ImguiManager::GetInstance().InitImGui(
			g_RenderContext.GetDevice(),
			g_RenderContext.GetSwapChain().GetWindowHandle(),
			2,
			swapChain.GetBackBufferFormat()));

        uint64_t width = swapChain.GetWidth();
        uint32_t height = swapChain.GetHeight();

		m_Scissor = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
        m_Camera = std::make_unique<Camera>();
		m_Camera->SetViewPort(0, 0, static_cast<float>(width), static_cast<float>(height));
        float aspect = float(width) / height;
        m_Camera->SetFrustum(DirectX::XM_PIDIV4, aspect == 0 ? 1 : aspect, 1.f, 1000.0f);
        m_Camera->SetPosition({ 13,2,3 });
        m_Camera->LookAt({ 0,0,0 }, { 0,1,0 });

        m_RayTracer = std::make_unique<RayTracer>();
        m_RayTracer->SetCamera(m_Camera.get());

        m_CameraController = std::make_unique<CameraController>();
        m_CameraController->InitCamera(m_Camera.get());
        m_CameraController->SetMoveSpeed(1);

        auto lihuazou = LoadModel("Models/AB/AliceADefault/AliceADefault.fbx");
        auto sponza = LoadModel("Models/Sponza/pbr/sponza2.gltf");
        auto plane = LoadModelFromeGeometry("Plane", Geometry::GeometryGenerator::CreateGrid(60, 60, 2, 2));
        plane->transform.SetPosition({ 0, -2, 0 });

        m_RayTracer->AddModel(lihuazou);
        m_RayTracer->AddModel(sponza);
        m_RayTracer->AddModel(plane);
        

        std::mt19937 engine{std::random_device{}()};
        std::uniform_real_distribution<float> dist(0.f, 1.f);
        auto randomFloat = [&engine, &dist]() {
            return dist(engine);
        };

        ProceduralGeometryDesc sphereDesc{};
        sphereDesc.type = RayTracing::AnalyticPrimitive::PrimitiveType::Sphere;
        sphereDesc.transform.SetPosition({ 0,-1000,0 });
        sphereDesc.transform.SetScale({ 1000,1000,1000 });
        auto material = std::make_shared<LambertianMaterial>();
        material->matData.albedo = Math::Vector3{0.5, 0.5, 0.5};
        material->materialType = RayTracing::MaterialType::Lambertian;
        sphereDesc.material = material;
        m_RayTracer->AddProceduralGeometry(sphereDesc);

        for (int a = -11; a < 11; a++) {
            for (int b = -11; b < 11; b++) {
                auto choose_mat = randomFloat();
                auto desc = sphereDesc;
                Math::Vector3 center{a + 0.9f * randomFloat(), 0.2f, b + 0.9f * randomFloat()};
                desc.transform.SetPosition(center);
                desc.transform.SetScale({ 0.2f, 0.2f, 0.2f });

                if (Math::Vector3::Length(center - Math::Vector3{4, 0.2, 0}) > 0.9f) {
                    if (choose_mat < 0.8) {
                        auto material = std::make_shared<LambertianMaterial>();
                        material->materialType = RayTracing::MaterialType::Lambertian;

                        Math::Vector3 color = Math::Vector3{randomFloat(), randomFloat(), randomFloat()};
                        color *= Math::Vector3{randomFloat(), randomFloat(), randomFloat()};
                        material->matData.albedo = color;
                        desc.material = material;
                    } else if (choose_mat < 0.95) {
                        auto material = std::make_shared<MetalMaterial>();
                        material->materialType = RayTracing::MaterialType::Metal;

                        Math::Vector3 color = Math::Vector3{randomFloat(), randomFloat(), randomFloat()};
                        color = (color + Math::Vector3{1.0f, 1.0f, 1.0f}) * 0.5f;
                        material->matData.albedo = color;
                        material->matData.fuzz = 0.5f * randomFloat();
                        desc.material = material;
                    } else {
                        auto material = std::make_shared<DielectricMaterial>();
                        material->materialType = RayTracing::MaterialType::Dielectric;

                        material->matData.refractiveIndex = 1.5f;
                        desc.material = material;
                    }
                    m_RayTracer->AddProceduralGeometry(desc);
                }
            }
        }

        sphereDesc.transform.SetPosition({ 0, 1, 0 });
        sphereDesc.transform.SetScale({ 1.0f, 1.0f, 1.0f });
        m_RayTracer->AddProceduralGeometry(sphereDesc);

        sphereDesc.transform.SetPosition({ -4, 1, 0 });
        m_RayTracer->AddProceduralGeometry(sphereDesc);

        sphereDesc.transform.SetPosition({ 4, 1, 0 });
        m_RayTracer->AddProceduralGeometry(sphereDesc);
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
        uint32_t width = swapChain.GetWidth();
        uint32_t height = swapChain.GetHeight();

        GraphicsCommandList cmdList{ L"Render Scene" };

        m_RayTracer->TraceRays(cmdList.GetComputeCommandList());

        assert(width == g_Renderer.m_RayTracingOutput.GetWidth() && height == g_Renderer.m_RayTracingOutput.GetHeight());
        auto rect = RECT{0, 0, static_cast<long>(width), static_cast<long>(height)};
        cmdList.CopyTextureRegion(*swapChain.GetBackBuffer(), 0, 0, 0, g_Renderer.m_RayTracingOutput, rect);
        cmdList.TransitionResource(*swapChain.GetBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET);
        cmdList.TransitionResource(g_Renderer.m_RayTracingOutput, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        cmdList.FlushResourceBarriers();

        cmdList.SetRenderTarget(swapChain.GetBackBufferRTV());
        ImguiManager::GetInstance().RenderImGui(cmdList.GetCommandList());

        cmdList.TransitionResource(*swapChain.GetBackBuffer(), D3D12_RESOURCE_STATE_PRESENT);
        cmdList.ExecuteCommandList();
        ASSERT_SUCCEEDED(g_RenderContext.GetDevice()->GetDeviceRemovedReason());
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

    std::unique_ptr<RayTracer> m_RayTracer{};
};

int WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nShowCmd)
{
    try {
        RayTracingApp sandbox{};
        return GameCore::RunApplication(sandbox, 1024, 768, "DSMEngine", hInstance, nShowCmd);
    }
    catch(const std::exception& e) {
        std::cerr << e.what() << '\n';
        return -1;
    }
    return 0;
}