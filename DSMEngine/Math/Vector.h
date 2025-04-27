#pragma once
#ifndef __VECTOR_H__
#define __VECTOR_H__

#include "Scalar.h"

namespace DSM::Math {
    class Vector4;

    class BoolVector
    {
    public:
        INLINE BoolVector(DirectX::FXMVECTOR v) noexcept : m_Vector(v){}

        INLINE operator DirectX::XMVECTOR() const noexcept { return m_Vector; }
        INLINE operator bool() const noexcept { return DirectX::XMVectorGetX(m_Vector) == 0; }

    private:
        DirectX::XMVECTOR m_Vector{};
    };
    
    
    class Vector3
    {
    public:
        INLINE Vector3() noexcept : m_Vector(DirectX::XMVectorReplicate(0)){}
        INLINE Vector3(Scalar s) noexcept : m_Vector(s){}
        INLINE Vector3(float x, float y, float z) noexcept : m_Vector(DirectX::XMVectorSet(x, y, z, z)){}
        INLINE Vector3(const DirectX::XMFLOAT3& f3) noexcept : m_Vector(DirectX::XMLoadFloat3(&f3)) {}
        INLINE explicit Vector3(DirectX::FXMVECTOR v) noexcept : m_Vector(v) {}
        INLINE explicit Vector3(const Vector4& v4) noexcept;

        INLINE Vector3(const Vector3& other) noexcept : m_Vector(other) {};
        INLINE Vector3& operator=(const Vector3& other) noexcept { m_Vector = other; return *this; };

        INLINE Scalar GetX() const noexcept { return Scalar{DirectX::XMVectorSplatX(m_Vector)}; }
        INLINE Scalar GetY() const noexcept { return Scalar{DirectX::XMVectorSplatY(m_Vector)}; }
        INLINE Scalar GetZ() const noexcept { return Scalar{DirectX::XMVectorSplatZ(m_Vector)}; }
        // XMVectorPermute 会从两个向量中重新布局新的向量，8个标量分别对应0 - 7 
        INLINE void SetX(Scalar x) noexcept { m_Vector = DirectX::XMVectorPermute<4,1,2,3>(m_Vector, x); }
        INLINE void SetY(Scalar y) noexcept { m_Vector = DirectX::XMVectorPermute<0,5,2,3>(m_Vector, y); }
        INLINE void SetZ(Scalar z) noexcept { m_Vector = DirectX::XMVectorPermute<0,1,6,3>(m_Vector, z); }

        INLINE Vector3& operator+=(Vector3 other) noexcept
        {
            m_Vector = DirectX::XMVectorAdd(m_Vector, other);
            return *this;
        }
        INLINE Vector3& operator-=(Vector3 other) noexcept
        {
            m_Vector = DirectX::XMVectorSubtract(m_Vector, other);
            return *this;
        }
        INLINE Vector3& operator*=(Vector3 scalar) noexcept
        {
            m_Vector = DirectX::XMVectorMultiply(m_Vector, scalar);
            return *this;
        }
        INLINE Vector3& operator*=(Scalar scalar) noexcept { return operator*=(Vector3{scalar}); }
        INLINE Vector3& operator*=(float v) noexcept { return operator*=(Scalar{v}); }
        INLINE Vector3& operator/=(Vector3 scalar) noexcept
        {
            m_Vector = DirectX::XMVectorDivide(m_Vector, scalar);
            return *this;
        }
        INLINE Vector3& operator/=(Scalar scalar) noexcept { return operator/=(Vector3{scalar}); }
        INLINE Vector3& operator/=(float v) noexcept { return operator/=(Scalar{v}); }

        INLINE bool operator==(Vector3 other) const noexcept
        {
            return DirectX::XMVector3Equal(m_Vector, other);
        }
        
        INLINE operator DirectX::XMVECTOR() const noexcept { return m_Vector; }

        static INLINE Scalar Dot(Vector3 v0, Vector3 v1) noexcept { return Scalar{DirectX::XMVector3Dot(v0, v1)}; }
        static INLINE Vector3 Cross(Vector3 v0, Vector3 v1) noexcept { return Vector3{DirectX::XMVector3Cross(v0, v1)}; }
        
    private:
        DirectX::XMVECTOR m_Vector{};
    };

    INLINE Vector3 operator-(Vector3 v) noexcept { return Vector3{DirectX::XMVectorNegate(v)}; }
    INLINE Vector3 operator+(Vector3 v0, Vector3 v1) noexcept { return v0 += v1; };
    INLINE Vector3 operator-(Vector3 v0, Vector3 v1) noexcept { return v0 -= v1; };
    INLINE Vector3 operator*(Vector3 v0, Vector3 v1) noexcept { return v0 *= v1; };
    INLINE Vector3 operator*(Vector3 v0, Scalar scalar) noexcept { return v0 *= scalar; };
    INLINE Vector3 operator*(Scalar scalar, Vector3 v) noexcept { return v *= scalar; };
    INLINE Vector3 operator*(Vector3 v0, float scalar) noexcept { return v0 *= scalar; };
    INLINE Vector3 operator*(float scalar, Vector3 v) noexcept { return v *= scalar; };
    INLINE Vector3 operator/(Vector3 v0, Vector3 v1) noexcept { return v0 /= v1; };
    INLINE Vector3 operator/(Vector3 v0, Scalar scalar) noexcept { return v0 /= scalar; };
    INLINE Vector3 operator/(Vector3 v, float scalar) noexcept { return v /= scalar; };
    
    

