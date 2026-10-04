#pragma once
#include <vector>
#include <DirectXMath.h>

class GraphicsEngine;

class IModel {
public:
	virtual ~IModel() = default;

	virtual void Draw(GraphicsEngine* engine) = 0;
	virtual const std::vector<DirectX::XMFLOAT4>& GetMaterialColors() const = 0;
};