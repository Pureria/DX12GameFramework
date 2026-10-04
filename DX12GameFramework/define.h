#pragma once

#include "stb_image.h"

#define MAX_BONES 256

using Microsoft::WRL::ComPtr;
using namespace DirectX;

const int WindowWidth = 1280;
const int WindowHeight = 720;

struct Vertex {
	float position[3];
	float normal[3];
	float color[4];
	float uv[2];
	unsigned int boneIDs[4];
	float boneWeights[4];
};

struct MeshResource {
	ComPtr<ID3D12Resource> vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW vbView;
	ComPtr<ID3D12Resource> indexBuffer;
	D3D12_INDEX_BUFFER_VIEW ibView;
	UINT indexCount;
	int materialIndex;
};

struct SceneConstantBuffer {
	XMMATRIX view;
	XMMATRIX proj;
};

struct ObjectConstantBuffer {
	XMMATRIX world;
	XMMATRIX boneTransforms[MAX_BONES];
	XMFLOAT4 materialColors[256];
};

struct BoneInfo {
	int id;
	aiMatrix4x4 offsetMatrix;
};