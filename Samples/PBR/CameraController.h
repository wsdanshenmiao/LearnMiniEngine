#pragma once
#ifndef __CAMERACONTROLLER_H__
#define __CAMERACONTROLLER_H__

#include "Core/Camera.h"


namespace DSM {
        
    class CameraController
    {
    public:
        CameraController() = default;
        CameraController& operator=(const CameraController&) = delete;
        virtual ~CameraController() {}
        
        void Update(float deltaTime);

        void InitCamera(DSM::Camera* pCamera);

        void SetMouseSensitivity(float x, float y);
        void SetMoveSpeed(float speed);

    private:
        DSM::Camera* m_pCamera = nullptr;

        float m_MoveSpeed = 5.0f;
        float m_MouseSensitivityX = 0.005f;
        float m_MouseSensitivityY = 0.005f;

        float m_CurrentYaw = 0.0f;
        float m_CurrentPitch = 0.0f;

        Math::Vector3 m_MoveDir{};
        float m_MoveVelocity = 0.0f;
        float m_VelocityDrag = 0.0f;
        float m_TotalDragTimeToZero = 0.25f;
        float m_DragTimer = 0.0f;
    };
} // namespace DSM 



#endif
