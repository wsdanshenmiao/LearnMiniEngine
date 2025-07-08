#include "ImguiManager.h"

using namespace DirectX;

namespace DSM {

	void ImguiManager::UpdateImGui(float time)
	{
		static float lightDir[] = {0.57735f, -0.57735f, 0.57735f};
		static float lightColor[] = {1,1,1};
		static float fogColor[] = { 1,1,1 };
		static float fogStart = 100;
		static float fogRange = 1000;
		float dt = time;
		auto& io = ImGui::GetIO();

		if (ImGui::Begin("ImGui"))
		{
			
			ImGui::Text("Light Direction: (%.1f, %.1f, %.1f)",m_LightDir.x, m_LightDir.y, m_LightDir.z);
			ImGui::SliderFloat3("##4", lightDir, -1, 1 ,"");
			ImGui::Text("Light Color: (%.1f, %.1f, %.1f)",m_LightColor.x, m_LightColor.y, m_LightColor.z);
			ImGui::SliderFloat3("##5", lightColor, 0, 1,"");

			ImGui::Text("Fog Color: (%.1f, %.1f, %.1f)", m_FogColor.x, m_FogColor.y, m_FogColor.z);
			ImGui::SliderFloat3("##6", fogColor, 0, 1, "");
			ImGui::Text("Fog Start: %.2f", fogStart);
			ImGui::SliderFloat("##7", &fogStart, 0, 1000, "");
			ImGui::Text("Fog Range: %.2f", fogRange);
			ImGui::SliderFloat("##8", &fogRange, 0, 5000, "");

			ImGui::Text("Blur Count: %", m_BlurCount);
			ImGui::SliderInt("##9", &m_BlurCount, 0, 10, "");
		}
		ImGui::End();

		
		m_LightColor = {lightColor[0], lightColor[1], lightColor[2]};
		m_LightDir = {lightDir[0], lightDir[1], lightDir[2]};
		m_FogColor = { fogColor[0], fogColor[1], fogColor[2] };
		m_FogStart = fogStart;
		m_FogRange = fogRange;
	}
}
