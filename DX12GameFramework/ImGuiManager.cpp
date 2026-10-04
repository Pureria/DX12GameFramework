#include "pch.h"
#include "ImGuiManager.h"

bool ImGuiManager::Initialize(HWND hwnd, GraphicsEngine* engine)
{
	HRESULT hr;

	D3D12_DESCRIPTOR_HEAP_DESC imguiHeapDesc = {};
	imguiHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	imguiHeapDesc.NumDescriptors = 1;
	imguiHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	
	hr = engine->GetDevice()->CreateDescriptorHeap(&imguiHeapDesc, IID_PPV_ARGS(&_srvHeap));
	if (FAILED(hr)) { return false; }

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	ImGui_ImplWin32_Init(hwnd);

	ImGui_ImplDX12_InitInfo initInfo = {};
	initInfo.Device = engine->GetDevice();
	initInfo.CommandQueue = engine->GetCommandQueue();
	initInfo.NumFramesInFlight = 2;
	initInfo.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	initInfo.DSVFormat = DXGI_FORMAT_D32_FLOAT;

	initInfo.SrvDescriptorHeap = _srvHeap.Get();
	initInfo.LegacySingleSrvCpuDescriptor = _srvHeap->GetCPUDescriptorHandleForHeapStart();
	initInfo.LegacySingleSrvGpuDescriptor = _srvHeap->GetGPUDescriptorHandleForHeapStart();

	ImGui_ImplDX12_Init(&initInfo);

	return true;
}

void ImGuiManager::BeginFrame()
{
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void ImGuiManager::Draw(GraphicsEngine* engine)
{
	ImGui::Render();

	auto cmdList = engine->GetCommandList();

	ID3D12DescriptorHeap* descriptorHeaps[] = { _srvHeap.Get() };
	cmdList->SetDescriptorHeaps(1, descriptorHeaps);

	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList);
}

void ImGuiManager::Finalize()
{
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}
