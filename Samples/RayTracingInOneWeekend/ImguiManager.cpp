#include "ImguiManager.h"

using namespace DirectX;

namespace DSM {

	void ImguiManager::UpdateImGui(float time)
	{
		static std::array<float, 3> bgColor{};
		float dt = time;
		auto& io = ImGui::GetIO();

		if (ImGui::Begin("ImGui"))
		{
			ImGui::ColorEdit3("Background Color", bgColor.data());
			ImGui::Text("Sample Per Pixel: %d", samplePerPixel);
			ImGui::SliderInt("##1", (int*)&samplePerPixel, 1, 100, "");
			ImGui::Text("Max Trace Recursion Depth: %d", maxTraceRecursionDepth);
			ImGui::SliderInt("##2", (int*)&maxTraceRecursionDepth, 1, 32, "");
		}
		ImGui::End();

		backgroundColor = Math::Vector3{ bgColor[0], bgColor[1], bgColor[2] };
	}
}
