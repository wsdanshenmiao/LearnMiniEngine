#pragma once
#ifndef __MATRIX_H__
#define __MATRIX_H__

#include "Quaternion.h"
#include "MathCommon.h"
#include <array>

namespace DSM::Math {

    // 行主序的矩阵
    __declspec(align(16)) class Matrix3
    {
    public:
        __forceinline Matrix3() noexcept = default;
        __forceinline Matrix3(Vector3 x, Vector3 y, Vector3 z) noexcept : m_Matrix({x,y,z}){}
        __forceinline Matrix3(Quaternion q) noexcept { *this = Matrix3{DirectX::XMMatrixRotationQuaternion(q)}; };

        __forceinline explicit Matrix3(DirectX::FXMMATRIX other) noexcept
            :m_Matrix({Vector3{other.r[0]}, Vector3{other.r[1]}, Vector3{other.r[2]}}){}

        __forceinline Matrix3(const Matrix3& other) noexcept = default;
        __forceinline Matrix3& operator=(const Matrix3& other) noexcept = default;

        __forceinline Vector3 GetX() const noexcept{ return m_Matrix[0];}
        __forceinline Vector3 GetY() const noexcept{ return m_Matrix[1];}
        __forceinline Vector3 GetZ() const noexcept{ return m_Matrix[2];}

        __forceinline void SetX(Vector3 x) noexcept { m_Matrix[0] = x; }
        __forceinline void SetY(Vector3 y) noexcept { m_Matrix[1] = y; }
        __forceinline void SetZ(Vector3 z) noexcept { m_Matrix[2] = z; }

        __forceinline Matrix3& operator*=(Scalar s) noexcept
        {
            m_Matrix[0] *= s;
            m_Matrix[1] *= s;
            m_Matrix[2] *= s;
            return *this;
        }
        
        __forceinline operator DirectX::XMMATRIX() const noexcept
        {
            return DirectX::XMMATRIX{m_Matrix[0], m_Matrix[1], m_Matrix[2], DirectX::XMVectorZero()};
        }

        static __forceinline Matrix3 GetRotate(Quaternion q) noexcept { return Matrix3{DirectX::XMMatrixRotationQuaternion(q)}; }
        static __forceinline Matrix3 GetRotateX(float angle) noexcept { return Matrix3{DirectX::XMMatrixRotationX(angle)}; }
        static __forceinline Matrix3 GetRotateY(float angle) noexcept { return Matrix3{DirectX::XMMatrixRotationY(angle)}; }
        static __forceinline Matrix3 GetRotateZ(float angle) noexcept { return Matrix3{DirectX::XMMatrixRotationZ(angle)}; }
        static __forceinline Matrix3 GetScale(float s) noexcept { return Matrix3{DirectX::XMMatrixScaling(s, s, s)}; }
        static __forceinline Matrix3 GetScale(float x, float y, float z) noexcept { return Matrix3{DirectX::XMMatrixScaling(x, y, z)}; }
        static __forceinline Matrix3 GetScale(Vector3 s) noexcept { return Matrix3{DirectX::XMMatrixScalingFromVector(s)}; }
        static __forceinline Matrix3 Transpose(Matrix3 m) noexcept { return Matrix3{DirectX::XMMatrixTranspose(m)}; }
        static __forceinline Matrix3 InverseTranspose(Matrix3 m) noexcept
        {
            const Vector3 x = m.GetX();
            const Vector3 y = m.GetY();
            const Vector3 z = m.GetZ();

            const Vector3 inv0 = Vector3::Cross(y, z);
            const Vector3 inv1 = Vector3::Cross(z, x);
            const Vector3 inv2 = Vector3::Cross(x, y);
            const Scalar rDet = Recip(Vector3::Dot(z, inv2));

            return Matrix3{inv0, inv1, inv2} *= rDet;
        }

        static const Matrix3 Identity;
        
        
    private:
        std::array<Vector3, 3> m_Matrix;
    };

	inline const Matrix3 Matrix3::Identity = Matrix3{ Vector3{1, 0, 0}, Vector3{0, 1, 0}, Vector3{0, 0, 1} };

