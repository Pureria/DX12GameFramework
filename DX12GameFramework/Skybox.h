#pragma once
#include "GraphicsEngine.h"

class Skybox
{
private:
	ComPtr<ID3D12PipelineState> _pipelineState;
	ComPtr<ID3D12Resource> _vertexBuffer;
	ComPtr<ID3D12Resource> _indexBuffer;

	D3D12_VERTEX_BUFFER_VIEW _vbView;
	D3D12_INDEX_BUFFER_VIEW _ibView;

	ComPtr<ID3D12Resource> _texture;
	ComPtr<ID3D12Resource> _uploadBuffer;
	ComPtr<ID3D12DescriptorHeap> _srvHeap;

public:
	bool Initialize(GraphicsEngine* engine, ID3D12RootSignature* rootSignature, const char* texturePaths[6]);
	void Draw(GraphicsEngine* engine);
};

