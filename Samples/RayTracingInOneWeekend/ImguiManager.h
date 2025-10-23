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
		uint32_t maxTraceRecursionDepth = 5;
		uint32_t samplesPerPixel = 10;
		Math::Vector3 backgroundColor = { 0.7f, 0.8f, 1.0f };

		float focusDist = 10.0f;	// 焦距
		float defocusAngle = 0;		// 虚化角度
	};
}

#endif // !__IMGUIMANAGER__H__
