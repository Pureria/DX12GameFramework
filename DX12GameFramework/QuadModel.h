#pragma once
#include <vector>
#include <DirectXMath.h>

#include "define.h"
#include "IModel.h"

class GraphicsEngine;

class QuadModel : public IModel
{
private:
	std::vector<DirectX::XMFLOAT4> _materialColors;

	MeshResource _meshResource;
	ComPtr<ID3D12Resource> _dummyTextureBuffer;
	ComPtr<ID3D12Resource> _uploadTextureBuffer;
	ComPtr<ID3D12DescriptorHeap> _srvHeap;

public:
	QuadModel();
	~QuadModel() override;

	void Initialize(GraphicsEngine* engine);

	void Draw(GraphicsEngine* engine) override;
	const std::vector<DirectX::XMFLOAT4>& GetMaterialColors() const override { return _materialColors; }
};

