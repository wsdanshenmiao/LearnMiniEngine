#pragma once
#ifndef __GAMECORE_H__
#define __GAMECORE_H__

#include "../pch.h"

namespace DSM {
    class RenderContext;
}

namespace DSM::GameCore {
    // 程序抽象类
    struct IGameApp
    {
        // 初始化程序
        virtual void Startup() = 0;
        // 每帧调用一次更新函数
        virtual void Update(float deltaTime) = 0;
        virtual void OnResize(std::uint32_t width, std::uint32_t height){};
        // 自定义渲染场景
        virtual void RenderScene(RenderContext& renderContext) = 0;
        // 程序关闭时调用，清理资源
        virtual void Cleanup() = 0;

        virtual bool IsDown();
        
        virtual bool RequiresRaytracingSupport() const {return false;}
    };

    void OnResize(std::uint32_t width, std::uint32_t height);
    
    int RunApplication(
        IGameApp& app,
        std::uint32_t width,
        std::uint32_t height,
        const wchar_t* className,
        HINSTANCE hInstance,
        int nShowCmd);
}


#endif
