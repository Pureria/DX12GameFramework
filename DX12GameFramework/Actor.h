#pragma once
#include <DirectXMath.h>
#include <vector>
#include <memory>
#include "define.h"
#include "BaseComponent.h"

class IModel;
class GraphicsEngine;

class Actor
{
private:
	IModel* _model;

	DirectX::XMFLOAT3 _position;
	DirectX::XMFLOAT3 _rotation;
	DirectX::XMFLOAT3 _scale;

	ComPtr<ID3D12Resource> _objectConstantBuffer;
	ObjectConstantBuffer* _mappedData;

	std::vector<std::unique_ptr<BaseComponent>> _components;
public:
	Actor(IModel* model);
	~Actor();

	bool Initialize(GraphicsEngine* engine);
	void Update();
	void Draw(GraphicsEngine* engine);

	void SetPosition(float x, float y, float z) { _position = { x, y, z }; }
	void SetRotation(float x, float y, float z) { _rotation = { x, y, z }; }
	void SetScale(float x, float y, float z) { _scale = { x, y, z }; }
	void SetBoneTransform(int index, const DirectX::XMMATRIX& matrix) {
		if (_mappedData && index < MAX_BONES) {
			_mappedData->boneTransforms[index] = matrix;
		}
	}

	template<typename T, typename... Args>
	T* AddComponent(Args&&... args) {
		auto comp = std::make_unique<T>(this, std::forward<Args>(args)...);
		T* ptr = comp.get();
		_components.push_back(std::move(comp));
		return ptr;
	}

	IModel* GetModel() const { return _model; }
};

