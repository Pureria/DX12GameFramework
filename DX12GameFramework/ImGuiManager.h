#pragma once
#include <wrl/client.h>
#include <d3d12.h>

#include "ImGui//imgui.h"
#include "ImGui//imgui_impl_win32.h"
#include "ImGui/imgui_impl_dx12.h"
#include "GraphicsEngine.h"

class ImGuiManager
{
private:
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> _srvHeap;

public:
	ImGuiManager() = default;
	~ImGuiManager() = default;

	bool Initialize(HWND hwnd, GraphicsEngine* engine);

	void BeginFrame();
	void Draw(GraphicsEngine* engine);

	void Finalize();
};

