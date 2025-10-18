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
		Math::Vector3 backgroundColor = Math::Vector3(0.7f, 0.8f, 1.0f);
		uint32_t samplePerPixel = 1;
		uint32_t maxTraceRecursionDepth = 3;
	};
}

#endif // !__IMGUIMANAGER__H__
