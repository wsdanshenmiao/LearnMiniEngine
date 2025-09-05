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
		Math::Vector3 cubeAlbedo;
	};
}

#endif // !__IMGUIMANAGER__H__
