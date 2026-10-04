#include "pch.h"
#include "AssimpModel.h"
#include "GraphicsEngine.h"

AssimpModel::AssimpModel()
{
}

AssimpModel::~AssimpModel()
{
}

void AssimpModel::ProcessNode(aiNode* node, const aiScene* scene, GraphicsEngine* engine)
{
	for (unsigned int i = 0; i < node->mNumMeshes; i++) {
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

		_meshResources.push_back(ProcessMesh(mesh, scene, engine));
	}

	for (unsigned int i = 0; i < node->mNumChildren; i++) {
		ProcessNode(node->mChildren[i], scene, engine);
	}
}

MeshResource AssimpModel::ProcessMesh(aiMesh* mesh, const aiScene* scene, GraphicsEngine* engine)
{
	MeshResource meshResource;
	std::vector<Vertex> vertices;
	std::vector<uint16_t> indices;

	meshResource.materialIndex = mesh->mMaterialIndex;

	// 頂点データの抽出
	for (unsigned int v = 0; v < mesh->mNumVertices; v++) {
		Vertex vertex;
		vertex.position[0] = mesh->mVertices[v].x;
		vertex.position[1] = mesh->mVertices[v].y;
		vertex.position[2] = mesh->mVertices[v].z;

		if (mesh->HasNormals()) {
			vertex.normal[0] = mesh->mNormals[v].x;
			vertex.normal[1] = mesh->mNormals[v].y;
			vertex.normal[2] = mesh->mNormals[v].z;
		}
		else {
			vertex.normal[0] = 0.0f; vertex.normal[1] = 1.0f; vertex.normal[2] = 0.0f;
		}

		vertex.color[0] = 1.0f;
		vertex.color[1] = 1.0f;
		vertex.color[2] = 1.0f;
		vertex.color[3] = 1.0f;

		if (mesh->mTextureCoords[0]) {
			vertex.uv[0] = mesh->mTextureCoords[0][v].x;
			vertex.uv[1] = mesh->mTextureCoords[0][v].y;
		}
		else {
			vertex.uv[0] = 0.0f;
			vertex.uv[1] = 0.0f;
		}

		vertex.boneIDs[0] = 0; vertex.boneIDs[1] = 0; vertex.boneIDs[2] = 0; vertex.boneIDs[3] = 0;
		vertex.boneWeights[0] = 0.0f; vertex.boneWeights[1] = 0.0f; vertex.boneWeights[2] = 0.0f; vertex.boneWeights[3] = 0.0f;
		vertices.push_back(vertex);
	}

	// インデックスデータの抽出
	for (unsigned int f = 0; f < mesh->mNumFaces; f++) {
		aiFace face = mesh->mFaces[f];
		for (unsigned int j = 0; j < face.mNumIndices; j++) {
			indices.push_back(face.mIndices[j]);
		}
	}

	std::vector<int> vertexBoneCount(mesh->mNumVertices, 0);
	for (unsigned int b = 0; b < mesh->mNumBones; b++) {
		aiBone* bone = mesh->mBones[b];
		std::string boneName = bone->mName.C_Str();

		if (_boneMap.find(boneName) == _boneMap.end()) {
			BoneInfo newBone;
			newBone.id = _boneCount;
			newBone.offsetMatrix = bone->mOffsetMatrix;
			_boneMap[boneName] = newBone;
			_boneCount++;
		}

		int boneID = _boneMap[boneName].id;

		for (unsigned int w = 0; w < bone->mNumWeights; w++) {
			unsigned int vertexID = bone->mWeights[w].mVertexId;
			float weight = bone->mWeights[w].mWeight;

			int currentCount = vertexBoneCount[vertexID];
			if (currentCount < 4) {
				vertices[vertexID].boneIDs[currentCount] = boneID;
				vertices[vertexID].boneWeights[currentCount] = weight;
				vertexBoneCount[vertexID]++;
			}
		}

	}

	printf("メッシュ %s : ボーン数 %d （現在までの総ボーン数: %d）\n", mesh->mName.C_Str(), mesh->mNumBones, _boneCount);

	// 頂点バッファの作成とデータ転送
	UINT vertexBufferSize = static_cast<UINT>(vertices.size() * sizeof(Vertex));

	D3D12_RESOURCE_DESC vbDesc = {};
	vbDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	vbDesc.Width = vertexBufferSize;
	vbDesc.Height = 1;
	vbDesc.DepthOrArraySize = 1;
	vbDesc.MipLevels = 1;
	vbDesc.SampleDesc.Count = 1;
	vbDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	D3D12_HEAP_PROPERTIES heapProps = { D3D12_HEAP_TYPE_UPLOAD };

	engine->GetDevice()->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&vbDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&meshResource.vertexBuffer)
	);

	void* mappedData = nullptr;
	meshResource.vertexBuffer->Map(0, nullptr, &mappedData);
	memcpy(mappedData, vertices.data(), vertexBufferSize);
	meshResource.vertexBuffer->Unmap(0, nullptr);

	meshResource.vbView.BufferLocation = meshResource.vertexBuffer->GetGPUVirtualAddress();
	meshResource.vbView.SizeInBytes = vertexBufferSize;
	meshResource.vbView.StrideInBytes = sizeof(Vertex);

	// インデックスバッファの作成とデータ転送
	UINT indexBufferSize = static_cast<UINT>(indices.size() * sizeof(uint16_t));
	meshResource.indexCount = static_cast<UINT>(indices.size());

	D3D12_RESOURCE_DESC ibDesc = {};
	ibDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	ibDesc.Width = indexBufferSize;
	ibDesc.Height = 1;
	ibDesc.DepthOrArraySize = 1;
	ibDesc.MipLevels = 1;
	ibDesc.SampleDesc.Count = 1;
	ibDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	engine->GetDevice()->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&ibDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&meshResource.indexBuffer)
	);

	void* mappedIndexData = nullptr;
	meshResource.indexBuffer->Map(0, nullptr, &mappedIndexData);
	memcpy(mappedIndexData, indices.data(), indexBufferSize);
	meshResource.indexBuffer->Unmap(0, nullptr);

	meshResource.ibView.BufferLocation = meshResource.indexBuffer->GetGPUVirtualAddress();
	meshResource.ibView.SizeInBytes = indexBufferSize;
	meshResource.ibView.Format = DXGI_FORMAT_R16_UINT;

	return meshResource;
}

