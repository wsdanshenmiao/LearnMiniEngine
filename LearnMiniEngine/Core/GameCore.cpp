#include "GameCore.h"

#include "Window.h"
#include "Utilities/Macros.h"
#include "Graphics/RenderContext.h"
#include <iostream>

namespace DSM::GameCore{
    IGameApp* g_CurrGameApp = nullptr;
    CpuTimer g_Timer{};
    

    void CalculateFrameStates(Window& window)
    {
        static int frameCnt = 0;
        static float timeElapsed = 0.0f;
        static std::string originTitle = window.GetTitle();

        frameCnt++;

        if ((g_Timer.TotalTime() - timeElapsed) >= 1.0f) {
            float fps = (float)frameCnt; // fps = frameCnt / 1
            float mspf = 1000.0f / fps;

            auto title = std::format("{}    FPS: {}    Frame Time: {} (ms)", originTitle, fps, mspf);
            window.SetTitle(title);

            // Reset for next average.
            frameCnt = 0;
            timeElapsed += 1.0f;
        }
    }

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
        app.Update(g_Timer.DeltaTime());
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
		width = (std::max)(width, 1u);
		height = (std::max)(height, 1u);
        g_RenderContext.OnResize(width, height);
        if (g_CurrGameApp != nullptr) {
            g_CurrGameApp->OnResize(width, height);
        }
    }

    int RunApplication(
        IGameApp& app,
        std::uint32_t width,
        std::uint32_t height,
        const char* className,
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

        g_Timer.Reset();
        while (win.Loop()) {
            g_Timer.Tick();
            CalculateFrameStates(win);
            UpdateApplication(app);
        }

        TerminateApplication(app);
        
        return 0;
    }
    
}
