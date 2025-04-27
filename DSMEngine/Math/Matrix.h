#pragma once
#ifndef __MATRIX_H__
#define __MATRIX_H__

#include "Quaternion.h"
#include <array>

namespace DSM::Math {

    // 行主序的矩阵
    __declspec(align(16)) class Matrix3
    {
    public:
        INLINE Matrix3() noexcept = default;
        INLINE Matrix3(Vector3 x, Vector3 y, Vector3 z) noexcept : m_Matrix({x,y,z}){}
        INLINE Matrix3(Quaternion q) noexcept { *this = Matrix3{DirectX::XMMatrixRotationQuaternion(q)}; };

        INLINE explicit Matrix3(DirectX::FXMMATRIX other) noexcept
            :m_Matrix({Vector3{other.r[0]}, Vector3{other.r[1]}, Vector3{other.r[2]}}){}

        INLINE Matrix3(const Matrix3& other) noexcept = default;
        INLINE Matrix3& operator=(const Matrix3& other) noexcept = default;

        INLINE Vector3 GetX() const noexcept{ return m_Matrix[0];}
        INLINE Vector3 GetY() const noexcept{ return m_Matrix[1];}
        INLINE Vector3 GetZ() const noexcept{ return m_Matrix[2];}

        INLINE void SetX(Vector3 x) noexcept { m_Matrix[0] = x; }
        INLINE void SetY(Vector3 y) noexcept { m_Matrix[1] = y; }
        INLINE void SetZ(Vector3 z) noexcept { m_Matrix[2] = z; }

        INLINE Matrix3& operator*=(Scalar s) noexcept
        {
            m_Matrix[0] *= s;
            m_Matrix[1] *= s;
            m_Matrix[2] *= s;
            return *this;
        }
        
        INLINE operator DirectX::XMMATRIX() const noexcept
        {
            return DirectX::XMMATRIX{m_Matrix[0], m_Matrix[1], m_Matrix[2], DirectX::XMVectorZero()};
        }

        static INLINE Matrix3 GetRotate(Quaternion q) noexcept { return Matrix3{DirectX::XMMatrixRotationQuaternion(q)}; }
        static INLINE Matrix3 GetRotateX(float angle) noexcept { return Matrix3{DirectX::XMMatrixRotationX(angle)}; }
        static INLINE Matrix3 GetRotateY(float angle) noexcept { return Matrix3{DirectX::XMMatrixRotationY(angle)}; }
        static INLINE Matrix3 GetRotateZ(float angle) noexcept { return Matrix3{DirectX::XMMatrixRotationZ(angle)}; }
        static INLINE Matrix3 GetScale(float s) noexcept { return Matrix3{DirectX::XMMatrixScaling(s, s, s)}; }
        static INLINE Matrix3 GetScale(float x, float y, float z) noexcept { return Matrix3{DirectX::XMMatrixScaling(x, y, z)}; }
        static INLINE Matrix3 GetScale(Vector3 s) noexcept { return Matrix3{DirectX::XMMatrixScalingFromVector(s)}; }
        static INLINE Matrix3 Inverse(Matrix3 m) noexcept { return Matrix3{DirectX::XMMatrixInverse(nullptr, m)}; }
        static INLINE Matrix3 Transpose(Matrix3 m) noexcept { return Matrix3{DirectX::XMMatrixTranspose(m)}; }
        
    private:
        std::array<Vector3, 3> m_Matrix;
    };

    INLINE Vector3 operator*(Vector3 v, const Matrix3& m) noexcept { return Vector3{DirectX::XMVector3Transform(v, m)}; }
    // 行矩阵相乘
    INLINE Matrix3 operator*(const Matrix3& lhs, const Matrix3& rhs) noexcept
    {
        return Matrix3{rhs.GetX() * lhs, rhs.GetY() * lhs, rhs.GetZ() * lhs};
    }
    INLINE Matrix3 operator*(const Matrix3& m, Scalar s) noexcept { return Matrix3{m} *= s; }
    INLINE Matrix3 operator*(Scalar s, const Matrix3& rhs) noexcept { return rhs * s; }




