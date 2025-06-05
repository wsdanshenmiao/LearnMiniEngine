#include "Color.h"

using namespace DirectX;

namespace DSM {
    Color Color::ToSRGB() const noexcept
    {
        XMVECTOR c = XMVectorSaturate(m_Color);
        // 先进行指数部分的转换
        XMVECTOR result = XMVectorSubtract(XMVectorScale(XMVectorPow(c, XMVectorReplicate(1.f / 2.4f)), 1.055f), XMVectorReplicate(0.055f));
        // 将小于0.0031308的部分进行覆盖
        result = XMVectorSelect(result, XMVectorScale(c, 12.92f), XMVectorLessOrEqual(result, XMVectorReplicate(0.0031308f)));
        return XMVectorSelect(c, result, g_XMSelect1110);
    }

    Color Color::FromSRGB() const noexcept
    {
        XMVECTOR c = XMVectorSaturate(m_Color);
        // 先进行指数部分的转换
        XMVECTOR result = XMVectorPow(XMVectorScale(XMVectorAdd(c, XMVectorReplicate(0.055f)), 1.f / 1.055f), XMVectorReplicate(2.4f));
        // 将小于0.0031308的部分进行覆盖
        result = XMVectorSelect(result, XMVectorScale(c, 1.f / 12.92f), XMVectorLessOrEqual(result, XMVectorReplicate(0.04045f)));
        return XMVectorSelect(c, result, g_XMSelect1110);
    }

    Color Color::ToREC709() const noexcept
    {
        XMVECTOR c = XMVectorSaturate(m_Color);
        // 先进行指数部分的转换
        XMVECTOR result = XMVectorSubtract(XMVectorScale(XMVectorPow(c, XMVectorReplicate(1.f / 2.2f)), 1.099f), XMVectorReplicate(0.099f));
        // 将小于0.0031308的部分进行覆盖
        result = XMVectorSelect(result, XMVectorScale(c, 4.5f), XMVectorLessOrEqual(result, XMVectorReplicate(0.018f)));
        return XMVectorSelect(c, result, g_XMSelect1110);
    }

    Color Color::FromREC709() const noexcept
    {
        XMVECTOR c = XMVectorSaturate(m_Color);
        // 先进行指数部分的转换
        XMVECTOR result = XMVectorPow(XMVectorScale(XMVectorAdd(c, XMVectorReplicate(0.099f)), 1.f / 1.099f), XMVectorReplicate(2.2f));
        // 将小于0.0031308的部分进行覆盖
        result = XMVectorSelect(result, XMVectorScale(c, 1.f / 4.5f), XMVectorLessOrEqual(result, XMVectorReplicate(0.018f)));
        return XMVectorSelect(c, result, g_XMSelect1110);
    }

    uint32_t Color::R8G8B8A8() const noexcept
    {
        XMVECTOR result = XMVectorRound(XMVectorMultiply(XMVectorSaturate(m_Color), XMVectorReplicate(255.0f)));
        result = _mm_castsi128_ps(_mm_cvttps_epi32(result));
        uint32_t r = XMVectorGetIntX(result);
        uint32_t g = XMVectorGetIntY(result);
        uint32_t b = XMVectorGetIntZ(result);
        uint32_t a = XMVectorGetIntW(result);
        return a << 24 | b << 16 | g << 8 | r;
    }
}
