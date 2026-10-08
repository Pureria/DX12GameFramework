#include "pch.h"
#include "Actor.h"
#include "IModel.h"
#include "GraphicsEngine.h"

Actor::Actor(IModel* model)
	: _model(model)
	, _mappedData(nullptr)
{
	_position = { 0.0f, 0.0f, 0.0f };
	_rotation = { 0.0f, 0.0f, 0.0f };
	_scale = { 1.0f, 1.0f, 1.0f };
}

Actor::~Actor()
{
}

bool Actor::Initialize(GraphicsEngine* engine)
{
	UINT cbSize = (sizeof(ObjectConstantBuffer) + 255) & ~255;

	D3D12_HEAP_PROPERTIES heapProps = { D3D12_HEAP_TYPE_UPLOAD };
	D3D12_RESOURCE_DESC cbDesc = {};
	cbDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	cbDesc.Width = cbSize;
	cbDesc.Height = 1;
	cbDesc.DepthOrArraySize = 1;
	cbDesc.MipLevels = 1;
	cbDesc.SampleDesc.Count = 1;
	cbDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	auto hr = engine->GetDevice()->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &cbDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&_objectConstantBuffer));

	if (FAILED(hr)) {
		return false;
	}

	_objectConstantBuffer->Map(0, nullptr, reinterpret_cast<void**>(&_mappedData));

	return true;
}

void Actor::Update()
{
	if (!_mappedData || !_model) return;

	for (int i = 0; i < MAX_BONES; i++) {
		_mappedData->boneTransforms[i] = DirectX::XMMatrixIdentity();
	}

	for (auto& comp : _components) {
		comp->Update();
	}

	DirectX::XMMATRIX scale = DirectX::XMMatrixScaling(_scale.x, _scale.y, _scale.z);
	DirectX::XMMATRIX rot = DirectX::XMMatrixRotationRollPitchYaw(_rotation.x, _rotation.y, _rotation.z);
	DirectX::XMMATRIX trans = DirectX::XMMatrixTranslation(_position.x, _position.y, _position.z);

	// srt
	DirectX::XMMATRIX world = scale * rot * trans;

	_mappedData->world = DirectX::XMMatrixTranspose(world);

	const auto& matColors = _model->GetMaterialColors();
	for (int i = 0; i < matColors.size(); i++) {
		_mappedData->materialColors[i] = matColors[i];
	}
}

void Actor::Draw(GraphicsEngine* engine)
{
	engine->GetPipelineManager()->SetPipeline(engine->GetCommandList(), "Standard");
	engine->GetCommandList()->SetGraphicsRootConstantBufferView(1, _objectConstantBuffer->GetGPUVirtualAddress());

	if (_model) {
		_model->Draw(engine);
	}

}
