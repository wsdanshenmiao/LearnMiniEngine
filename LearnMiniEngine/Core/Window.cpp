#include "Window.h"

#include "GameCore.h"
#include "../Graphics/RenderContext.h"


namespace DSM {
    
    LRESULT CALLBACK WndProc( HWND, UINT, WPARAM, LPARAM );

    Window::Window(const WindowDesc& desc)
    {
        // 注册类
        WNDCLASSEX wcex{};
        wcex.hInstance = desc.m_HInstance;
        wcex.lpszClassName = desc.m_Title.c_str();
        wcex.style= CS_HREDRAW | CS_VREDRAW;
        wcex.hIcon = LoadIcon(desc.m_HInstance, IDI_APPLICATION);
        wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wcex.lpfnWndProc = WndProc;
        wcex.cbSize = sizeof(WNDCLASSEX);
        wcex.cbClsExtra = 0;
        wcex.cbWndExtra = 0;
        wcex.hIconSm = LoadIcon(desc.m_HInstance, IDI_APPLICATION);
        wcex.lpszMenuName = nullptr;
        ASSERT(0 != RegisterClassEx(&wcex), "RegisterClassEx failed");

        // 创建窗口
        RECT rect{ 0, 0, (LONG)desc.m_Width, (LONG)desc.m_Height };
        AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

        m_WindowHandle = CreateWindow(
            desc.m_Title.c_str(),
            desc.m_Title.c_str(),
            WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
            rect.right - rect.left,
            rect.bottom - rect.top,
            nullptr, nullptr,
            desc.m_HInstance, nullptr);

        auto showCmd = desc.m_Maximized ? SW_SHOWMAXIMIZED : SW_SHOWNORMAL;
        ShowWindow(m_WindowHandle, showCmd);


        UpdateWindow(m_WindowHandle);
    }

    bool Window::Loop()
    {
        MSG msg{};
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            if (msg.message == WM_QUIT) return false;
        }
        return true;
    }

    LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
    {
        switch( message )
        {
        case WM_SIZE: {
            GameCore::OnResize((UINT)(UINT64)lParam & 0xFFFF, (UINT)(UINT64)lParam >> 16); break;
        }
        case WM_DESTROY: {
            PostQuitMessage(0); break;
        }
        default:
            return DefWindowProc( hWnd, message, wParam, lParam );
        }

        return 0;
    }
}


