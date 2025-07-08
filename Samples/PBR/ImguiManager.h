#pragma once
#ifndef __IMGUIMANAGER__H__
#define __IMGUIMANAGER__H__

#include "Utilities/BaseImGuiManager.h"
#include "Math/Transform.h"

namespace DSM {
	class ImguiManager : public BaseImGuiManager<ImguiManager>
	{
	protected:
		friend BaseImGuiManager::BaseType;
		ImguiManager() = default;
		virtual ~ImguiManager() = default;

		void UpdateImGui(float time) override;

	public:
		DirectX::XMFLOAT3 m_LightDir;
		DirectX::XMFLOAT3 m_LightColor;

		float m_FogStart = 10;
		float m_FogRange = 100;
		DirectX::XMFLOAT3 m_FogColor = DirectX::XMFLOAT3(1, 1, 1);

		int m_BlurCount = 1;
	};
}

#endif // !__IMGUIMANAGER__H__
