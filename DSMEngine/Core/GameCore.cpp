#include "GameCore.h"

#include "Window.h"
#include "../Utilities/Macros.h"
#include "../Graphics/RenderContext.h"

namespace DSM::GameCore{
    IGameApp* g_CurrGameApp = nullptr;
    
    bool IGameApp::IsDown()
    {
        return false;
    }


    // 初始化引擎
    void InitializeApplication(IGameApp& app, const Window& window)
    {
        g_RenderContext.Create(app.RequiresRaytracingSupport(), window);
        
        app.Startup();
    }

    // 更新引擎
    bool UpdateApplication(IGameApp& app)
    {

        app.Update(0);
        app.RenderScene(g_RenderContext);
        
        return !app.IsDown();
    }

    void TerminateApplication(IGameApp& app)
    {
        app.Cleanup();

        g_RenderContext.Shutdown();
    }


    void OnResize(std::uint32_t width, std::uint32_t height)
    {
        g_RenderContext.OnResize(width, height);
        if (g_CurrGameApp != nullptr) {
            g_CurrGameApp->OnResize(width, height);
        }
    }

    int RunApplication(
        IGameApp& app,
        std::uint32_t width,
        std::uint32_t height,
        const wchar_t* className,
        HINSTANCE hInstance,
        int nShowCmd)
    {
        // 检测 DirectXMath 库是否支持当前平台
        if (!DirectX::XMVerifyCPUSupport()) {
            return 1;
        }

        WindowDesc winDesc{};
        winDesc.m_Width = width;
        winDesc.m_Height = height;
        winDesc.m_Title = className;
        winDesc.m_Maximized = false;
        winDesc.m_HInstance = hInstance;
        Window win{winDesc};

        g_CurrGameApp = &app;
        
        InitializeApplication(app, win);

        while (win.Loop()) {
            UpdateApplication(app);
        }

        TerminateApplication(app);
        
        return 0;
    }
    
}
