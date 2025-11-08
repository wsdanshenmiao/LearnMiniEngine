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
			ImGui::SliderInt("##2", (int*)&samplesPerPixel, 1, 400);
			ImGui::Text("Focus Distance: %.2f", focusDist);
			ImGui::SliderFloat("##3", &focusDist, 0.5f, 50.0f);
			ImGui::Text("Defocus Angle: %.2f", defocusAngle);
			ImGui::SliderFloat("##4", &defocusAngle, 0.0f, 90.0f);
		}
		ImGui::End();

		if(ImGui::Begin("Fourier Transform Output")){
			if(ftOutputTex != nullptr){
				static auto handle = m_ImGuiSrvHeap.Allocate();
				ftOutputTex->CreateShaderResourceView(handle);
				ImGui::Image(ImTextureID{handle.GetGpuPtr()}, ImVec2{ 256, 256 });
			}
			if(iftOutputTex != nullptr){
				static auto handle = m_ImGuiSrvHeap.Allocate();
				iftOutputTex->CreateShaderResourceView(handle);
				ImGui::Image(ImTextureID{handle.GetGpuPtr()}, ImVec2{ 256, 256 });
			}
		}
		ImGui::End();

		backgroundColor = { backgroundColorArr[0], backgroundColorArr[1], backgroundColorArr[2] };
	}
}
