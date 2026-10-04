#include "pch.h"
#include "Camera.h"
#include "Time.h"

Camera::Camera()
{
	_position = DirectX::XMFLOAT3(0.0f, 0.0f, -5.0f);
	_rotation = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
	_view = DirectX::XMMatrixIdentity();
	_proj = DirectX::XMMatrixIdentity();
}

Camera::~Camera(){ }

void Camera::Update() {
	Move();

	//targetを決めてviewを作るのではなく自分の位置と自分の回転角度（向いている方向）からview行列を作る

	// カメラの回転角度（Pitch=X軸, Yaw=Y軸, Roll=Z軸）から回転行列を作成
	DirectX::XMMATRIX rotationMatrix = DirectX::XMMatrixRotationRollPitchYaw(_rotation.x, _rotation.y, _rotation.z);

	// 基本となる「前方（Z+）」と「上方（Y+）」のベクトルを定義
	DirectX::XMVECTOR defaultForward = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	DirectX::XMVECTOR defaultUp = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	// 基本ベクトルを回転行列で回し、現在のカメラが向いている「前方」と「上方」を算出する
	DirectX::XMVECTOR forward = XMVector3TransformNormal(defaultForward, rotationMatrix);
	DirectX::XMVECTOR up = XMVector3TransformNormal(defaultUp, rotationMatrix);

	// XMFLOAT3（ただの構造体）から XMVECTOR（SIMD計算用）へ変換
	DirectX::XMVECTOR position = XMLoadFloat3(&_position);

	// カメラの位置、向いている方向、上方向を元にView（ビュー）行列を作成
	_view = DirectX::XMMatrixLookToLH(position, forward, up);
}

void Camera::SetPosition(float x, float y, float z)
{
	_position = DirectX::XMFLOAT3(x, y, z);
}

void Camera::SetPerspective(float fov, float aspect, float nearZ, float farZ)
{
	_proj = DirectX::XMMatrixPerspectiveFovLH(fov, aspect, nearZ, farZ);
}

void Camera::Move()
{
	// 移動速度と回転速度の設定
	float moveSpeed = 5.0f * Time::GetDeltaTime();
	float turnSpeed = 1.0f * Time::GetDeltaTime();

	// 矢印キーによる視点の回転（Pitch:上下, Yaw:左右）
	if (GetAsyncKeyState(VK_LEFT) & 0x8000) _rotation.y -= turnSpeed;
	if (GetAsyncKeyState(VK_RIGHT) & 0x8000) _rotation.y += turnSpeed;
	if (GetAsyncKeyState(VK_UP) & 0x8000) _rotation.x -= turnSpeed;
	if (GetAsyncKeyState(VK_DOWN) & 0x8000) _rotation.x += turnSpeed;

	// 現在の回転角度から、カメラの「前方」と「右方」のベクトルを計算
	DirectX::XMMATRIX rotationMatrix = DirectX::XMMatrixRotationRollPitchYaw(_rotation.x, _rotation.y, _rotation.z);
	DirectX::XMVECTOR forward = DirectX::XMVector3TransformNormal(DirectX::XMVectorSet(0, 0, 1, 0), rotationMatrix);
	DirectX::XMVECTOR right = DirectX::XMVector3TransformNormal(DirectX::XMVectorSet(1, 0, 0, 0), rotationMatrix);

	// 計算しやすくするために XMVECTOR から XMFLOAT3（ただの構造体）に変換
	DirectX::XMFLOAT3 fwd, rgt;
	DirectX::XMStoreFloat3(&fwd, forward);
	DirectX::XMStoreFloat3(&rgt, right);

	// WASDキーによる移動（計算した「前方」と「右方」ベクトルを使って移動座標を加算する）
	if (GetAsyncKeyState('W') & 0x8000) {
		_position.x += fwd.x * moveSpeed;
		_position.y += fwd.y * moveSpeed;
		_position.z += fwd.z * moveSpeed;
	}

	if (GetAsyncKeyState('S') & 0x8000) {
		_position.x -= fwd.x * moveSpeed;
		_position.y -= fwd.y * moveSpeed;
		_position.z -= fwd.z * moveSpeed;
	}

	if (GetAsyncKeyState('D') & 0x8000) {
		_position.x += rgt.x * moveSpeed;
		_position.y += rgt.y * moveSpeed;
		_position.z += rgt.z * moveSpeed;
	}

	if (GetAsyncKeyState('A') & 0x8000) {
		_position.x -= rgt.x * moveSpeed;
		_position.y -= rgt.y * moveSpeed;
		_position.z -= rgt.z * moveSpeed;
	}

	// Q/Eキーによる絶対的な上下移動（ワールドのY軸に対して直接移動）
	if (GetAsyncKeyState('Q') & 0x8000) {
		_position.y += moveSpeed;
	}

	if (GetAsyncKeyState('E') & 0x8000) {
		_position.y -= moveSpeed;
	}
}