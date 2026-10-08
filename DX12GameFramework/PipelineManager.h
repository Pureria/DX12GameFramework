#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <string>
#include <unordered_map>

using Microsoft::WRL::ComPtr;

// PSOを動的生成するための設定データ
struct PipelineDesc {
	std::wstring vsFilePath;
	std::wstring psFilePath;
	bool depthEnable = true;
	bool depthWrite = true;
	D3D12_COMPARISON_FUNC depthFunc = D3D12_COMPARISON_FUNC_LESS;
	D3D12_CULL_MODE cullMode = D3D12_CULL_MODE_BACK;
	bool isTransparent = false;
};

class GraphicsEngine;

class PipelineManager
{
private:
	GraphicsEngine* _graphicsEngine = nullptr;
	ComPtr<ID3D12RootSignature> _rootSignature;

	std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> _pipelines;

	ComPtr<ID3DBlob> CompileShader(const std::wstring& filePath, const char* entryPoint, const char* target);
public:
	PipelineManager() = default;
	~PipelineManager() = default;

	bool Initialize(GraphicsEngine* engine);
	bool CreatePipeline(const std::string& pipelineName, const PipelineDesc& desc);
	void SetPipeline(ID3D12GraphicsCommandList* commandList, const std::string& pipelineName);

	ID3D12RootSignature* GetRootSignature() const { return _rootSignature.Get(); }
};