    __declspec(align(16)) class Matrix4
    {
    public:
        INLINE Matrix4() noexcept = default;
        INLINE Matrix4(Vector3 x, Vector3 y, Vector3 z, Vector3 w) noexcept
        {
            m_Matrix.r[0] = DirectX::XMVectorSetW(x, 0);
            m_Matrix.r[1] = DirectX::XMVectorSetW(y, 0);
            m_Matrix.r[2] = DirectX::XMVectorSetW(z, 0);
            m_Matrix.r[3] = DirectX::XMVectorSetW(w, 1);
        }
        INLINE Matrix4(const DirectX::XMFLOAT4X4& f4x4) : m_Matrix(DirectX::XMLoadFloat4x4(&f4x4)) {}
        INLINE Matrix4(Vector4 x, Vector4 y, Vector4 z, Vector4 w) noexcept: m_Matrix({x, y, z, w}) {}
        INLINE Matrix4(const Matrix3& m)
        {
            m_Matrix.r[0] = DirectX::XMVectorSetW(m.GetX(), 0);
            m_Matrix.r[1] = DirectX::XMVectorSetW(m.GetY(), 0);
            m_Matrix.r[2] = DirectX::XMVectorSetW(m.GetZ(), 0);
            m_Matrix.r[3] = DirectX::XMVectorSetW(DirectX::XMVectorZero(), 1);
        }
        INLINE Matrix4(const Matrix3& m, Vector3 w)
        {
            m_Matrix.r[0] = DirectX::XMVectorSetW(m.GetX(), 0);
            m_Matrix.r[1] = DirectX::XMVectorSetW(m.GetY(), 0);
            m_Matrix.r[2] = DirectX::XMVectorSetW(m.GetZ(), 0);
            m_Matrix.r[3] = DirectX::XMVectorSetW(w, 1);
        }
        INLINE explicit Matrix4(DirectX::FXMMATRIX matrix) noexcept : m_Matrix(matrix) {}

        INLINE Matrix4(const Matrix4& other) noexcept = default;
        INLINE Matrix4& operator=(const Matrix4& other) noexcept = default;

        INLINE Vector4 GetX() const noexcept{ return Vector4{m_Matrix.r[0]};}
        INLINE Vector4 GetY() const noexcept{ return Vector4{m_Matrix.r[1]};}
        INLINE Vector4 GetZ() const noexcept{ return Vector4{m_Matrix.r[2]};}
        INLINE Vector4 GetW() const noexcept{ return Vector4{m_Matrix.r[3]};}

        INLINE void SetX(Vector4 x) noexcept { m_Matrix.r[0] = x; }
        INLINE void SetY(Vector4 y) noexcept { m_Matrix.r[1] = y; }
        INLINE void SetZ(Vector4 z) noexcept { m_Matrix.r[2] = z; }
        INLINE void SetW(Vector4 w) noexcept { m_Matrix.r[3] = w; }

        INLINE Matrix4& operator*=(Scalar s) noexcept
        {
            m_Matrix.r[0] = DirectX::XMVectorMultiply(m_Matrix.r[0], s);
            m_Matrix.r[1] = DirectX::XMVectorMultiply(m_Matrix.r[1], s);
            m_Matrix.r[2] = DirectX::XMVectorMultiply(m_Matrix.r[2], s);
            m_Matrix.r[3] = DirectX::XMVectorMultiply(m_Matrix.r[3], s);
            return *this;
        }

        INLINE operator DirectX::XMMATRIX() const noexcept { return m_Matrix; }

        static INLINE Matrix4 GetRotate(Quaternion q) noexcept { return Matrix4{DirectX::XMMatrixRotationQuaternion(q)}; }
        static INLINE Matrix4 GetRotateX(float angle) noexcept { return Matrix4{DirectX::XMMatrixRotationX(angle)}; }
        static INLINE Matrix4 GetRotateY(float angle) noexcept { return Matrix4{DirectX::XMMatrixRotationY(angle)}; }
        static INLINE Matrix4 GetRotateZ(float angle) noexcept { return Matrix4{DirectX::XMMatrixRotationZ(angle)}; }
        static INLINE Matrix4 GetScale(float s) noexcept { return Matrix4{DirectX::XMMatrixScaling(s, s, s)}; }
        static INLINE Matrix4 GetScale(float x, float y, float z) noexcept { return Matrix4{DirectX::XMMatrixScaling(x, y, z)}; }
        static INLINE Matrix4 GetScale(Vector3 s) noexcept { return Matrix4{DirectX::XMMatrixScalingFromVector(s)}; }
        static INLINE Matrix4 Inverse(Matrix4 m) noexcept { return Matrix4{DirectX::XMMatrixInverse(nullptr, m)}; }
        static INLINE Matrix4 Transpose(Matrix4 m) noexcept { return Matrix4{DirectX::XMMatrixTranspose(m)}; }
        
    private:
        DirectX::XMMATRIX m_Matrix;
    };

    INLINE Vector4 operator*(Vector4 v, const Matrix4& m) noexcept { return Vector4{DirectX::XMVector4Transform(v, m)}; }
    // 行矩阵相乘
    INLINE Matrix4 operator*(const Matrix4& lhs, const Matrix4& rhs) noexcept
    {
        return Matrix4{rhs.GetX() * lhs, rhs.GetY() * lhs, rhs.GetZ() * lhs, rhs.GetW() * lhs};
    }
    INLINE Matrix4 operator*(const Matrix4& m, Scalar s) noexcept { return Matrix4{m} *= s; }
    INLINE Matrix4 operator*(Scalar s, const Matrix4& rhs) noexcept { return rhs * s; }

}

#endif