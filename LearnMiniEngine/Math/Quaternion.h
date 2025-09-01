#pragma once
#ifndef __QUATERNION_H__
#define __QUATERNION_H__

#include <cmath>
#include "Vector.h"

namespace DSM::Math {
    
    class Quaternion
    {
    public:
        __forceinline Quaternion() noexcept : m_Vector(DirectX::XMQuaternionIdentity()){}
        __forceinline Quaternion(const Vector3& axis, const Scalar& angle) noexcept
            :m_Vector(DirectX::XMQuaternionRotationAxis(axis, angle)){}
        // 三个角分别为 俯仰角(x)、偏航角(y)、滚动角(z)
        __forceinline Quaternion(float pitch, float yaw, float roll) noexcept
            :m_Vector((DirectX::XMQuaternionRotationRollPitchYaw(pitch, yaw, roll)))
        {
            m_Vector = DirectX::XMVectorModAngles(m_Vector);
        }
        __forceinline Quaternion(const Vector3& pyr) noexcept
            :Quaternion(pyr.GetX(), pyr.GetY(), pyr.GetZ()) {}
        __forceinline explicit Quaternion(DirectX::FXMMATRIX matrix) noexcept : m_Vector(XMQuaternionRotationMatrix(matrix)){}
        __forceinline explicit Quaternion(DirectX::FXMVECTOR vector) noexcept : m_Vector(vector){}

        __forceinline Quaternion(const Quaternion& quaternion) noexcept = default;
        __forceinline Quaternion& operator=(const Quaternion& quaternion) noexcept = default;

        __forceinline Quaternion& operator*=(Quaternion other) noexcept
        {
            m_Vector = DirectX::XMQuaternionMultiply(m_Vector, other.m_Vector);
            return *this;
        }

        // 四元数转欧拉角（Pitch, Yaw, Roll），返回Vector3f，单位为弧度
        Vector3 ToEulerAngles()
        {
            float x = DirectX::XMVectorGetX(m_Vector);
            float y = DirectX::XMVectorGetY(m_Vector);
            float z = DirectX::XMVectorGetZ(m_Vector);
            float w = DirectX::XMVectorGetW(m_Vector);

            // pitch (X轴)
            float sinX = 2.0f * (w * x - y * z);
            sinX = sinX > 1.0f ? 1.0f : sinX;
            sinX = sinX < -1.0f ? -1.0f : sinX;
            float pitch = std::asin(sinX);

            // roll (Y轴)
            float sinY_cosX = 2.0f * (w * y + x * z);
            float cosY_cosX = 1.0f - 2.0f * (x * x + y * y);
            float yaw = std::atan2(sinY_cosX, cosY_cosX);

            // yaw (Z轴)
            float sinZ_cosX = 2.0f * (w * z + x * y);
            float cosZ_cosX = 1.0f - 2.0f * (x * x + z * z);
            float roll = std::atan2(sinZ_cosX, cosZ_cosX);

            return Vector3{pitch, yaw, roll};
        }

        inline Quaternion Normalized() const noexcept { Quaternion ret = *this; Normalize(ret); return ret; }

        __forceinline operator DirectX::XMVECTOR() const noexcept { return m_Vector; }


        __forceinline static Quaternion Normalize(Quaternion q) noexcept { return Quaternion{DirectX::XMQuaternionNormalize(q)}; }
        // 使用球面线性插值来对两个四元数进行插值
        __forceinline static Quaternion Slerp(Quaternion q0, Quaternion q1, float t) noexcept
        {
            return Normalize(Quaternion{DirectX::XMQuaternionSlerp(q0, q1, t)});
        }
        __forceinline static Quaternion Lerp(Quaternion q0, Quaternion q1, float t) noexcept
        {
            return Normalize(Quaternion{DirectX::XMVectorLerp(q0, q1, t)});
        }
        
    private:
        DirectX::XMVECTOR m_Vector{};
    };

    __forceinline Quaternion operator-(Quaternion q){ return Quaternion{DirectX::XMVectorNegate(q)}; }
    // 返回四元数的共轭
    __forceinline Quaternion operator~(Quaternion q){ return Quaternion{DirectX::XMQuaternionConjugate(q)}; }
    __forceinline Quaternion operator*(Quaternion q0, Quaternion q1) { return q0 *= q1; }
    // 旋转一个向量
    __forceinline Vector3 operator*(Quaternion q, Vector3 v)
    {
        return Vector3{DirectX::XMVector3Rotate(v, q)};
    }

    __forceinline bool operator==(const Quaternion& q0, const Quaternion& q1){ return DirectX::XMQuaternionEqual(q0, q1); }
    

}


#endif