void AssimpModel::LoadTexturesAndMaterials(GraphicsEngine* engine)
{
	unsigned int materialCount = _scene->mNumMaterials;
	_materialColors.resize(materialCount);

	D3D12_HEAP_PROPERTIES defaultHeap = { D3D12_HEAP_TYPE_DEFAULT };
	D3D12_HEAP_PROPERTIES uploadHeap = { D3D12_HEAP_TYPE_UPLOAD };

	// ダミーテクスチャの作成
	ComPtr<ID3D12Resource> dummyTexture;
	ComPtr<ID3D12Resource> dummyUpload;
	{
		D3D12_RESOURCE_DESC dummyDesc = {};
		dummyDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		dummyDesc.Width = 1;
		dummyDesc.Height = 1;
		dummyDesc.DepthOrArraySize = 1;
		dummyDesc.MipLevels = 1;
		dummyDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		dummyDesc.SampleDesc.Count = 1;
	
		engine->GetDevice()->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &dummyDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&dummyTexture));

		UINT dummyRowPitch = 256;
		D3D12_RESOURCE_DESC upDesc = dummyDesc;
		upDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		upDesc.Width = dummyRowPitch;
		upDesc.Format = DXGI_FORMAT_UNKNOWN;
		upDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		
		engine->GetDevice()->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &upDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&dummyUpload));

		uint32_t whitePixel = 0xFFFFFFFF;
		void* mapped = nullptr;
		dummyUpload->Map(0, nullptr, &mapped);
		memcpy(mapped, &whitePixel, 4);
		dummyUpload->Unmap(0, nullptr);

		D3D12_TEXTURE_COPY_LOCATION dst = { dummyTexture.Get(), D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX, 0 };
		D3D12_TEXTURE_COPY_LOCATION src = { dummyUpload.Get(), D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT, 0 };
		src.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		src.PlacedFootprint.Footprint.Width = 1;
		src.PlacedFootprint.Footprint.Height = 1;
		src.PlacedFootprint.Footprint.Depth = 1;
		src.PlacedFootprint.Footprint.RowPitch = dummyRowPitch;

		engine->GetCommandList()->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Transition.pResource = dummyTexture.Get();
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

		engine->GetCommandList()->ResourceBarrier(1, &barrier);

		_textureUploadBuffers.push_back(dummyUpload);
	}

	// 各マテリアルの処理
	for (unsigned int i = 0; i < materialCount; i++) {
		aiMaterial* material = _scene->mMaterials[i];

		aiColor4D color(1.0f, 1.0f, 1.0f, 1.0f);
		aiGetMaterialColor(material, AI_MATKEY_COLOR_DIFFUSE, &color);
		_materialColors[i] = DirectX::XMFLOAT4(color.r, color.g, color.b, color.a);

		aiString texPath;
		if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == aiReturn_SUCCESS) {
			std::string fullPath = _directory + "/" + texPath.C_Str();

			int texWidth, texHeight, texChannels;
			stbi_uc* pixels = stbi_load(fullPath.c_str(), &texWidth, &texHeight, &texChannels, 4);

			if (pixels) {
				ComPtr<ID3D12Resource> textureResource;
				D3D12_RESOURCE_DESC texDesc = {};
				texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
				texDesc.Width = texWidth;
				texDesc.Height = texHeight;
				texDesc.DepthOrArraySize = 1;
				texDesc.MipLevels = 1;
				texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				texDesc.SampleDesc.Count = 1;

				engine->GetDevice()->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &texDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&textureResource));

				// 1行を256バイトの倍数にアライメントする
				UINT rowPitch = (texWidth * 4 + 255) & ~255;
				UINT64 uploadBufferSize = (UINT64)rowPitch * texHeight;

				ComPtr<ID3D12Resource> textureUploadBuffer;
				D3D12_RESOURCE_DESC uploadDesc = {};
				uploadDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
				uploadDesc.Width = uploadBufferSize;
				uploadDesc.Height = 1;
				uploadDesc.DepthOrArraySize = 1;
				uploadDesc.MipLevels = 1;
				uploadDesc.SampleDesc.Count = 1;
				uploadDesc.Format = DXGI_FORMAT_UNKNOWN;
				uploadDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

				engine->GetDevice()->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &uploadDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&textureUploadBuffer));

				void* mappedTexData = nullptr;
				textureUploadBuffer->Map(0, nullptr, &mappedTexData);
				for (int y = 0; y < texHeight; ++y) {
					memcpy((uint8_t*)mappedTexData + y * rowPitch, pixels + y * texWidth * 4, texWidth * 4);
				}
				textureUploadBuffer->Unmap(0, nullptr);

				D3D12_TEXTURE_COPY_LOCATION dst = { textureResource.Get(), D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX, 0 };
				D3D12_TEXTURE_COPY_LOCATION src = { textureUploadBuffer.Get(), D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT, 0 };
				src.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				src.PlacedFootprint.Footprint.Width = texWidth;
				src.PlacedFootprint.Footprint.Height = texHeight;
				src.PlacedFootprint.Footprint.Depth = 1;
				src.PlacedFootprint.Footprint.RowPitch = rowPitch;

				engine->GetCommandList()->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

				D3D12_RESOURCE_BARRIER barrier = {};
				barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
				barrier.Transition.pResource = textureResource.Get();
				barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
				barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
				barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

				engine->GetCommandList()->ResourceBarrier(1, &barrier);

				_textures.push_back(textureResource);
				_textureUploadBuffers.push_back(textureUploadBuffer);
				stbi_image_free(pixels);
			}
			else {
				_textures.push_back(dummyTexture);
			}
		}
		else {
			_textures.push_back(dummyTexture);
		}
	}

	if (_textures.size() > 0) {
		D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
		srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		srvHeapDesc.NumDescriptors = static_cast<UINT>(_textures.size());
		srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		
		engine->GetDevice()->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&_srvHeap));

		D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = _srvHeap->GetCPUDescriptorHandleForHeapStart();
		UINT srvDescriptorSize = engine->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;

		for (size_t i = 0; i < _textures.size(); i++) {
			engine->GetDevice()->CreateShaderResourceView(_textures[i].Get(), &srvDesc, srvHandle);
			srvHandle.ptr += srvDescriptorSize;
		}
	}
}

