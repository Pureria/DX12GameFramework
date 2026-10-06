#include "pch.h"
#include "Skybox.h"

bool Skybox::Initialize(GraphicsEngine* engine, ID3D12RootSignature* rootSignature, const char* texturePaths[6])
{
	// Skybox Shader Compile
	ComPtr<ID3DBlob> skyboxVsBlob;
	ComPtr<ID3DBlob> skyboxPsBlob;
	ComPtr<ID3DBlob> errorBlob;

	HRESULT hr;
	hr = D3DCompileFromFile(
		L"skybox.hlsl",
		nullptr,
		nullptr,
		"VSMain",
		"vs_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		&skyboxVsBlob,
		&errorBlob
	);

	if (FAILED(hr)) {
		if (errorBlob) {
			printf("Skybox VS Compile Error: %s\n", (char*)errorBlob->GetBufferPointer());
		}
		return false;
	}

	hr = D3DCompileFromFile(
		L"skybox.hlsl",
		nullptr,
		nullptr,
		"PSMain",
		"ps_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		&skyboxPsBlob,
		&errorBlob
	);

	if (FAILED(hr)) {
		if (errorBlob) {
			printf("Skybox PS Compile Error: %s\n", (char*)errorBlob->GetBufferPointer());
		}
		return false;
	}

	D3D12_INPUT_ELEMENT_DESC skyboxInputLayout[] = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC skyboxPsoDesc = {};
	skyboxPsoDesc.VS.pShaderBytecode = skyboxVsBlob->GetBufferPointer();
	skyboxPsoDesc.VS.BytecodeLength = skyboxVsBlob->GetBufferSize();
	skyboxPsoDesc.PS.pShaderBytecode = skyboxPsBlob->GetBufferPointer();
	skyboxPsoDesc.PS.BytecodeLength = skyboxPsBlob->GetBufferSize();
	skyboxPsoDesc.InputLayout.pInputElementDescs = skyboxInputLayout;
	skyboxPsoDesc.InputLayout.NumElements = _countof(skyboxInputLayout);
	skyboxPsoDesc.pRootSignature = rootSignature;

	skyboxPsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	skyboxPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	skyboxPsoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	skyboxPsoDesc.DepthStencilState.DepthEnable = TRUE;
	skyboxPsoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	skyboxPsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	skyboxPsoDesc.DepthStencilState.StencilEnable = FALSE;
	skyboxPsoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

	skyboxPsoDesc.SampleMask = UINT_MAX;
	skyboxPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	skyboxPsoDesc.NumRenderTargets = 1;
	skyboxPsoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	skyboxPsoDesc.SampleDesc.Count = 1;

	// Z=Wにより空の深度は常に1.0になるため、LESS_EQUAL を指定して描画を許可
	skyboxPsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	// 他のモデル描画に影響を与えないよう、深度バッファへの書き込みを無効化
	skyboxPsoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

	hr = engine->GetDevice()->CreateGraphicsPipelineState(&skyboxPsoDesc, IID_PPV_ARGS(&_pipelineState));
	if (FAILED(hr)) {
		printf("Skybox PSOの作成に失敗しました。\n");
		return false;
	}

	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

	struct SkyboxVertex {
		float pos[3];
	};

	SkyboxVertex skyboxVertices[] = {
	{ -1.0f,  1.0f, -1.0f }, // 0: 左・上・手前
	{  1.0f,  1.0f, -1.0f }, // 1: 右・上・手前
	{  1.0f,  1.0f,  1.0f }, // 2: 右・上・奥
	{ -1.0f,  1.0f,  1.0f }, // 3: 左・上・奥
	{ -1.0f, -1.0f, -1.0f }, // 4: 左・下・手前
	{  1.0f, -1.0f, -1.0f }, // 5: 右・下・手前
	{  1.0f, -1.0f,  1.0f }, // 6: 右・下・奥
	{ -1.0f, -1.0f,  1.0f }  // 7: 左・下・奥
	};

	// 12枚の三角形（36個の頂点インデックス）
	uint16_t skyboxIndices[] = {
		3, 1, 0, 2, 1, 3, // 上面
		0, 5, 4, 1, 5, 0, // 手前面
		1, 6, 5, 2, 6, 1, // 右面
		2, 7, 6, 3, 7, 2, // 奥面
		3, 4, 7, 0, 4, 3, // 左面
		4, 6, 7, 5, 6, 4  // 下面
	};

	UINT skyboxVBSize = sizeof(skyboxVertices);
	D3D12_RESOURCE_DESC skyboxVbDesc = {};
	skyboxVbDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	skyboxVbDesc.Width = skyboxVBSize;
	skyboxVbDesc.Height = 1;
	skyboxVbDesc.DepthOrArraySize = 1;
	skyboxVbDesc.MipLevels = 1;
	skyboxVbDesc.SampleDesc.Count = 1;
	skyboxVbDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	engine->GetDevice()->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&skyboxVbDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&_vertexBuffer));

	void* pSkyboxVbData;
	_vertexBuffer->Map(0, nullptr, &pSkyboxVbData);
	memcpy(pSkyboxVbData, skyboxVertices, skyboxVBSize);
	_vertexBuffer->Unmap(0, nullptr);

	_vbView.BufferLocation = _vertexBuffer->GetGPUVirtualAddress();
	_vbView.SizeInBytes = skyboxVBSize;
	_vbView.StrideInBytes = sizeof(SkyboxVertex);

	UINT skyboxIbSize = sizeof(skyboxIndices);
	D3D12_RESOURCE_DESC skyboxIbDesc = skyboxVbDesc;
	skyboxIbDesc.Width = skyboxIbSize;

	engine->GetDevice()->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&skyboxIbDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&_indexBuffer));

	void* pSkyboxIbData;
	_indexBuffer->Map(0, nullptr, &pSkyboxIbData);
	memcpy(pSkyboxIbData, skyboxIndices, skyboxIbSize);
	_indexBuffer->Unmap(0, nullptr);

	_ibView.BufferLocation = _indexBuffer->GetGPUVirtualAddress();
	_ibView.SizeInBytes = skyboxIbSize;
	_ibView.Format = DXGI_FORMAT_R16_UINT;

	int skyWidth, skyHeight, skyChannels;
	stbi_uc* skyPixels[6] = { nullptr };
	skyPixels[0] = stbi_load(texturePaths[0], &skyWidth, &skyHeight, &skyChannels, 4);

	// スカイボックス用 キューブマップテクスチャの作成
	D3D12_RESOURCE_DESC skyTexDesc = {};
	skyTexDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	skyTexDesc.Width = skyWidth;
	skyTexDesc.Height = skyHeight;
	//キューブマップとして扱うため、6枚の画像を保持できる「テクスチャ配列（6層）」として作成
	skyTexDesc.DepthOrArraySize = 6;
	skyTexDesc.MipLevels = 1;
	skyTexDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	skyTexDesc.SampleDesc.Count = 1;

	D3D12_HEAP_PROPERTIES skyboxTexHeapProp = {};
	skyboxTexHeapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
	engine->GetDevice()->CreateCommittedResource(&skyboxTexHeapProp, D3D12_HEAP_FLAG_NONE, &skyTexDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&_texture));

	UINT numSubresources = skyTexDesc.MipLevels * skyTexDesc.DepthOrArraySize;
	std::vector<UINT> numRows(numSubresources);
	std::vector<UINT64>rowSize(numSubresources);
	UINT64 totalBytes;
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprints[6];

	// 6枚の画像がGPU上でどのようにアライメント（配置）されるべきかのメモリレイアウト情報を取得
	engine->GetDevice()->GetCopyableFootprints(&skyTexDesc, 0, 6, 0, footprints, numRows.data(), rowSize.data(), &totalBytes);

	D3D12_RESOURCE_DESC upDesc = skyTexDesc;
	upDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	upDesc.Width = totalBytes;
	upDesc.Height = 1;
	upDesc.DepthOrArraySize = 1;
	upDesc.Format = DXGI_FORMAT_UNKNOWN;
	upDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	D3D12_HEAP_PROPERTIES skyboxUploadHeap = { D3D12_HEAP_TYPE_UPLOAD };
	engine->GetDevice()->CreateCommittedResource(&skyboxUploadHeap, D3D12_HEAP_FLAG_NONE, &upDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&_uploadBuffer));

	uint8_t* pSkyboxUploadData;
	_uploadBuffer->Map(0, nullptr, (void**)&pSkyboxUploadData);

	for (int i = 0; i < 6; i++) {
		if (i > 0) skyPixels[i] = stbi_load(texturePaths[i], &skyWidth, &skyHeight, &skyChannels, 4);

		if (skyPixels[i] == nullptr) {
			printf("Skybox画像の読み込みに失敗: %s\n", texturePaths[i]);
			continue;
		}

		for (UINT y = 0; y < numRows[i]; y++) {
			uint8_t* dest = pSkyboxUploadData + footprints[i].Offset + y * footprints[i].Footprint.RowPitch;
			uint8_t* src = skyPixels[i] + y * (skyWidth * 4);
			memcpy(dest, src, skyWidth * 4);
		}
		stbi_image_free(skyPixels[i]);
	}

	_uploadBuffer->Unmap(0, nullptr);

	for (int i = 0; i < 6; i++) {
		D3D12_TEXTURE_COPY_LOCATION dst = {};
		dst.pResource = _texture.Get();
		dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		dst.SubresourceIndex = i;

		D3D12_TEXTURE_COPY_LOCATION src = {};
		src.pResource = _uploadBuffer.Get();
		src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		src.PlacedFootprint = footprints[i];

		engine->GetCommandList()->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
	}

	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = _texture.Get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	engine->GetCommandList()->ResourceBarrier(1, &barrier);

	// Skybox専用のSRVヒープを作成
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.NumDescriptors = 1;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	engine->GetDevice()->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&_srvHeap));

	// キューブマップとしてビュー(SRV)を作成
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = skyTexDesc.Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.TextureCube.MipLevels = 1;

	engine->GetDevice()->CreateShaderResourceView(
		_texture.Get(),
		&srvDesc,
		_srvHeap->GetCPUDescriptorHandleForHeapStart());

	return true;
}

void Skybox::Draw(GraphicsEngine* engine)
{
	// skybox専用のパイプラインに切り替える
	engine->GetCommandList()->SetPipelineState(_pipelineState.Get());

	// skyboxを描画する「箱」の頂点とインデックスをセット
	engine->GetCommandList()->IASetVertexBuffers(0, 1, &_vbView);
	engine->GetCommandList()->IASetIndexBuffer(&_ibView);

	// SRVヒープをセット
	ID3D12DescriptorHeap* descriptorHeaps[] = { _srvHeap.Get() };
	engine->GetCommandList()->SetDescriptorHeaps(1, descriptorHeaps);

	// ヒープの先頭にあるSRVを、ルートシグネチャのパラメータ番号3にセット
	engine->GetCommandList()->SetGraphicsRootDescriptorTable(3, _srvHeap->GetGPUDescriptorHandleForHeapStart());

	// 箱（12枚の三角形 = 36インデックス）を描画
	engine->GetCommandList()->DrawIndexedInstanced(36, 1, 0, 0, 0);
}
