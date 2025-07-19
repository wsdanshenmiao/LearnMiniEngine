#pragma once
#ifndef __VECTOR_H__
#define __VECTOR_H__

#include "Scalar.h"

namespace DSM::Math {
    class Vector4;


    
    
    class Vector3
    {
    public:
        __forceinline Vector3() noexcept : m_Vector(DirectX::XMVectorReplicate(0)){}
        __forceinline Vector3(Scalar s) noexcept : m_Vector(s){}
        __forceinline Vector3(float x, float y, float z) noexcept : m_Vector(DirectX::XMVectorSet(x, y, z, z)){}
        __forceinline Vector3(const DirectX::XMFLOAT3& f3) noexcept : m_Vector(DirectX::XMLoadFloat3(&f3)) {}
        __forceinline explicit Vector3(DirectX::FXMVECTOR v) noexcept : m_Vector(v) {}
        __forceinline explicit Vector3(const Vector4& v4) noexcept;

        __forceinline Vector3(const Vector3& other) noexcept : m_Vector(other) {};
        __forceinline Vector3& operator=(const Vector3& other) noexcept { m_Vector = other; return *this; };

        __forceinline Scalar GetX() const noexcept { return Scalar{DirectX::XMVectorSplatX(m_Vector)}; }
        __forceinline Scalar GetY() const noexcept { return Scalar{DirectX::XMVectorSplatY(m_Vector)}; }
        __forceinline Scalar GetZ() const noexcept { return Scalar{DirectX::XMVectorSplatZ(m_Vector)}; }
        // XMVectorPermute 会从两个向量中重新布局新的向量，8个标量分别对应0 - 7 
        __forceinline void SetX(Scalar x) noexcept { m_Vector = DirectX::XMVectorPermute<4,1,2,3>(m_Vector, x); }
        __forceinline void SetY(Scalar y) noexcept { m_Vector = DirectX::XMVectorPermute<0,5,2,3>(m_Vector, y); }
        __forceinline void SetZ(Scalar z) noexcept { m_Vector = DirectX::XMVectorPermute<0,1,6,3>(m_Vector, z); }

        __forceinline Vector3& operator+=(Vector3 other) noexcept
        {
            m_Vector = DirectX::XMVectorAdd(m_Vector, other);
            return *this;
        }
        __forceinline Vector3& operator-=(Vector3 other) noexcept
        {
            m_Vector = DirectX::XMVectorSubtract(m_Vector, other);
            return *this;
        }
        __forceinline Vector3& operator*=(Vector3 scalar) noexcept
        {
            m_Vector = DirectX::XMVectorMultiply(m_Vector, scalar);
            return *this;
        }
        __forceinline Vector3& operator*=(Scalar scalar) noexcept { return operator*=(Vector3{scalar}); }
        __forceinline Vector3& operator*=(float v) noexcept { return operator*=(Scalar{v}); }
        __forceinline Vector3& operator/=(Vector3 scalar) noexcept
        {
            m_Vector = DirectX::XMVectorDivide(m_Vector, scalar);
            return *this;
        }
        __forceinline Vector3& operator/=(Scalar scalar) noexcept { return operator/=(Vector3{scalar}); }
        __forceinline Vector3& operator/=(float v) noexcept { return operator/=(Scalar{v}); }
        
        __forceinline operator DirectX::XMVECTOR() const noexcept { return m_Vector; }

        static __forceinline Scalar Dot(Vector3 v0, Vector3 v1) noexcept { return Scalar{DirectX::XMVector3Dot(v0, v1)}; }
        static __forceinline Vector3 Cross(Vector3 v0, Vector3 v1) noexcept { return Vector3{DirectX::XMVector3Cross(v0, v1)}; }
		static __forceinline Vector3 Normalize(Vector3 v) noexcept { return Vector3{ DirectX::XMVector3Normalize(v) }; }
		static __forceinline Scalar Length(Vector3 v) noexcept { return Scalar{ DirectX::XMVector3Length(v) }; }
		static __forceinline Scalar LengthSquare(Vector3 v) noexcept { return Scalar{ DirectX::XMVector3LengthSq(v) }; }
		static __forceinline Scalar LengthRecip(Vector3 v) noexcept { return Scalar{ DirectX::XMVector3ReciprocalLength(v) }; }


