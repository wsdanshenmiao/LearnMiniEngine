#pragma once
#ifndef __QUATERNION_H__
#define __QUATERNION_H__

#include "Vector.h"

namespace DSM::Math {
    
    class Quaternion
    {
    public:
        INLINE Quaternion() noexcept : m_Vector(DirectX::XMQuaternionIdentity()){}
        INLINE Quaternion(const Vector3& axis, const Scalar& angle) noexcept
            :m_Vector(DirectX::XMQuaternionRotationAxis(axis, angle)){}
        // 三个角分别为 俯仰角(x)、偏航角(y)、滚动角(z)
        INLINE Quaternion(float pitch, float yaw, float roll) noexcept
            :m_Vector((DirectX::XMQuaternionRotationRollPitchYaw(pitch, yaw, roll))){}
        INLINE explicit Quaternion(DirectX::FXMMATRIX matrix) noexcept : m_Vector(XMQuaternionRotationMatrix(matrix)){}
        INLINE explicit Quaternion(DirectX::FXMVECTOR vector) noexcept : m_Vector(vector){}

        INLINE Quaternion(const Quaternion& quaternion) noexcept = default;
        INLINE Quaternion& operator=(const Quaternion& quaternion) noexcept = default;

        INLINE Quaternion& operator*=(Quaternion other) noexcept
        {
            m_Vector = DirectX::XMQuaternionMultiply(m_Vector, other.m_Vector);
            return *this;
        }

        INLINE operator DirectX::XMVECTOR() const noexcept { return m_Vector; }


        INLINE static Quaternion Normalize(Quaternion q) noexcept { return Quaternion{DirectX::XMQuaternionNormalize(q)}; }
        // 使用球面线性插值来对两个四元数进行插值
        INLINE static Quaternion Slerp(Quaternion q0, Quaternion q1, float t) noexcept
        {
            return Normalize(Quaternion{DirectX::XMQuaternionSlerp(q0, q1, t)});
        }
        INLINE static Quaternion Lerp(Quaternion q0, Quaternion q1, float t) noexcept
        {
            return Normalize(Quaternion{DirectX::XMVectorLerp(q0, q1, t)});
        }
        
    private:
        DirectX::XMVECTOR m_Vector{};
    };

    INLINE Quaternion operator-(Quaternion q){ return Quaternion{DirectX::XMVectorNegate(q)}; }
    // 返回四元数的共轭
    INLINE Quaternion operator~(Quaternion q){ return Quaternion{DirectX::XMQuaternionConjugate(q)}; }
    INLINE Quaternion operator*(Quaternion q0, Quaternion q1) { return q0 *= q1; }
    // 旋转一个向量
    INLINE Vector3 operator*(Quaternion q, Vector3 v)
    {
        return Vector3{DirectX::XMVector3Rotate(v, q)};
    }
    

}


#endif