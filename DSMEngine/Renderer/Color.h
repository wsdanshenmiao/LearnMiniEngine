#pragma once
#ifndef __COLOR_H__
#define __COLOR_H__

#include <DirectXMath.h>

namespace DSM {
    
    class Color
    {
    public:
        Color() noexcept : m_Color(DirectX::g_XMOne){}
        Color(float r, float g, float b, float a = 1.0f) noexcept { m_Color.v = DirectX::XMVectorSet(r, g, b, a); }
        Color(DirectX::FXMVECTOR v) noexcept { m_Color.v = v; }
        Color(const DirectX::XMVECTORF32& v) noexcept { m_Color.v = v; }

        float R() const noexcept { return DirectX::XMVectorGetX(m_Color.v); }
        float G() const noexcept { return DirectX::XMVectorGetY(m_Color.v); }
        float B() const noexcept { return DirectX::XMVectorGetZ(m_Color.v); }
        float A() const noexcept { return DirectX::XMVectorGetW(m_Color.v); }

        void SetR(float r) noexcept { m_Color.f[0] = r; }
        void SetG(float g) noexcept { m_Color.f[1] = g; }
        void SetB(float b) noexcept { m_Color.f[2] = b; }
        void SetA(float a) noexcept { m_Color.f[3] = a; }
        void SetRGB(float r, float g, float b) noexcept
        {
            m_Color.v = DirectX::XMVectorSelect(m_Color, DirectX::XMVectorSet(r,g,b,b), DirectX::g_XMMask3);
        }

        float* GetPtr() { return reinterpret_cast<float*>(this); }
        const float* GetPtr() const { return reinterpret_cast<const float*>(this); }

        // 从线性空间转换到SRGB：
        // 当c大于0.0031308时使用指数部分进行转换：c_srgb = 1.055 * c_linear ^ (1 / 2.4) - 0.055
        // 反之使用线性部分：c_srgb = 12.92 * c_linear
        Color ToSRGB() const noexcept;
        // SRGB空间转换到线性空间
        Color FromSRGB() const noexcept;

        // 从线性空间变换到 REC.709
        // 当c大于0.018时转换为：c_709 = 1.099 * c_linear ^ (1 / 2.2) - 0.099
        // 反之使用：c_709 = 4.5 * c_linear
        Color ToREC709() const noexcept;
        Color FromREC709() const noexcept;

        operator DirectX::XMVECTOR() const noexcept { return m_Color; }

    private:
        DirectX::XMVECTORF32 m_Color;
    };

    
	__forceinline Color Min(Color a, Color b) { return Color(DirectX::XMVectorMin(a, b)); }
	__forceinline Color Max(Color a, Color b) { return Color(DirectX::XMVectorMax(a, b)); }
	__forceinline Color Clamp(Color c, Color min, Color max) { return Color(DirectX::XMVectorClamp(c, min, max)); }

}

#endif