    private:
        DirectX::XMVECTOR m_Vector{};
    };

    __forceinline Vector3 operator-(Vector3 v) noexcept { return Vector3{DirectX::XMVectorNegate(v)}; }
    __forceinline Vector3 operator+(Vector3 v0, Vector3 v1) noexcept { return v0 += v1; };
    __forceinline Vector3 operator-(Vector3 v0, Vector3 v1) noexcept { return v0 -= v1; };
    __forceinline Vector3 operator*(Vector3 v0, Vector3 v1) noexcept { return v0 *= v1; };
    __forceinline Vector3 operator*(Vector3 v0, Scalar scalar) noexcept { return v0 *= scalar; };
    __forceinline Vector3 operator*(Scalar scalar, Vector3 v) noexcept { return v *= scalar; };
    __forceinline Vector3 operator*(Vector3 v0, float scalar) noexcept { return v0 *= scalar; };
    __forceinline Vector3 operator*(float scalar, Vector3 v) noexcept { return v *= scalar; };
    __forceinline Vector3 operator/(Vector3 v0, Vector3 v1) noexcept { return v0 /= v1; };
    __forceinline Vector3 operator/(Vector3 v0, Scalar scalar) noexcept { return v0 /= scalar; };
    __forceinline Vector3 operator/(Vector3 v, float scalar) noexcept { return v /= scalar; };
    
    

    class Vector4
    {
    public:
        __forceinline Vector4() noexcept : m_Vector(DirectX::XMVectorReplicate(0)){}
        __forceinline Vector4(Scalar s) noexcept : m_Vector(s){}
        __forceinline Vector4(float x, float y, float z, float w) noexcept : m_Vector(DirectX::XMVectorSet(x, y, z, w)){}
        __forceinline Vector4(const DirectX::XMFLOAT4& f4) noexcept : m_Vector(DirectX::XMLoadFloat4(&f4)) {}
        __forceinline Vector4(Vector3 xyz, float w) noexcept : m_Vector(DirectX::XMVectorSetW(xyz, w)){}
        __forceinline explicit Vector4(DirectX::FXMVECTOR v) noexcept : m_Vector(v) {}
        __forceinline explicit Vector4(const Vector3& v3) noexcept : m_Vector(DirectX::XMVectorSetW(v3, 1)){}

        __forceinline Vector4(const Vector4& other) noexcept : m_Vector(other) {};
        __forceinline Vector4& operator=(const Vector4& other) noexcept { m_Vector = other; return *this; };