    class Vector4
    {
    public:
        INLINE Vector4() noexcept : m_Vector(DirectX::XMVectorReplicate(0)){}
        INLINE Vector4(Scalar s) noexcept : m_Vector(s){}
        INLINE Vector4(float x, float y, float z, float w) noexcept : m_Vector(DirectX::XMVectorSet(x, y, z, w)){}
        INLINE Vector4(const DirectX::XMFLOAT4& f4) noexcept : m_Vector(DirectX::XMLoadFloat4(&f4)) {}
        INLINE Vector4(Vector3 xyz, float w) noexcept : m_Vector(DirectX::XMVectorSetW(xyz, w)){}
        INLINE explicit Vector4(DirectX::FXMVECTOR v) noexcept : m_Vector(v) {}
        INLINE explicit Vector4(const Vector3& v3) noexcept : m_Vector(DirectX::XMVectorSetW(v3, 1)){}

        INLINE Vector4(const Vector4& other) noexcept : m_Vector(other) {};
        INLINE Vector4& operator=(const Vector4& other) noexcept { m_Vector = other; return *this; };

        INLINE Scalar GetX() const noexcept { return Scalar{DirectX::XMVectorSplatX(m_Vector)}; }
        INLINE Scalar GetY() const noexcept { return Scalar{DirectX::XMVectorSplatY(m_Vector)}; }
        INLINE Scalar GetZ() const noexcept { return Scalar{DirectX::XMVectorSplatZ(m_Vector)}; }
        INLINE Scalar GetW() const noexcept { return Scalar{DirectX::XMVectorSplatW(m_Vector)}; }
        // XMVectorPermute 会从两个向量中重新布局新的向量，8个标量分别对应0 - 7 
        INLINE void SetX(Scalar x) noexcept { m_Vector = DirectX::XMVectorPermute<4,1,2,3>(m_Vector, x); }
        INLINE void SetY(Scalar y) noexcept { m_Vector = DirectX::XMVectorPermute<0,5,2,3>(m_Vector, y); }
        INLINE void SetZ(Scalar z) noexcept { m_Vector = DirectX::XMVectorPermute<0,1,6,3>(m_Vector, z); }
        INLINE void SetW(Scalar w) noexcept { m_Vector = DirectX::XMVectorPermute<0,1,2,7>(m_Vector, w); }
        INLINE void SetXYZ(Vector3 xyz) noexcept { m_Vector = DirectX::XMVectorPermute<4,5,6,3>(m_Vector, xyz); }

        
        INLINE Vector4& operator+=(Vector4 other) noexcept
        {
            m_Vector = DirectX::XMVectorAdd(m_Vector, other);
            return *this;
        }
        INLINE Vector4& operator-=(Vector4 other) noexcept
        {
            m_Vector = DirectX::XMVectorSubtract(m_Vector, other);
            return *this;
        }
        INLINE Vector4& operator*=(Vector4 scalar) noexcept
        {
            m_Vector = DirectX::XMVectorMultiply(m_Vector, scalar);
            return *this;
        }
        INLINE Vector4& operator*=(Scalar scalar) noexcept { return operator*=(Vector4{scalar}); }
        INLINE Vector4& operator*=(float v) noexcept { return operator*=(Scalar{v}); }
        INLINE Vector4& operator/=(Vector4 scalar) noexcept
        {
            m_Vector = DirectX::XMVectorDivide(m_Vector, scalar);
            return *this;
        }
        INLINE Vector4& operator/=(Scalar scalar) noexcept { return operator/=(Vector4{scalar}); }
        INLINE Vector4& operator/=(float v) noexcept { return operator/=(Scalar{v}); }

        INLINE bool operator==(Vector4 other) const noexcept
        {
            return DirectX::XMVector4Equal(m_Vector, other);
        }
        
        INLINE operator DirectX::XMVECTOR() const noexcept { return m_Vector; }

        INLINE static Scalar Dot(Vector4 v0, Vector4 v1) noexcept { return Scalar{DirectX::XMVector4Dot(v0, v1)}; }
        INLINE static Vector4 Cross(Vector4 v0, Vector4 v1, Vector4 v2) noexcept
        {
            return Vector4{DirectX::XMVector4Cross(v0, v1, v2)};
        }
        
    private:
        DirectX::XMVECTOR m_Vector{};
    };

    INLINE Vector4 operator-(Vector4 v) noexcept { return Vector4{DirectX::XMVectorNegate(v)}; }
    INLINE Vector4 operator+(Vector4 v0, Vector4 v1) noexcept { return v0 += v1; };
    INLINE Vector4 operator-(Vector4 v0, Vector4 v1) noexcept { return v0 -= v1; };
    INLINE Vector4 operator*(Vector4 v0, Vector4 v1) noexcept { return v0 *= v1; };
    INLINE Vector4 operator*(Vector4 v0, Scalar scalar) noexcept { return v0 *= scalar; };
    INLINE Vector4 operator*(Scalar scalar, Vector4 v) noexcept { return v *= scalar; };
    INLINE Vector4 operator*(Vector4 v0, float scalar) noexcept { return v0 *= scalar; };
    INLINE Vector4 operator*(float scalar, Vector4 v) noexcept { return v *= scalar; };
    INLINE Vector4 operator/(Vector4 v0, Vector4 v1) noexcept { return v0 /= v1; };
    INLINE Vector4 operator/(Vector4 v0, Scalar scalar) noexcept { return v0 /= scalar; };
    INLINE Vector4 operator/(Vector4 v, float scalar) noexcept { return v /= scalar; };

    
    inline Vector3::Vector3(const Vector4& v4) noexcept
        :m_Vector(v4){
    }



}

#endif