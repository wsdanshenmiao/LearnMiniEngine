#include "GameCore.h"
#include "../Utilities/Macros.h"
#include "../Graphics/Display.h"
#include "../Graphics/RenderContext.h"

namespace DSM::GameCore{
    using namespace Graphics;
    
    bool IGameApp::IsDown()
    {
        return false;
    }


    // 初始化引擎
    void InitializeApplication(IGameApp& app)
    {
        g_RenderContext.Create(app.RequiresRaytracingSupport());
        
        app.Startup();
    }

    // 更新引擎
    bool UpdateApplication(IGameApp& app)
    {

        app.Update(0);
        app.RenderScene();

        
        return !app.IsDown();
    }

    void TerminateApplication(IGameApp& app)
    {
        app.Cleanup();

        g_RenderContext.Shutdown();
    }
    
    HWND g_hWnd = nullptr;
    
    LRESULT CALLBACK WndProc( HWND, UINT, WPARAM, LPARAM );
    
    int RunApplication(IGameApp& app, const wchar_t* className, HINSTANCE hInstance, int nShowCmd)
    {
        // 检测 DirectXMath 库是否支持当前平台
        if (!DirectX::XMVerifyCPUSupport()) {
            return 1;
        }

        // 注册类
        WNDCLASSEX wcex{};
        wcex.hInstance = hInstance;
        wcex.lpszClassName = className;
        wcex.style= CS_HREDRAW | CS_VREDRAW;
        wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
        wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wcex.lpfnWndProc = WndProc;
        wcex.cbSize = sizeof(WNDCLASSEX);
        wcex.cbClsExtra = 0;
        wcex.cbWndExtra = 0;
        wcex.hIconSm = LoadIcon(hInstance, IDI_APPLICATION);
        wcex.lpszMenuName = nullptr;
        ASSERT(0 != RegisterClassEx(&wcex), "RegisterClassEx failed");

        // 创建窗口
        RECT rect{ 0, 0, (LONG)g_DisplayWidth, (LONG)g_DisplayHeight};
        AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

        g_hWnd = CreateWindow(className, className, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
            rect.right - rect.left, rect.bottom - rect.top, nullptr, nullptr, hInstance, nullptr);

        InitializeApplication(app);
        
        ShowWindow(g_hWnd, nShowCmd);

        do {
            MSG msg{};
            bool done = false;
            while (PeekMessage(&msg, g_hWnd, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);

                if (msg.message == WM_QUIT) {
                    done = true;
                }
            }
            if (done == true) break;
        }while (UpdateApplication(app));

        TerminateApplication(app);
        
        return 0;
    }

    LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
    {
        switch( message )
        {
        case WM_SIZE:
            
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc( hWnd, message, wParam, lParam );
        }

        return 0;
    }
}
