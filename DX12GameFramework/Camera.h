#pragma once
#include <DirectXMath.h>

class Camera
{
private:
	DirectX::XMFLOAT3 _position;
	DirectX::XMFLOAT3 _rotation;

	DirectX::XMMATRIX _view;
	DirectX::XMMATRIX _proj;

	void Move();
public:
	Camera();
	~Camera();

	void Update();

	void SetPosition(float x, float y, float z);
	void SetPerspective(float fov, float aspect, float nearZ, float farZ);

	DirectX::XMMATRIX GetViewMatrix() const { return _view; }
	DirectX::XMMATRIX GetProjectionMatrix() const { return _proj; }
};

