#include "SwapChain.h"
#include "RenderContext.h"

namespace DSM {
    
    SwapChain::SwapChain(const SwapChainDesc& swapChainDesc)
        :m_Width(swapChainDesc.m_Width), m_Height(swapChainDesc.m_Height){
        m_BackBufferFormat = swapChainDesc.m_Format;

        DXGI_SWAP_CHAIN_DESC1 swapChainDesc1{};
        swapChainDesc1.Width = swapChainDesc.m_Width;
        swapChainDesc1.Height = swapChainDesc.m_Height;
        swapChainDesc1.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc1.SampleDesc = { 1, 0 };
        swapChainDesc1.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc1.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        swapChainDesc1.Scaling = DXGI_SCALING_NONE;
        swapChainDesc1.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
        swapChainDesc1.BufferCount = sm_BackBufferCount;
        swapChainDesc1.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc1.Stereo = false;

        DXGI_SWAP_CHAIN_FULLSCREEN_DESC swapChainFullscreenDesc{};
        swapChainFullscreenDesc.Windowed = !swapChainDesc.m_FullScreen;

        Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain1;
        ASSERT_SUCCEEDED(g_RenderContext.GetFactory()->CreateSwapChainForHwnd(
            g_RenderContext.GetGraphicsQueue().GetCommandQueue(),
            swapChainDesc.m_hWnd,
            &swapChainDesc1,
            &swapChainFullscreenDesc,
            nullptr,
            swapChain1.GetAddressOf()));

        ASSERT_SUCCEEDED(swapChain1->QueryInterface(&m_SwapChain));

        m_hWnd = swapChainDesc.m_hWnd;

        m_BackBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();

        for (auto& backBuffer : m_BackBuffers) {
            backBuffer = std::make_unique<Texture>();
        }

        for (auto& bufferHanlde : m_BackBufferRTVs) {
            bufferHanlde = g_RenderContext.AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        }
        
        CreateBackBuffers();
    }

    SwapChain::~SwapChain()
    {
        for (auto& bufferHandle : m_BackBufferRTVs) {
            g_RenderContext.FreeDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, bufferHandle);
        }
        ASSERT_SUCCEEDED(m_SwapChain->SetFullscreenState(false, nullptr));
    }

    void SwapChain::Present(std::uint32_t sync)
    {
        ASSERT_SUCCEEDED(m_SwapChain->Present(sync, 0));
        m_BackBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();
    }

    void SwapChain::OnResize(std::uint32_t width, std::uint32_t height)
    {
        g_RenderContext.IdleGPU();
        
        ASSERT(m_SwapChain != nullptr);
        m_Width = std::max<std::uint32_t>(width, 8);
        m_Height = std::max<std::uint32_t>(height, 8);
        m_Height = height;

        for (auto& buffer : m_BackBuffers) {
            buffer->Destroy();
        }
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc1{};
        ASSERT_SUCCEEDED(m_SwapChain->GetDesc1(&swapChainDesc1));
        ASSERT_SUCCEEDED(m_SwapChain->ResizeBuffers(
            sm_BackBufferCount,
            m_Width, m_Height,
            swapChainDesc1.Format,
            0));

        m_BackBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();
        CreateBackBuffers();
    }

    void SwapChain::CreateBackBuffers()
    {
        for (std::uint32_t i = 0; i < sm_BackBufferCount; ++i) {
            ID3D12Resource* backBuffer{};
            ASSERT_SUCCEEDED(m_SwapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)))
            m_BackBuffers[i]->Create(L"SwapChain Back Buffer", backBuffer);
            D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
            rtvDesc.Format = m_BackBufferFormat;
            rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
            rtvDesc.Texture2D.MipSlice = 0;
            g_RenderContext.GetDevice()->
                CreateRenderTargetView(m_BackBuffers[i]->GetResource(), &rtvDesc, m_BackBufferRTVs[i]);
        }
    }
}
