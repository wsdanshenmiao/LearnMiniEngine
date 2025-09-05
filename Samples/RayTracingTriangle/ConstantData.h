#pragma once
#ifndef  __CONSTANTDATA_H__
#define  __CONSTANTDATA_H__

#include "Math/Matrix.h"


namespace DSM {
    __declspec(align(256)) struct MeshConstants
    {
        Math::Matrix4 m_World{};
        Math::Matrix4 m_WorldIT{};
    };


    __declspec(align(256)) struct MaterialConstants
    {
        float m_BaseColor[4] = {1,1,1,1};
        float m_EmissiveColor[3] = {0,0,0};
        float m_NormalTexScale = 1;
        float m_MetallicFactor = 1;
        float m_RoughnessFactor = 1;
    };

    __declspec(align(256)) struct PassConstants
    {
        Math::Matrix4 m_View{};
        Math::Matrix4 m_ViewInv{};
        Math::Matrix4 m_Proj{};
        Math::Matrix4 m_ProjInv{};
        Math::Matrix4 m_ShadowTrans{};
        float m_CameraPos[3] = { 0,0,0 };
        float m_TotalTime;
        float m_DeltaTime;
    };
}

#endif
