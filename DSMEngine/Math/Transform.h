#pragma once
#ifndef __TRANSFORM_H__
#define __TRANSFORM_H__

#include "Matrix.h"
#include "Quaternion.h"


namespace DSM {
    
    class Transform
    {
    public:
        Transform() noexcept = default;
        Transform(Math::Vector3 pos, Math::Vector3 scale, Math::Quaternion rot) noexcept
            :m_Position(pos), m_Scale(scale), m_Rotation(rot){}

        const Math::Vector3& GetPosition() const noexcept { return m_Position; }
        const Math::Vector3& GetScale() const noexcept { return m_Scale; }
        const Math::Quaternion& GetRotation() const noexcept { return m_Rotation; }

        Math::Vector3 GetRightAxis() const noexcept { return Math::Matrix3::GetRotate(m_Rotation).GetX(); }
        Math::Vector3 GetUpAxis() const noexcept { return Math::Matrix3::GetRotate(m_Rotation).GetY(); }
        Math::Vector3 GetForwardAxis() const noexcept { return Math::Matrix3::GetRotate(m_Rotation).GetZ(); }

        Math::Matrix4 GetLocalToWorld() const noexcept
        {
            return Math::Matrix4{DirectX::XMMatrixAffineTransformation(
                m_Scale, DirectX::g_XMZero, m_Rotation, m_Position)};
        }
        Math::Matrix4 GetWorldToLocal() const noexcept { return Math::Matrix4::Inverse(GetLocalToWorld());}

        void SetPosition(Math::Vector3 pos) noexcept { m_Position = pos; }
        void SetPosition(float x, float y, float z) noexcept { m_Position = Math::Vector3(x, y, z); }
        void SetScale(Math::Vector3 scale) noexcept { m_Scale = scale; }
        void SetScale(float x, float y, float z) noexcept { m_Scale = Math::Vector3(x, y, z); }
        void SetRotation(Math::Quaternion rot) noexcept { m_Rotation = rot; }
        void SetRotation(float x, float y, float z) noexcept { m_Rotation = Math::Quaternion{x, y, z}; }

        void Translate(Math::Vector3 translation) noexcept { m_Position += translation; }
        void Rotate(float angle, Math::Vector3 axis) noexcept
        {
            m_Rotation *= Math::Quaternion{axis, angle};
        }
        // 根据 俯仰角、偏航角、滚动角 进行旋转
        void Rotate(float pitch, float yaw, float roll) noexcept
        {
            m_Rotation *= Math::Quaternion{pitch, yaw, roll};
        }
        // 根据 俯仰角、偏航角、滚动角 进行旋转
        void Rotate(Math::Vector3 pyr)
        {
            m_Rotation *= Math::Quaternion{pyr.GetX(), pyr.GetY(), pyr.GetZ()};
        }
        // 绕特定的点进行旋转
        void Rotate(Math::Vector3 point, Math::Vector3 axis, float angle) noexcept
        {
            // 计算新的旋转
            Math::Quaternion rotate{axis, angle};
            m_Rotation = rotate * m_Rotation;
            // 计算新的位置
            // 先将向量旋转
            Math::Vector3 rotateRelativePos = rotate * (m_Position - point);
            m_Position = point + rotateRelativePos;
        }
        
        void LookAt(Math::Vector3 target, Math::Vector3 up = {0, 1, 0}) noexcept
        {
            Math::Matrix4 view{DirectX::XMMatrixLookAtLH(m_Position, target, up)};
            m_Rotation = Math::Quaternion{Math::Matrix4::Inverse(view)};
        }
        void LookTo(Math::Vector3 dir, Math::Vector3 up = {0, 1, 0}) noexcept
        {
            Math::Matrix4 view{DirectX::XMMatrixLookToLH(m_Position, dir, up)};
            m_Rotation = Math::Quaternion{Math::Matrix4::Inverse(view)};
        }

    private:
        Math::Vector3 m_Position{};
        Math::Vector3 m_Scale = {1,1,1};
        Math::Quaternion m_Rotation;
    };

}

#endif