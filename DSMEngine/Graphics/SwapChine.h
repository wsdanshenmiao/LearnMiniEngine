#pragma once
#ifndef __SWAPCHINE_H__
#define __SWAPCHINE_H__


#include <dxgi1_6.h>
#include "DescriptorHeap.h"

namespace DSM {
    
    class SwapChine
    {
    public:






    protected:
        Microsoft::WRL::ComPtr<IDXGISwapChain4> m_SwapChain{};

        std::uint32_t m_Width;
        std::uint32_t m_Height;
        std::uint64_t m_BackBufferIndex;
    };
}


#endif
