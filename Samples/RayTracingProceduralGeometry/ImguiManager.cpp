#include "ImguiManager.h"

using namespace DirectX;

namespace DSM {

	void ImguiManager::UpdateImGui(float time)
	{
		static float albedo[] = {1,0.8,0.8};
		static float _lightDir[] = {-0.5,-1,0.05};
		static float lightCol[] = {1,1,1};
		static int gloss = 90;
		float dt = time;
		auto& io = ImGui::GetIO();

		if (ImGui::Begin("ImGui"))
		{
			ImGui::ColorEdit3("Albedo", albedo);
			ImGui::ColorEdit3("Light Color", lightCol);
			ImGui::Text("Light Direction: %.2f, %.2f, %.2f", _lightDir[0], _lightDir[1], _lightDir[2]);
			ImGui::SliderFloat3("##0", _lightDir, -1, 1, "");
			ImGui::Text("Gloss: %d", gloss);
			ImGui::SliderInt("##1", &gloss, 0, 255, "");
		}
		ImGui::End();

		cubeAlbedo = {albedo[0], albedo[1], albedo[2]};
		lightColor = {lightCol[0], lightCol[1], lightCol[2]};
		lightDir = {_lightDir[0], _lightDir[1], _lightDir[2]};
		cubeGloss = gloss;
	}
}
