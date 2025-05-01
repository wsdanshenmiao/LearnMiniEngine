#pragma once
#ifndef __CAMERA__H__
#define __CAMERA__H__

#include <d3d12.h>
#include <DirectXCollision.h>
#include "../Math/Transform.h"

namespace DSM {
    class Camera
    {
    public:
        const Transform& GetTransform() const noexcept;
        
        Math::Matrix4 GetViewMatrix() const noexcept { return m_Transform.GetWorldToLocal(); }
        Math::Matrix4 GetProjMatrix() const noexcept
        {
            return Math::Matrix4{DirectX::XMMatrixPerspectiveFovLH(
                m_FovY, m_Aspect, m_ReversedZ ? m_FarZ : m_NearZ, m_ReversedZ ? m_NearZ : m_FarZ)};
        }
        Math::Matrix4 GetViewProjMatrix() const noexcept { return GetViewMatrix() * GetProjMatrix(); }

        Math::Vector3 GetRightAxis() const noexcept { return m_Transform.GetRightAxis(); }
        Math::Vector3 GetUpAxis() const noexcept { return m_Transform.GetUpAxis(); }
        Math::Vector3 GetLookAxis() const noexcept { return m_Transform.GetForwardAxis(); }

        const D3D12_VIEWPORT& GetViewPort() const noexcept { return m_ViewPort; }
        float GetNearZ() const noexcept { return m_NearZ; }
        float GetFarZ() const noexcept { return m_FarZ; }
        float GetFovY() const noexcept { return m_FovY; }
        float GetAspectRatio() const noexcept { return m_Aspect; }

        void SetPosition(float x, float y, float z) noexcept { m_Transform.SetPosition(x, y, z); }
        void SetPosition(Math::Vector3 position) noexcept { m_Transform.SetPosition(position); }
        void LookAt(Math::Vector3 target,Math::Vector3 up) noexcept { m_Transform.LookAt(target, up); }
        void LookTo(Math::Vector3 to, Math::Vector3 up) noexcept { m_Transform.LookTo(to, up); }
        void RotateX(float angle) noexcept { m_Transform.Rotate(angle, 0, 0); }
        void RotateY(float angle) noexcept { m_Transform.Rotate(0, angle, 0); }
        void SetFrustum(float fovY, float aspect, float nearZ, float farZ) noexcept;

        // 设置视口
        void SetViewPort(const D3D12_VIEWPORT& viewPort) noexcept { m_ViewPort = viewPort; }
        void SetViewPort(
            float topLeftX, float topLeftY,
            float width, float height,
            float minDepth = 0.0f, float maxDepth = 1.0f) noexcept
        {
            m_ViewPort = {topLeftX, topLeftY, width, height, minDepth, maxDepth};
        }

        void ReverseZ(bool enable) { m_ReversedZ = enable; }
    
    protected:
        Transform m_Transform{};
        D3D12_VIEWPORT m_ViewPort{};
        float m_NearZ = 0.0f;
        float m_FarZ = 0.0f;
        float m_Aspect = 0.0f;
        float m_FovY = 0.0f;

        bool m_ReversedZ = false;
    };

    
}

#endif