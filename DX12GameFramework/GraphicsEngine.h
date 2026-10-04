#pragma once
#include "define.h"

class GraphicsEngine
{
private:
	ComPtr<ID3D12Device> _device;
	ComPtr<ID3D12CommandQueue> _commandQueue;
	ComPtr<IDXGISwapChain4> _swapChain;
	ComPtr<ID3D12GraphicsCommandList> _commandList;
	ComPtr<ID3D12CommandAllocator> _commandAllocator;
	ComPtr<ID3D12DescriptorHeap> _rtvHeap;
	ComPtr<ID3D12DescriptorHeap> _dsvHeap;
	ComPtr<ID3D12Resource> _backBuffers[2];
	ComPtr<ID3D12Resource> _depthBuffer;
	ComPtr<ID3D12Fence> _fence;

	UINT64 _fenceValue = 0;
	HANDLE _fenceEvent = nullptr;
	UINT _rtvDescriptorSize = 0;

public:
	GraphicsEngine();
	~GraphicsEngine();

	bool Initialize(HWND hwnd);
	void Finalize();
	void WaitForGPU();

	void BeginFrame();
	void EndFrame();

	void ResetCommandList();

	ID3D12Device* GetDevice() const { return _device.Get(); }
	ID3D12GraphicsCommandList* GetCommandList() const { return _commandList.Get(); }
	ID3D12CommandQueue* GetCommandQueue() const { return _commandQueue.Get(); }
};