    __forceinline Vector3 operator*(Vector3 v, const Matrix3& m) noexcept { return Vector3{DirectX::XMVector3Transform(v, m)}; }
    // 行矩阵相乘
    __forceinline Matrix3 operator*(const Matrix3& lhs, const Matrix3& rhs) noexcept
    {
        return Matrix3{rhs.GetX() * lhs, rhs.GetY() * lhs, rhs.GetZ() * lhs};
    }
    __forceinline Matrix3 operator*(const Matrix3& m, Scalar s) noexcept { return Matrix3{m} *= s; }
    __forceinline Matrix3 operator*(Scalar s, const Matrix3& rhs) noexcept { return rhs * s; }

    __forceinline bool operator==(const Matrix3& m0, const Matrix3& m1) noexcept
    {
        BoolVector equalX = DirectX::XMVectorSetW(m0.GetX() == m1.GetX(), 0);
        BoolVector equalY = DirectX::XMVectorSetW(m0.GetY() == m1.GetY(), 0);
        BoolVector equalZ = DirectX::XMVectorSetW(m0.GetZ() == m1.GetZ(), 0);
        bool equal = (equalX == equalY) && (equalX == equalZ);
        if (equal) {
            return DirectX::XMVectorGetX(equalX) == 1 &&
                DirectX::XMVectorGetY(equalX) == 1 &&
                DirectX::XMVectorGetZ(equalX) == 1;
        }
        else {
            return false;
        }
    }




    __declspec(align(16)) class Matrix4
    {
    public:
        __forceinline Matrix4() noexcept = default;
        __forceinline Matrix4(Vector3 x, Vector3 y, Vector3 z, Vector3 w) noexcept
        {
            m_Matrix.r[0] = DirectX::XMVectorSetW(x, 0);
            m_Matrix.r[1] = DirectX::XMVectorSetW(y, 0);
            m_Matrix.r[2] = DirectX::XMVectorSetW(z, 0);
            m_Matrix.r[3] = DirectX::XMVectorSetW(w, 1);
        }
        __forceinline Matrix4(const DirectX::XMFLOAT4X4& f4x4) : m_Matrix(DirectX::XMLoadFloat4x4(&f4x4)) {}
        __forceinline Matrix4(Vector4 x, Vector4 y, Vector4 z, Vector4 w) noexcept: m_Matrix({x, y, z, w}) {}
        __forceinline Matrix4(const Matrix3& m)
        {
            m_Matrix.r[0] = DirectX::XMVectorSetW(m.GetX(), 0);
            m_Matrix.r[1] = DirectX::XMVectorSetW(m.GetY(), 0);
            m_Matrix.r[2] = DirectX::XMVectorSetW(m.GetZ(), 0);
            m_Matrix.r[3] = DirectX::XMVectorSetW(DirectX::XMVectorZero(), 1);
        }
        __forceinline Matrix4(const Matrix3& m, Vector3 w)
        {
            m_Matrix.r[0] = DirectX::XMVectorSetW(m.GetX(), 0);
            m_Matrix.r[1] = DirectX::XMVectorSetW(m.GetY(), 0);
            m_Matrix.r[2] = DirectX::XMVectorSetW(m.GetZ(), 0);
            m_Matrix.r[3] = DirectX::XMVectorSetW(w, 1);
        }
        __forceinline explicit Matrix4(DirectX::FXMMATRIX matrix) noexcept : m_Matrix(matrix) {}

        __forceinline Matrix4(const Matrix4& other) noexcept = default;
        __forceinline Matrix4& operator=(const Matrix4& other) noexcept = default;

        __forceinline Vector4 GetX() const noexcept{ return Vector4{m_Matrix.r[0]};}
        __forceinline Vector4 GetY() const noexcept{ return Vector4{m_Matrix.r[1]};}
        __forceinline Vector4 GetZ() const noexcept{ return Vector4{m_Matrix.r[2]};}
        __forceinline Vector4 GetW() const noexcept{ return Vector4{m_Matrix.r[3]};}

        __forceinline void SetX(Vector4 x) noexcept { m_Matrix.r[0] = x; }
        __forceinline void SetY(Vector4 y) noexcept { m_Matrix.r[1] = y; }
        __forceinline void SetZ(Vector4 z) noexcept { m_Matrix.r[2] = z; }
        __forceinline void SetW(Vector4 w) noexcept { m_Matrix.r[3] = w; }

        __forceinline Matrix4& operator*=(Scalar s) noexcept
        {
            m_Matrix.r[0] = DirectX::XMVectorMultiply(m_Matrix.r[0], s);
            m_Matrix.r[1] = DirectX::XMVectorMultiply(m_Matrix.r[1], s);
            m_Matrix.r[2] = DirectX::XMVectorMultiply(m_Matrix.r[2], s);
            m_Matrix.r[3] = DirectX::XMVectorMultiply(m_Matrix.r[3], s);
            return *this;
        }

        __forceinline operator DirectX::XMMATRIX() const noexcept { return m_Matrix; }

        static __forceinline Matrix4 GetRotate(Quaternion q) noexcept { return Matrix4{DirectX::XMMatrixRotationQuaternion(q)}; }
        static __forceinline Matrix4 GetRotateX(float angle) noexcept { return Matrix4{DirectX::XMMatrixRotationX(angle)}; }
        static __forceinline Matrix4 GetRotateY(float angle) noexcept { return Matrix4{DirectX::XMMatrixRotationY(angle)}; }
        static __forceinline Matrix4 GetRotateZ(float angle) noexcept { return Matrix4{DirectX::XMMatrixRotationZ(angle)}; }
        static __forceinline Matrix4 GetScale(float s) noexcept { return Matrix4{DirectX::XMMatrixScaling(s, s, s)}; }
        static __forceinline Matrix4 GetScale(float x, float y, float z) noexcept { return Matrix4{DirectX::XMMatrixScaling(x, y, z)}; }
        static __forceinline Matrix4 GetScale(Vector3 s) noexcept { return Matrix4{DirectX::XMMatrixScalingFromVector(s)}; }
        static __forceinline Matrix4 Inverse(Matrix4 m) noexcept { return Matrix4{DirectX::XMMatrixInverse(nullptr, m)}; }
        static __forceinline Matrix4 Transpose(Matrix4 m) noexcept { return Matrix4{DirectX::XMMatrixTranspose(m)}; }
        static __forceinline Matrix4 InverseTranspose(Matrix4 m) noexcept { return Transpose(Inverse(m)); }
        
		static const Matrix4 Identity;

    private:
        DirectX::XMMATRIX m_Matrix;
    };

