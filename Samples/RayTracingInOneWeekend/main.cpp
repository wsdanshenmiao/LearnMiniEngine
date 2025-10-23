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
#include <numbers>

using namespace DSM;
using namespace DirectX;

struct Scene
{
    static std::vector<ProceduralGeometryDesc> CreateSphereScene()
    {
        std::mt19937 engine{std::random_device{}()};
        std::uniform_real_distribution<float> dist(0.f, 1.f);
        auto randomFloat = [&engine, &dist]() {
            return dist(engine);
        };

        std::vector<ProceduralGeometryDesc> proceduralGeometries{};
        ProceduralGeometryDesc landDesc{};
        landDesc.type = RayTracing::AnalyticPrimitive::PrimitiveType::Sphere;
        landDesc.transform.SetPosition({ 0,-1000,0 });
        landDesc.transform.SetScale({ 1000,1000,1000 });
        auto lamMaterial = std::make_shared<LambertianMaterial>();
        lamMaterial->matData.albedo = Math::Vector3{0.5, 0.5, 0.5};
        lamMaterial->materialType = RayTracing::MaterialType::Lambertian;
        landDesc.material = lamMaterial;
        proceduralGeometries.push_back(landDesc);

        for (int a = -11; a < 11; a++) {
            for (int b = -11; b < 11; b++) {
                auto choose_mat = randomFloat();
                auto& sphereDesc = proceduralGeometries.emplace_back();
                sphereDesc = landDesc;
                Math::Vector3 center{a + 0.9f * randomFloat(), 0.2f, b + 0.9f * randomFloat()};
                sphereDesc.transform.SetPosition(center);
                sphereDesc.transform.SetScale({ 0.2f, 0.2f, 0.2f });

                if (Math::Vector3::Length(center - Math::Vector3{4, 0.2, 0}) > 0.9f) {
                    if (choose_mat < 0.8) {
                        auto material = std::make_shared<LambertianMaterial>();
                        material->materialType = RayTracing::MaterialType::Lambertian;

                        Math::Vector3 color = Math::Vector3{randomFloat(), randomFloat(), randomFloat()};
                        color *= Math::Vector3{randomFloat(), randomFloat(), randomFloat()};
                        material->matData.albedo = color;
                        sphereDesc.material = material;
                    } else if (choose_mat < 0.95) {
                        auto material = std::make_shared<MetalMaterial>();
                        material->materialType = RayTracing::MaterialType::Metal;

                        Math::Vector3 color = Math::Vector3{randomFloat(), randomFloat(), randomFloat()};
                        color = (color + Math::Vector3{1.0f, 1.0f, 1.0f}) * 0.5f;
                        material->matData.albedo = color;
                        material->matData.fuzz = 0.5f * randomFloat();
                        sphereDesc.material = material;
                    } else {
                        auto material = std::make_shared<DielectricMaterial>();
                        material->materialType = RayTracing::MaterialType::Dielectric;

                        material->matData.refractiveIndex = 1.5f;
                        sphereDesc.material = material;
                    }
                }
            }
        }

        auto& sphereDesc0 = proceduralGeometries.emplace_back();
        sphereDesc0.type = RayTracing::AnalyticPrimitive::PrimitiveType::Sphere;
        sphereDesc0.transform.SetPosition({ 0, 1, 0 });
        sphereDesc0.transform.SetScale({ 1.0f, 1.0f, 1.0f });
        auto dieMat = std::make_shared<DielectricMaterial>();
        dieMat->matData.refractiveIndex = 1.5f;
        sphereDesc0.material = dieMat;

        auto& sphereDesc1 = proceduralGeometries.emplace_back();
        sphereDesc1.type = RayTracing::AnalyticPrimitive::PrimitiveType::Sphere;
        sphereDesc1.transform.SetPosition({ -4, 1, 0 });
        auto lamMat0 = std::make_shared<LambertianMaterial>();
        lamMat0->matData.albedo = Math::Vector3{0.4, 0.2, 0.1};
        sphereDesc1.material = lamMat0;

        auto& sphereDesc2 = proceduralGeometries.emplace_back();
        sphereDesc2.type = RayTracing::AnalyticPrimitive::PrimitiveType::Sphere;
        sphereDesc2.transform.SetPosition({ 4, 1, 0 });
        auto metalMat = std::make_shared<MetalMaterial>();
        metalMat->matData.albedo = Math::Vector3{0.7, 0.6, 0.5};
        metalMat->matData.fuzz = 0;
        sphereDesc2.material = metalMat;

        return proceduralGeometries;
    }

