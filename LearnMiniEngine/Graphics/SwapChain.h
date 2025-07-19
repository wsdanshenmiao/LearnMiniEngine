#pragma once
#ifndef __SWAPCHINE_H__
#define __SWAPCHINE_H__

#include "DescriptorHeap.h"
#include "Resource/Texture.h"

namespace DSM {

    struct SwapChainDesc
    {
        std::uint32_t m_Width{};
        std::uint32_t m_Height{};
        DXGI_FORMAT m_Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        HWND m_hWnd = nullptr;
        bool m_FullScreen = false;
    };
    
    class SwapChain
    {
    public:
        SwapChain(const SwapChainDesc& swapChainDesc);
        ~SwapChain();
        DSM_NONCOPYABLE(SwapChain);

        std::uint32_t GetBackBufferIndex() const noexcept { return m_BackBufferIndex; }
        DescriptorHandle GetBackBufferRTV() { return m_BackBufferRTVs[m_BackBufferIndex]; }
        Texture* GetBackBuffer() { return m_BackBuffers[m_BackBufferIndex].get(); }
        std::uint32_t GetWidth() const noexcept { return m_Width; }
        std::uint32_t GetHeight() const noexcept { return m_Height; }
        HWND GetWindowHandle() const noexcept { return m_hWnd; }

        void Present(std::uint32_t sync = 0);
        void OnResize(std::uint32_t width, std::uint32_t height);


    private:
        void CreateBackBuffers();

    public:
        inline static constexpr int sm_BackBufferCount = 3;
        inline static bool sm_EnableHDROutput = false;
        
    protected:
        IDXGISwapChain4* m_SwapChain{};
        std::array<std::unique_ptr<Texture>, sm_BackBufferCount> m_BackBuffers{};
        std::array<DescriptorHandle, sm_BackBufferCount> m_BackBufferRTVs{};

        std::uint32_t m_Width;
        std::uint32_t m_Height;
        std::uint64_t m_BackBufferIndex;
        DXGI_FORMAT m_BackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

        HWND m_hWnd{};
    };
}


#endif
