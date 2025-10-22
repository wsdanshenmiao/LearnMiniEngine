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
			ImGui::SliderInt("##1", (int*)&maxTraceRecursionDepth, 1, 32);
			ImGui::Text("Samples Per Pixel: %d", samplesPerPixel);
			ImGui::SliderInt("##2", (int*)&samplesPerPixel, 1, 100);
		}
		ImGui::End();

		backgroundColor = { backgroundColorArr[0], backgroundColorArr[1], backgroundColorArr[2] };
	}
}
