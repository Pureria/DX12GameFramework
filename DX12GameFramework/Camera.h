#pragma once
#include <DirectXMath.h>
#include <wrl/client.h>
#include <d3d12.h>
#include "define.h"

class GraphicsEngine;

class Camera
{
private:
	DirectX::XMFLOAT3 _position;
	DirectX::XMFLOAT3 _rotation;

	DirectX::XMMATRIX _view;
	DirectX::XMMATRIX _proj;

	Microsoft::WRL::ComPtr<ID3D12Resource> _cameraConstantBuffer;
	void* _mappedData = nullptr;

	void Move();
public:
	Camera();
	~Camera();

	bool Initialize(GraphicsEngine* engine);
	void Bind(GraphicsEngine* engine);
	void Update();

	void SetPosition(float x, float y, float z);
	void SetPerspective(float fov, float aspect, float nearZ, float farZ);

	DirectX::XMMATRIX GetViewMatrix() const { return _view; }
	DirectX::XMMATRIX GetProjectionMatrix() const { return _proj; }
};

