#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <string>
#include <unordered_map>

using Microsoft::WRL::ComPtr;
class GraphicsEngine;

// PSOを動的生成するための設定データ
struct PipelineDesc {
	std::wstring vsFilePath;
	std::wstring psFilePath;
	bool depthEnable = true;
	bool depthWrite = true;
	D3D12_COMPARISON_FUNC depthFunc = D3D12_COMPARISON_FUNC_LESS;
	D3D12_CULL_MODE cullMode = D3D12_CULL_MODE_BACK;
	bool isTransparent = false;

	std::string rootSignatureName = "Default";
};

// PSOと紐づくRoot Signatureの参照をセットで保持する構造体
struct PipelineData {
	ComPtr<ID3D12PipelineState> pso;
	ID3D12RootSignature* pRootSignature = nullptr;
};

class PipelineManager
{
private:
	GraphicsEngine* _graphicsEngine = nullptr;

	std::unordered_map<std::string, ComPtr<ID3D12RootSignature>> _rootSignatures;
	std::unordered_map<std::string, PipelineData> _pipelines;
	std::string _currentPipelineName = "";
	ID3D12RootSignature* _currentRootSignature = nullptr;

	ComPtr<ID3DBlob> CompileShader(const std::wstring& filePath, const char* entryPoint, const char* target);
public:
	PipelineManager() = default;
	~PipelineManager() = default;

	bool Initialize(GraphicsEngine* engine);
	bool CreatePipeline(const std::string& pipelineName, const PipelineDesc& desc);
	void SetPipeline(ID3D12GraphicsCommandList* commandList, const std::string& pipelineName);
	void ResetCurrentPipeline();

	ID3D12RootSignature* GetRootSignature(const std::string& name = "Default") const {
		auto it = _rootSignatures.find(name);
		return (it != _rootSignatures.end()) ? it->second.Get() : nullptr;
	}
};

