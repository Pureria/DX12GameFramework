#pragma once
#include <string>
#include <vector>
#include <map>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "define.h"
#include "IModel.h"

class GraphicsEngine;

class AssimpModel : public IModel
{
private:
	// Assimp
	Assimp::Importer _importer;
	const aiScene* _scene = nullptr;
	std::string _directory;
	std::vector<ComPtr<ID3D12Resource>> _textures;
	std::vector<ComPtr<ID3D12Resource>> _textureUploadBuffers;
	std::vector<DirectX::XMFLOAT4> _materialColors;
	ComPtr<ID3D12DescriptorHeap> _srvHeap;

	// DX12 Resource
	std::vector<MeshResource> _meshResources;

	// bone info
	std::map<std::string, BoneInfo> _boneMap;
	int _boneCount = 0;

	// helper
	void ProcessNode(aiNode* node, const aiScene* scene, GraphicsEngine* engine);
	MeshResource ProcessMesh(aiMesh* mesh, const aiScene* scene, GraphicsEngine* engine);
	void LoadTexturesAndMaterials(GraphicsEngine* engine);

public:
	AssimpModel();
	~AssimpModel() override;

	// initialize
	bool Initialize(const std::string& filePath, GraphicsEngine* engine);

	// draw model
	void Draw(GraphicsEngine* engine) override;

	const std::vector<DirectX::XMFLOAT4>& GetMaterialColors() const override { return _materialColors; }
	const aiScene* GetScene() const { return _scene; }
	const std::map<std::string, BoneInfo>& GetBoneMap() const { return _boneMap; }
	int GetBoneCount() const { return _boneCount; }
};