        __forceinline Scalar GetX() const noexcept { return Scalar{DirectX::XMVectorSplatX(m_Vector)}; }
        __forceinline Scalar GetY() const noexcept { return Scalar{DirectX::XMVectorSplatY(m_Vector)}; }
        __forceinline Scalar GetZ() const noexcept { return Scalar{DirectX::XMVectorSplatZ(m_Vector)}; }
        __forceinline Scalar GetW() const noexcept { return Scalar{DirectX::XMVectorSplatW(m_Vector)}; }
        // XMVectorPermute 会从两个向量中重新布局新的向量，8个标量分别对应0 - 7 
        __forceinline void SetX(Scalar x) noexcept { m_Vector = DirectX::XMVectorPermute<4,1,2,3>(m_Vector, x); }
        __forceinline void SetY(Scalar y) noexcept { m_Vector = DirectX::XMVectorPermute<0,5,2,3>(m_Vector, y); }
        __forceinline void SetZ(Scalar z) noexcept { m_Vector = DirectX::XMVectorPermute<0,1,6,3>(m_Vector, z); }
        __forceinline void SetW(Scalar w) noexcept { m_Vector = DirectX::XMVectorPermute<0,1,2,7>(m_Vector, w); }
        __forceinline void SetXYZ(Vector3 xyz) noexcept { m_Vector = DirectX::XMVectorPermute<4,5,6,3>(m_Vector, xyz); }

        
        __forceinline Vector4& operator+=(Vector4 other) noexcept
        {
            m_Vector = DirectX::XMVectorAdd(m_Vector, other);
            return *this;
        }
        __forceinline Vector4& operator-=(Vector4 other) noexcept
        {
            m_Vector = DirectX::XMVectorSubtract(m_Vector, other);
            return *this;
        }
        __forceinline Vector4& operator*=(Vector4 scalar) noexcept
        {
            m_Vector = DirectX::XMVectorMultiply(m_Vector, scalar);
            return *this;
        }
        __forceinline Vector4& operator*=(Scalar scalar) noexcept { return operator*=(Vector4{scalar}); }
        __forceinline Vector4& operator*=(float v) noexcept { return operator*=(Scalar{v}); }
        __forceinline Vector4& operator/=(Vector4 scalar) noexcept
        {
            m_Vector = DirectX::XMVectorDivide(m_Vector, scalar);
            return *this;
        }
        __forceinline Vector4& operator/=(Scalar scalar) noexcept { return operator/=(Vector4{scalar}); }
        __forceinline Vector4& operator/=(float v) noexcept { return operator/=(Scalar{v}); }
        
        __forceinline operator DirectX::XMVECTOR() const noexcept { return m_Vector; }

        __forceinline static Scalar Dot(Vector4 v0, Vector4 v1) noexcept { return Scalar{DirectX::XMVector4Dot(v0, v1)}; }
        __forceinline static Vector4 Cross(Vector4 v0, Vector4 v1, Vector4 v2) noexcept{return Vector4{DirectX::XMVector4Cross(v0, v1, v2)};}
        __forceinline static Scalar Length(Vector4 v) noexcept { return Scalar{DirectX::XMVector4Length(v)}; }
        __forceinline static Scalar LengthSquare(Vector4 v) noexcept { return Scalar{DirectX::XMVector4LengthSq(v)}; }
        __forceinline static Scalar LengthRecip(Vector4 v) noexcept { return Scalar{DirectX::XMVector4ReciprocalLength(v)}; }
        __forceinline static Vector4 Normalize(Vector4 v) noexcept { return Vector4{DirectX::XMVector4Normalize(v)}; }
        
    private:
        DirectX::XMVECTOR m_Vector{};
    };

    __forceinline Vector4 operator-(Vector4 v) noexcept { return Vector4{DirectX::XMVectorNegate(v)}; }
    __forceinline Vector4 operator+(Vector4 v0, Vector4 v1) noexcept { return v0 += v1; };
    __forceinline Vector4 operator-(Vector4 v0, Vector4 v1) noexcept { return v0 -= v1; };
    __forceinline Vector4 operator*(Vector4 v0, Vector4 v1) noexcept { return v0 *= v1; };
    __forceinline Vector4 operator*(Vector4 v0, Scalar scalar) noexcept { return v0 *= scalar; };
    __forceinline Vector4 operator*(Scalar scalar, Vector4 v) noexcept { return v *= scalar; };
    __forceinline Vector4 operator*(Vector4 v0, float scalar) noexcept { return v0 *= scalar; };
    __forceinline Vector4 operator*(float scalar, Vector4 v) noexcept { return v *= scalar; };
    __forceinline Vector4 operator/(Vector4 v0, Vector4 v1) noexcept { return v0 /= v1; };
    __forceinline Vector4 operator/(Vector4 v0, Scalar scalar) noexcept { return v0 /= scalar; };
    __forceinline Vector4 operator/(Vector4 v, float scalar) noexcept { return v /= scalar; };

    
    inline Vector3::Vector3(const Vector4& v4) noexcept
        :m_Vector(v4){
    }



}

#endif