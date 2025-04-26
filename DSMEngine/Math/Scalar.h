#pragma once
#ifndef __SCALAR_H__
#define __SCALAR_H__

#include "MathCommon.h"


namespace DSM::Math {
    
    class Scalar
    {
    public:
        INLINE Scalar(float v) noexcept { m_Vector = DirectX::XMVectorReplicate(v);}
        INLINE explicit Scalar(DirectX::FXMVECTOR v) noexcept : m_Vector(v) {}

        INLINE Scalar(const Scalar& other) noexcept : m_Vector(other) {};
        INLINE Scalar& operator=(const Scalar& other) noexcept { m_Vector = other; return *this; };

        INLINE Scalar& operator+=(const Scalar& other) noexcept
        {
            m_Vector = DirectX::XMVectorAdd(m_Vector, other);
            return *this;
        }
        INLINE Scalar& operator+=(float v) noexcept{return operator+=(Scalar{v});}
        
        INLINE Scalar& operator-=(const Scalar& other) noexcept
        {
            m_Vector = DirectX::XMVectorSubtract(m_Vector, other);
            return *this;
        }
        INLINE Scalar& operator-=(float v) noexcept { return operator-=(Scalar{v}); }
        
        INLINE Scalar& operator*=(const Scalar& other) noexcept
        {
            DirectX::XMVectorMultiply(m_Vector, other);
            return *this;
        }
        INLINE Scalar& operator*=(float v) noexcept { return operator*=(Scalar{v}); }

        INLINE Scalar& operator/=(const Scalar& other) noexcept
        {
            DirectX::XMVectorDivide(m_Vector, other);
            return *this;
        }
        INLINE Scalar& operator/=(float v) noexcept { return operator/=(Scalar{v}); }
        
        
        INLINE operator DirectX::XMVECTOR() const noexcept { return m_Vector; }
        INLINE operator float() const noexcept { return DirectX::XMVectorGetX(m_Vector); }

    private:
        DirectX::XMVECTOR m_Vector{};
    };

    INLINE Scalar operator-(const Scalar& s) noexcept{ return Scalar(DirectX::XMVectorNegate(s)); }
    INLINE Scalar operator+(const Scalar& s0, const Scalar& s1) noexcept{ return Scalar(s0) += s1; }
    INLINE Scalar operator+(const Scalar& s0, float s1) noexcept{ return Scalar(s0) += s1; }
    INLINE Scalar operator+(float s0, const Scalar& s1) noexcept{ return Scalar(s0) += s1; }
    INLINE Scalar operator-(const Scalar& s0, const Scalar& s1) noexcept{ return Scalar(s0) -= s1; }
    INLINE Scalar operator-(const Scalar& s0, float s1) noexcept{ return Scalar(s0) -= s1; }
    INLINE Scalar operator-(float s0, const Scalar& s1) noexcept{ return Scalar(s0) -= s1; }
    INLINE Scalar operator*(const Scalar& s0, const Scalar& s1) noexcept{ return Scalar(s0) *= s1; }
    INLINE Scalar operator*(const Scalar& s0, float s1) noexcept{ return Scalar(s0) *= s1; }
    INLINE Scalar operator*(float s0, const Scalar& s1) noexcept{ return Scalar(s0 *= s1); }
    INLINE Scalar operator/(const Scalar& s0, const Scalar& s1) noexcept{ return Scalar(s0) /= s1; }
    INLINE Scalar operator/(const Scalar& s0, float s1) noexcept{ return Scalar(s0) /= s1; }
    INLINE Scalar operator/(float s0, const Scalar& s1) noexcept{ return Scalar(s0) /= s1; }

}


#endif