    static std::vector<ProceduralGeometryDesc> CreateCornellBox()
    {
        std::vector<ProceduralGeometryDesc> world;
        float scale = 0.001f;

        auto red = std::make_shared<LambertianMaterial>();
        red->matData.albedo = Math::Vector3{.65, .05, .05};
        auto green = std::make_shared<LambertianMaterial>();
        green->matData.albedo = Math::Vector3{0.12, 0.45, 0.15};
        auto white = std::make_shared<LambertianMaterial>();
        white->matData.albedo = Math::Vector3{.73, .73, .73};

        auto light = std::make_shared<DiffuseLightMaterial>();
        light->matData.emitColor = Math::Vector3{15, 15, 15};

        auto addQuad = [&world, scale](
            const Math::Vector3& p0, 
            float scaleU, float scaleV, 
            const Math::Quaternion& rotation, 
            std::shared_ptr<RTMaterial> material) {
            ProceduralGeometryDesc quad{};
            quad.type = RayTracing::AnalyticPrimitive::PrimitiveType::Quad;
            quad.transform.SetPosition(p0 * scale);
            quad.transform.SetScale(Math::Vector3{ scaleU, scaleV, 1.0f } * scale);
            quad.transform.SetRotation(rotation);
            quad.material = material;
            world.push_back(quad);
        };
        float pi = (float)std::numbers::pi;
        addQuad(Math::Vector3{-555,0,0}, 555, 555, Math::Quaternion{0, pi / 2, 0}, green);
        addQuad(Math::Vector3{555,0,0}, 555, 555, Math::Quaternion{0, -pi / 2, 0}, red);
        addQuad(Math::Vector3{343 - 555 / 2, 554, 332 - 555 / 2}, -130, -105, Math::Quaternion{pi / 2, 0, 0}, light);
        addQuad(Math::Vector3{0,-555,0}, 555, 555, Math::Quaternion{pi / 2, 0, 0}, white);
        addQuad(Math::Vector3{0,0,555}, -555, -555, Math::Quaternion{}, white);
        addQuad(Math::Vector3{0,555,0}, 555, 555, Math::Quaternion{pi / 2, 0, 0}, white);

        auto addBox = [&world, scale](
            const Math::Vector3& min, 
            const Math::Vector3& max, 
            const Math::Quaternion& rotation, 
            std::shared_ptr<RTMaterial> material) {
            ProceduralGeometryDesc boxDesc{};
            boxDesc.type = RayTracing::AnalyticPrimitive::PrimitiveType::Cube;
            boxDesc.transform.SetPosition((min + max) * scale);
            boxDesc.transform.SetScale((max - min) * scale);
            boxDesc.transform.SetRotation(rotation);
            boxDesc.material = material;
            world.push_back(boxDesc);
        };

        addBox(
            Math::Vector3{130 - 555 / 2, 0 - 555 / 2, 65 - 555 / 2}, 
            Math::Vector3{295 - 555 / 2, 165 - 555 / 2, 230 - 555 / 2}, 
            Math::Quaternion{0, -pi * 10.f / 180.f, 0}, white);
        addBox(
            Math::Vector3{265 - 555 / 2, 0 - 555 / 2, 295 - 555 / 2}, 
            Math::Vector3{430 - 555 / 2, 330 - 555 / 2, 460 - 555 / 2}, 
            Math::Quaternion{0, pi * 18.f / 180.f, 0}, white);

        return world;
    }
};


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
        m_Camera->SetPosition({ 0, 0, -2 });

        m_RayTracer = std::make_unique<RayTracer>();
        m_RayTracer->SetCamera(m_Camera.get());

        m_CameraController = std::make_unique<CameraController>();
        m_CameraController->InitCamera(m_Camera.get());
        m_CameraController->SetMoveSpeed(1);

        // auto lihuazou = LoadModel("Models/AB/AliceADefault/AliceADefault.fbx");
        // auto sponza = LoadModel("Models/Sponza/pbr/sponza2.gltf");
        // auto plane = LoadModelFromeGeometry("Plane", Geometry::GeometryGenerator::CreateGrid(60, 60, 2, 2));
        // plane->transform.SetPosition({ 0, -2, 0 });

        // m_RayTracer->AddModel(lihuazou);
        // m_RayTracer->AddModel(sponza);
        // m_RayTracer->AddModel(plane);

        // auto scene = Scene::CreateSphereScene();
        auto scene = Scene::CreateCornellBox();
        m_RayTracer->AddProceduralGeometries(scene);
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