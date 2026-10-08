#include "pch.h"
#include "GraphicsEngine.h"

GraphicsEngine::GraphicsEngine()
{
}

GraphicsEngine::~GraphicsEngine()
{
	Finalize();
}

bool GraphicsEngine::Initialize(HWND hwnd)
{
	HRESULT hr;

#ifdef _DEBUG
	// Debug Layerの有効化
	// Device作成より前に有効化する必要がある
	ComPtr<ID3D12Debug> debugController;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
	{
		debugController->EnableDebugLayer();
	}
#endif

	ComPtr<IDXGIFactory6> factory;
	UINT dxgiFactoryFlags = 0;

#ifdef _DEBUG
	dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

	// DXGI Factoryの作成
	hr = CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory));
	if (FAILED(hr)) return false;

	// Adapter（GPU）の選択
	ComPtr<IDXGIAdapter> adapter;
	hr = factory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter));
	if (FAILED(hr)) return false;

	// Deviceの作成
	hr = D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&_device));
	if (FAILED(hr)) return false;

	// Command Queueの作成
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;  // 描画コマンド用キュー
	hr = _device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&_commandQueue));
	if (FAILED(hr)) return false;

	// Swap Chainの作成
	ComPtr<IDXGISwapChain1> swapChain1;
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Width = 1280;
	swapChainDesc.Height = 720;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;					// 各チャンネル8bit(RGBA)
	swapChainDesc.SampleDesc.Count = 1;									// MSAAなし
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 2;										// ダブルバッファリング
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;			// DX12ではこれ一択

	hr = factory->CreateSwapChainForHwnd(_commandQueue.Get(), hwnd, &swapChainDesc, nullptr, nullptr, &swapChain1);
	if (FAILED(hr)) return false;

	swapChain1.As(&_swapChain);

	// Descriptor Heap の作成（RTV用）
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.NumDescriptors = 2;

	hr = _device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&_rtvHeap));
	if (FAILED(hr)) return false;
	_rtvDescriptorSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// RTVの作成
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = _rtvHeap->GetCPUDescriptorHandleForHeapStart();
	for (UINT i = 0; i < 2; i++) {
		_swapChain->GetBuffer(i, IID_PPV_ARGS(&_backBuffers[i]));
		_device->CreateRenderTargetView(_backBuffers[i].Get(), nullptr, rtvHandle);
		rtvHandle.ptr += _rtvDescriptorSize;
	}

	// DSV(Depth Stencil View)用ディスクリプタヒープの作成
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	hr = _device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&_dsvHeap));
	if (FAILED(hr)) return false;

	// 深度バッファ用リソース（テクスチャ）の作成
	D3D12_HEAP_PROPERTIES depthHeapProp = {};
	depthHeapProp.Type = D3D12_HEAP_TYPE_DEFAULT;					// 高速なVRAM上に配置

	D3D12_RESOURCE_DESC depthResDesc = {};
	depthResDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;	// 2Dテクスチャとして扱う
	depthResDesc.Width = WindowWidth;
	depthResDesc.Height = WindowHeight;
	depthResDesc.DepthOrArraySize = 1;
	depthResDesc.MipLevels = 0;
	depthResDesc.Format = DXGI_FORMAT_D32_FLOAT;					// 高精度な32bit浮動小数点
	depthResDesc.SampleDesc.Count = 1;
	depthResDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;	// 深度バッファとして使用するフラグ

	// クリア時の初期値（1.0 = 一番奥）
	D3D12_CLEAR_VALUE depthClearValue = {};
	depthClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	depthClearValue.DepthStencil.Depth = 1.0f;
	depthClearValue.DepthStencil.Stencil = 0;

	hr = _device->CreateCommittedResource(
		&depthHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&depthResDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE, // 初回から書き込み可能状態にする
		&depthClearValue,
		IID_PPV_ARGS(&_depthBuffer));
	if (FAILED(hr)) return false;

	// DSV（Depth Stencil View）の作成
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

	_device->CreateDepthStencilView(_depthBuffer.Get(), &dsvDesc, _dsvHeap->GetCPUDescriptorHandleForHeapStart());

	// Command Allocator の作成
	hr = _device->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,								// Command Queueと同じタイプにする
		IID_PPV_ARGS(&_commandAllocator));
	if (FAILED(hr)) return false;


	// Command List の作成
	_device->CreateCommandList(
		0,															// ノードマスク（GPU1つなら0）
		D3D12_COMMAND_LIST_TYPE_DIRECT,								// 描画コマンド用
		_commandAllocator.Get(),									// 紐づけるAllocator
		nullptr,													// 初期PSO（今はなし）
		IID_PPV_ARGS(&_commandList));
	if (FAILED(hr)) return false;

	// Fence の作成（CPU-GPU同期用）
	_device->CreateFence(
		0,                          // 初期値
		D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(&_fence));

	_fenceValue = 0;
	_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

	if (!_pipelineManager.Initialize(this)) {
		printf("Failed to initialize PipelineManager\n");
		return false;
	}

	return true;
}

