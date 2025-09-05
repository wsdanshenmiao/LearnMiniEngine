#pragma once
#ifndef __SCALAR_H__
#define __SCALAR_H__

#include <DirectXMath.h>
#include <compare>


namespace DSM::Math {
    
    class Scalar
    {
    public:
        __forceinline Scalar(float v) noexcept { m_Vector = DirectX::XMVectorReplicate(v);}
        __forceinline explicit Scalar(DirectX::FXMVECTOR v) noexcept : m_Vector(v) {}

        __forceinline Scalar(const Scalar& other) noexcept : m_Vector(other) {};
        __forceinline Scalar& operator=(const Scalar& other) noexcept { m_Vector = other; return *this; };

        __forceinline Scalar& operator+=(Scalar other) noexcept
        {
            m_Vector = DirectX::XMVectorAdd(m_Vector, other);
            return *this;
        }
        __forceinline Scalar& operator+=(float v) noexcept{return operator+=(Scalar{v});}
        
        __forceinline Scalar& operator-=(Scalar other) noexcept
        {
            m_Vector = DirectX::XMVectorSubtract(m_Vector, other);
            return *this;
        }
        __forceinline Scalar& operator-=(float v) noexcept { return operator-=(Scalar{v}); }
        
        __forceinline Scalar& operator*=(Scalar other) noexcept
        {
            DirectX::XMVectorMultiply(m_Vector, other);
            return *this;
        }
        __forceinline Scalar& operator*=(float v) noexcept { return operator*=(Scalar{v}); }

        __forceinline Scalar& operator/=(Scalar other) noexcept
        {
            DirectX::XMVectorDivide(m_Vector, other);
            return *this;
        }
        __forceinline Scalar& operator/=(float v) noexcept { return operator/=(Scalar{v}); }

        __forceinline bool operator==(const Scalar& other) const noexcept { return DirectX::XMVector4Equal(m_Vector, other); }

        __forceinline operator DirectX::XMVECTOR() const noexcept { return m_Vector; }
        __forceinline operator float() const noexcept { return DirectX::XMVectorGetX(m_Vector); }

    private:
        DirectX::XMVECTOR m_Vector{};
    };

    __forceinline Scalar operator-(Scalar s) noexcept{ return Scalar(DirectX::XMVectorNegate(s)); }
    __forceinline Scalar operator+(Scalar s0, Scalar s1) noexcept{ return Scalar(s0) += s1; }
    __forceinline Scalar operator+(Scalar s0, float s1) noexcept{ return Scalar(s0) += s1; }
    __forceinline Scalar operator+(float s0, Scalar s1) noexcept{ return Scalar(s0) += s1; }
    __forceinline Scalar operator-(Scalar s0, Scalar s1) noexcept{ return Scalar(s0) -= s1; }
    __forceinline Scalar operator-(Scalar s0, float s1) noexcept{ return Scalar(s0) -= s1; }
    __forceinline Scalar operator-(float s0, Scalar s1) noexcept{ return Scalar(s0) -= s1; }
    __forceinline Scalar operator*(Scalar s0, Scalar s1) noexcept{ return Scalar(s0) *= s1; }
    __forceinline Scalar operator*(Scalar s0, float s1) noexcept{ return Scalar(s0) *= s1; }
    __forceinline Scalar operator*(float s0, Scalar s1) noexcept{ return Scalar(s0 *= s1); }
    __forceinline Scalar operator/(Scalar s0, Scalar s1) noexcept{ return Scalar(s0) /= s1; }
    __forceinline Scalar operator/(Scalar s0, float s1) noexcept{ return Scalar(s0) /= s1; }
    __forceinline Scalar operator/(float s0, Scalar s1) noexcept{ return Scalar(s0) /= s1; }

    
    __forceinline std::partial_ordering operator<=>(Scalar s0, Scalar s1) noexcept { return float(s0) <=> float(s1); }
    __forceinline std::partial_ordering operator<=>(Scalar s0, float s1) noexcept { return float(s0) <=> s1; }
    __forceinline std::partial_ordering operator<=>(float s0, Scalar s1) noexcept { return s0 <=> float(s1); }
    __forceinline bool operator==(Scalar s0, Scalar s1) noexcept { return float(s0) == float(s1); }
    __forceinline bool operator==(Scalar s0, float s1) noexcept { return float(s0) == s1; }
    __forceinline bool operator==(float s0, Scalar s1) noexcept { return s0 == float(s1); }
}


#endif