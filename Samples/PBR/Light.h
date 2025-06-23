#pragma once
#ifndef __LIGHT__H__
#define __LIGHT__H__

#include <DirectXMath.h>

namespace DSM {
	struct Light
	{
		DirectX::XMFLOAT3 m_Color;
	};

	struct DirectionalLight : public Light
	{
		float m_Pad0;
		DirectX::XMFLOAT3 m_Dir;
		float m_Pad1;
	};

	struct PointLight : Light
	{
		float m_StartAtten;
		DirectX::XMFLOAT3 m_Pos;
		float m_EndAtten;
	};

	struct SpotLight : Light
	{
		float m_StartAtten;
		DirectX::XMFLOAT3 m_Pos;
		float m_EndAtten;
		DirectX::XMFLOAT3 m_Dir;
		float m_SpotPower;
	};

}

#endif // !__LIGHT__H__