void GraphicsEngine::Finalize()
{
	if (_fenceEvent) {
		CloseHandle(_fenceEvent);
		_fenceEvent = nullptr;
	}
}

void GraphicsEngine::BeginFrame()
{
	// コマンドリストの記録をリセット
	ResetCommandList();

	// 現在のバックバッファのインデックス（0 または 1）を取得
	UINT backBufferindex = _swapChain->GetCurrentBackBufferIndex();

	// バッファを「表示用」から「描画用」へ切り替える
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = _backBuffers[backBufferindex].Get();
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	_commandList->ResourceBarrier(1, &barrier);

	// 描画先をセットし、指定色でクリア
	D3D12_CPU_DESCRIPTOR_HANDLE rtvH = _rtvHeap->GetCPUDescriptorHandleForHeapStart();
	rtvH.ptr += backBufferindex * _rtvDescriptorSize;

	// 描画先のセット（RTVと同時にDSVもセットする）
	D3D12_CPU_DESCRIPTOR_HANDLE dsvH = _dsvHeap->GetCPUDescriptorHandleForHeapStart();
	_commandList->OMSetRenderTargets(1, &rtvH, FALSE, &dsvH);

	// 画面の色をクリア
	float clearColor[] = { 0.392f, 0.584f, 0.928f, 1.0f };
	_commandList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);

	// 深度バッファも毎フレームクリアする（1.0 = 一番奥）
	_commandList->ClearDepthStencilView(dsvH, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
}

void GraphicsEngine::EndFrame()
{
	// バッファを「描画用」から「表示用」に戻す
	UINT backBufferindex = _swapChain->GetCurrentBackBufferIndex();
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = _backBuffers[backBufferindex].Get();
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	_commandList->ResourceBarrier(1, &barrier);

	// 記録終了と実行
	_commandList->Close();
	ID3D12CommandList* ppCommandListsInit[] = { _commandList.Get() };
	_commandQueue->ExecuteCommandLists(1, ppCommandListsInit);

	// 画面のフリップとCPU-GPU同期
	_swapChain->Present(1, 0);

	_fenceValue++;
	_commandQueue->Signal(_fence.Get(), _fenceValue);
	if (_fence->GetCompletedValue() < _fenceValue) {
		_fence->SetEventOnCompletion(_fenceValue, _fenceEvent);
		WaitForSingleObject(_fenceEvent, INFINITE);
	}
}

void GraphicsEngine::ResetCommandList()
{
	_commandAllocator->Reset();
	_commandList->Reset(_commandAllocator.Get(), nullptr);
	_pipelineManager.ResetCurrentPipeline();
}

void GraphicsEngine::WaitForGPU()
{
	_commandList->Close();
	ID3D12CommandList* ppCommandListsInit[] = { _commandList.Get() };
	_commandQueue->ExecuteCommandLists(1, ppCommandListsInit);

	_fenceValue++;
	_commandQueue->Signal(_fence.Get(), _fenceValue);
	if (_fence->GetCompletedValue() < _fenceValue) {
		_fence->SetEventOnCompletion(_fenceValue, _fenceEvent);
		WaitForSingleObject(_fenceEvent, INFINITE);
	}
}
