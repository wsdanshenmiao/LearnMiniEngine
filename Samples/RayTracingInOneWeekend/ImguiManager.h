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
		uint32_t maxTraceRecursionDepth = 4;
		uint32_t samplesPerPixel = 5;
		Math::Vector3 backgroundColor = { 0.7f, 0.8f, 1.0f };
	};
}

#endif // !__IMGUIMANAGER__H__