bool AssimpModel::Initialize(const std::string& filePath, GraphicsEngine* engine)
{
	_directory = filePath.substr(0, filePath.find_last_of('/'));

	//_importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);
	_scene = _importer.ReadFile(filePath.c_str(),
		aiProcess_Triangulate | aiProcess_MakeLeftHanded | aiProcess_FlipUVs);

	if (!_scene || _scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !_scene->mRootNode) {
		printf("モデルの読み込みに失敗しました: %s\n", _importer.GetErrorString());
		return false;
	}

	ProcessNode(_scene->mRootNode, _scene, engine);
	LoadTexturesAndMaterials(engine);

	return true;
}

void AssimpModel::Draw(GraphicsEngine* engine)
{
	auto cmdList = engine->GetCommandList();

	if (_srvHeap) {
		ID3D12DescriptorHeap* descriptorHeaps[] = { _srvHeap.Get() };
		cmdList->SetDescriptorHeaps(1, descriptorHeaps);
	}

	UINT srvDescriptorSize = engine->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	for (const auto& mesh : _meshResources) {
		cmdList->IASetVertexBuffers(0, 1, &mesh.vbView);
		cmdList->IASetIndexBuffer(&mesh.ibView);

		// このメッシュが使うテクスチャのGPUハンドルを計算してセット
		if (_srvHeap) {
			D3D12_GPU_DESCRIPTOR_HANDLE srvGpuHandle = _srvHeap->GetGPUDescriptorHandleForHeapStart();

			// mesh.materialIndex 分だけずらして、対応するテクスチャを見つける
			srvGpuHandle.ptr += srvDescriptorSize * mesh.materialIndex;

			// パラメータ[3]（テクスチャ）にセット
			cmdList->SetGraphicsRootDescriptorTable(3, srvGpuHandle);
		}

		cmdList->SetGraphicsRoot32BitConstants(2, 1, &mesh.materialIndex, 0);

		cmdList->DrawIndexedInstanced(mesh.indexCount, 1, 0, 0, 0);
	}
}
