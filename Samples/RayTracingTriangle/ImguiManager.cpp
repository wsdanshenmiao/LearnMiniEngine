#include "ImguiManager.h"

using namespace DirectX;

namespace DSM {

	void ImguiManager::UpdateImGui(float time)
	{
		static float outSideCol[] = {1,1,1};
		float dt = time;
		auto& io = ImGui::GetIO();

		if (ImGui::Begin("ImGui"))
		{
			ImGui::ColorEdit3("Outside Color", outSideCol);
		}
		ImGui::End();

		cubeAlbedo = {outSideCol[0], outSideCol[1], outSideCol[2]};
	}
}
