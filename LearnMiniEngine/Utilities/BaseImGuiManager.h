#pragma once
#ifndef __BASEIMGUIMANAGER__H__
#define __BASEIMGUIMANAGER__H__


#include "Graphics/DescriptorHeap.h"
#include <wrl/client.h>
#include "imgui.h"
#include "imgui_impl_dx12.h"
#include "imgui_impl_win32.h"
#include "Singleton.h"
#include "Core/CpuTimer.h"


namespace DSM {

	template<typename Driver>
	class BaseImGuiManager : public Singleton<Driver>
	{
	public:
		using BaseType = Singleton<Driver>;
		virtual bool InitImGui(ID3D12Device* device, HWND hMainWnd, int bufferCount, DXGI_FORMAT bufferFormat);
		void ImGuiNewFrame();
		void Update(float time);
		virtual void RenderImGui(ID3D12GraphicsCommandList* cmdList);
		const DescriptorHeap& GetDescriptorHeap() const { return m_ImGuiSrvHeap; }

	protected:
		BaseImGuiManager()
			: m_ImGuiSrvHeap(L"ImGuiSrvHeap", D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 256) {}
		virtual ~BaseImGuiManager() override;

		virtual void UpdateImGui(float time) = 0;

	protected:
		DescriptorHeap m_ImGuiSrvHeap;		// 提供给ImGui的着色器资源描述符堆
	};

	template<typename Driver>
	bool BaseImGuiManager<Driver>::InitImGui(
		ID3D12Device* device,
		HWND hMainWnd,
		int frameCount,
		DXGI_FORMAT bufferFormat)
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;	// 允许键盘控制

		ImGui::StyleColorsDark();

		auto handle = m_ImGuiSrvHeap.Allocate();

		ImGui_ImplWin32_Init(hMainWnd);
		ImGui_ImplDX12_Init(
			device,
			frameCount,
			bufferFormat,
			m_ImGuiSrvHeap.GetHeap(),
			handle,
			handle);

		return true;
	}

	template<typename Driver>
	void BaseImGuiManager<Driver>::ImGuiNewFrame()
	{
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
	}

	template<typename Driver>
	void BaseImGuiManager<Driver>::Update(float time)
	{
		ImGuiNewFrame();
		UpdateImGui(time);
	}

	template<typename Driver>
	void BaseImGuiManager<Driver>::RenderImGui(ID3D12GraphicsCommandList* cmdList)
	{
		ImGui::Render();
		auto heap = m_ImGuiSrvHeap.GetHeap();
		cmdList->SetDescriptorHeaps(1, &heap);
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList);
	}

    template<typename Driver>
	BaseImGuiManager<Driver> ::~BaseImGuiManager()
	{
		ImGui_ImplDX12_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
	}

}

#endif // !__BASEIMGUIMANAGER__H__
