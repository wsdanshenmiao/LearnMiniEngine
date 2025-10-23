#include "ImguiManager.h"

using namespace DirectX;

namespace DSM {

	void ImguiManager::UpdateImGui(float time)
	{
		float backgroundColorArr[3] = { backgroundColor.GetX(), backgroundColor.GetY(), backgroundColor.GetZ() };
		float dt = time;
		auto& io = ImGui::GetIO();

		if (ImGui::Begin("ImGui"))
		{
			ImGui::ColorEdit3("Background Color", backgroundColorArr);
			ImGui::Text("Max Trace Depth: %d", maxTraceRecursionDepth);
			ImGui::SliderInt("##1", (int*)&maxTraceRecursionDepth, 1, 50);
			ImGui::Text("Samples Per Pixel: %d", samplesPerPixel);
			ImGui::SliderInt("##2", (int*)&samplesPerPixel, 1, 2000);
			ImGui::Text("Focus Distance: %.2f", focusDist);
			ImGui::SliderFloat("##3", &focusDist, 0.5f, 50.0f);
			ImGui::Text("Defocus Angle: %.2f", defocusAngle);
			ImGui::SliderFloat("##4", &defocusAngle, 0.0f, 90.0f);
		}
		ImGui::End();

		backgroundColor = { backgroundColorArr[0], backgroundColorArr[1], backgroundColorArr[2] };
	}
}
