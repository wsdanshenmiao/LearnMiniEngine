#pragma once
#ifndef __WINDOW_H__
#define __WINDOW_H__

#include <string>
#include "../Utilities/Macros.h"


namespace DSM {
    struct WindowDesc
    {
        std::uint32_t m_Width{};
        std::uint32_t m_Height{};
        bool m_Maximized = false;
        HINSTANCE m_HInstance;
        std::string m_Title{};
    };

    
    class Window
    {
    public:
        Window(const WindowDesc& desc);
        ~Window()
        {
            if (m_WindowHandle != nullptr) {
                DestroyWindow(m_WindowHandle);
            }
        };
        DSM_NONCOPYABLE_NONMOVABLE(Window);

        bool Loop();

        std::uint32_t GetWidth() const
        {
            RECT rect{};
            GetClientRect(m_WindowHandle, &rect);
            return static_cast<std::uint32_t>(rect.right - rect.left);
        }
        std::uint32_t GetHeight() const
        {
            RECT rect{};
            GetClientRect(m_WindowHandle, &rect);
            return static_cast<std::uint32_t>(rect.bottom - rect.top);
        }
        HWND GetHandle() const noexcept { return m_WindowHandle; }
        const std::string& GetTitle() const noexcept { return m_Desc.m_Title; }
        void SetTitle(const std::string& title);

    private:
        WindowDesc m_Desc{};
        HWND m_WindowHandle = nullptr;
    };

}


#endif