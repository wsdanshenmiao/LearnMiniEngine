#pragma once
#ifndef __VECTOR_H__
#define __VECTOR_H__

#include <initializer_list>
#include "Scalar.h"

namespace DSM::Math {

    template<std::size_t N> requires 1 < N && N <= 4
    class Vector
    {
    public:
        INLINE constexpr Vector() noexcept : m_Vector(DirectX::XMVectorReplicate(0)){}
        INLINE constexpr Vector(DirectX::FXMVECTOR v) : m_Vector(v) {}
        INLINE constexpr explicit Vector(std::initializer_list<float> values) noexcept
        {
            DirectX::XMFLOAT4 tmp{};
            memcpy(&tmp, values.begin(), std::min(values.size(), std::size_t(4)));
            m_Vector = DirectX::XMLoadFloat4(&tmp);
        }

        INLINE constexpr Vector(const Vector& other) noexcept = default;
        INLINE constexpr Vector& operator=(const Vector& other) noexcept = default;

        
        INLINE constexpr operator DirectX::XMVECTOR() { return m_Vector; }

    private:
        DirectX::XMVECTOR m_Vector{};
    };

    using Vector2 = Vector<2>;
    using Vector3 = Vector<3>;
    using Vector4 = Vector<4>;
}

#endif