    inline const Matrix4 Matrix4::Identity = Matrix4{ DirectX::XMMatrixIdentity() };

    __forceinline Vector4 operator*(Vector4 v, const Matrix4& m) noexcept { return Vector4{DirectX::XMVector4Transform(v, m)}; }
    // 行矩阵相乘
    __forceinline Matrix4 operator*(const Matrix4& lhs, const Matrix4& rhs) noexcept
    {
        return Matrix4{rhs.GetX() * lhs, rhs.GetY() * lhs, rhs.GetZ() * lhs, rhs.GetW() * lhs};
    }
    __forceinline Matrix4 operator*(const Matrix4& m, Scalar s) noexcept { return Matrix4{m} *= s; }
    __forceinline Matrix4 operator*(Scalar s, const Matrix4& rhs) noexcept { return rhs * s; }

    /*bool operator==(const Matrix4& m0, const Matrix4& m1) noexcept
    {
        BoolVector equalX = m0.GetX() == m1.GetX();
        BoolVector equalZ = (m0.GetZ() == m1.GetZ());
        bool equal0 = equalX == (m0.GetY() == m1.GetY());
        bool equal1 = equalZ == (m0.GetW() == m1.GetW());
        bool equal2 = equalX == equalZ;
        bool equal = equal0 && equal1 && equal2;
        if (equal) {
            return DirectX::XMVectorGetX(equalX) == 1 &&
                DirectX::XMVectorGetY(equalX) == 1 &&
                DirectX::XMVectorGetZ(equalX) == 1 &&
                DirectX::XMVectorGetW(equalX) == 1;
        }
        else {
            return false;
        }
    }*/

}

#endif