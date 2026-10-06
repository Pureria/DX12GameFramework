#include "pch.h"
#define STB_IMAGE_IMPLEMENTATION
#include "define.h"
#include "GraphicsEngine.h"
#include "ImGuiManager.h"

#include "Skybox.h"
#include "Time.h"
#include "Camera.h"
#include "AssimpModel.h"
#include "QuadModel.h"
#include "Actor.h"
#include "AnimatorComponent.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// ウィンドウプロシージャ: OSから送られるメッセージ（入力、リサイズ等）を処理するコールバック関数
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam)) {
		return true;
	}

	switch (msg) {
	case WM_DESTROY: // ×ボタンが押された
		PostQuitMessage(0); //メッセージループを終了させる
		return 0;
	}

	// 処理しないメッセージはデフォルト処理に任せる
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

int main()
{
	SetConsoleOutputCP(932);
	printf("Hello World\n");

#pragma region CreateWindow
	// ウィンドウクラスの登録
	// ウィンドウの「種類」を定義する。アイコン、カーソル、メッセージ処理関数などを指定する
	WNDCLASSEX wc = {};
	wc.cbSize = sizeof(WNDCLASSEX);         // 構造体のサイズ（必須）
	wc.lpfnWndProc = WndProc;                    // メッセージ処理関数
	wc.hInstance = GetModuleHandle(nullptr);    // アプリケーションのインスタンスハンドル
	wc.lpszClassName = L"DX12WindowClass";          // クラス名（CreateWindowExで参照する）

	RegisterClassEx(&wc);

	RECT wrc = { 0, 0, WindowWidth, WindowHeight };
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	// ウィンドウの作成
	// ※ 1280x720 はタイトルバー・枠を含むサイズ。描画領域(クライアント領域)はやや小さくなる
	HWND hwnd = CreateWindowEx(
		0,                          // 拡張ウィンドウスタイル
		L"DX12WindowClass",         // 登録したウィンドウクラス名
		L"DX12 SandBox",            // タイトルバーに表示する文字列
		WS_OVERLAPPEDWINDOW,        // 標準的なウィンドウスタイル（枠、タイトルバー、最大化等）
		CW_USEDEFAULT,              // X座標（OSに任せる）
		CW_USEDEFAULT,              // Y座標（OSに任せる）
		wrc.right - wrc.left,
		wrc.bottom - wrc.top,                  // ウィンドウの幅、高さ
		nullptr,                    // 親ウィンドウ（なし）
		nullptr,                    // メニュー（なし）
		GetModuleHandle(nullptr),   // アプリケーションのインスタンスハンドル
		nullptr                     // 追加パラメータ（なし）
	);

	ShowWindow(hwnd, SW_SHOW);
#pragma endregion CreateWindow

#pragma region DX12Initialize
	GraphicsEngine graphicsEngine;
	if (!graphicsEngine.Initialize(hwnd)) {
		printf("Failed initialized in Graphics Engine");
		return -1;
	}

	HRESULT hr;


	// シェーダーとパイプラインの構築
	// シェーダーのコンパイル
	ComPtr<ID3DBlob> vsBlob;    // 頂点シェーダーのコンパイル結果
	ComPtr<ID3DBlob> psBlob;    // ピクセルシェーダーのコンパイル結果
	ComPtr<ID3DBlob> errorBlob; // エラーメッセージ用

	// 頂点シェーダー (VS) のコンパイル
	hr = D3DCompileFromFile(
		L"shaders.hlsl",          // 読み込むHLSLファイル
		nullptr,                  // マクロ設定（なし）
		nullptr,                  // インクルード設定（なし）
		"VSMain",                 // 実行開始する関数名
		"vs_5_0",                 // 頂点シェーダーのバージョン5.0
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, // デバッグ用のフラグ
		0,
		&vsBlob,
		&errorBlob
	);

	if (FAILED(hr)) {
		// コンパイルエラーがある場合は、エラー内容をコンソールに表示して強制終了
		if (errorBlob) {
			printf("VS Compile Error: %s\n", (char*)errorBlob->GetBufferPointer());
		}
		return -1;
	}

	// ピクセルシェーダー (PS) のコンパイル
	hr = D3DCompileFromFile(
		L"shaders.hlsl",
		nullptr,
		nullptr,
		"PSMain",                 // 実行開始する関数名
		"ps_5_0",                 // ピクセルシェーダーのバージョン5.0
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		&psBlob,
		&errorBlob
	);

	if (FAILED(hr)) {
		if (errorBlob) {
			printf("PS Compile Error: %s\n", (char*)errorBlob->GetBufferPointer());
		}
		return -1;
	}

	// Root Signature の作成
	// シェーダーの register(b0) に定数バッファを紐づけるためのパラメータ設定
	D3D12_ROOT_PARAMETER rootParams[4] = {};
	rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParams[0].Descriptor.ShaderRegister = 0;
	rootParams[0].Descriptor.RegisterSpace = 0;
	// ピクセルシェーダー(PS)からも定数バッファ内のマテリアル色を参照できるよう、アクセス権限をALLに設定
	rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParams[1].Descriptor.ShaderRegister = 1;
	rootParams[1].Descriptor.RegisterSpace = 0;
	rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// パラメータ2: Root Constants (マテリアルインデックス)
	// 定数バッファを介さずに、32bitの整数値を直接高速にシェーダー(register b1)へ渡すための設定
	rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	rootParams[2].Constants.ShaderRegister = 2;
	rootParams[2].Constants.RegisterSpace = 0;
	rootParams[2].Constants.Num32BitValues = 1;
	rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_DESCRIPTOR_RANGE descRange = {};
	descRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descRange.NumDescriptors = 1;
	descRange.BaseShaderRegister = 0;
	descRange.RegisterSpace = 0;
	descRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	rootParams[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParams[3].DescriptorTable.NumDescriptorRanges = 1;
	rootParams[3].DescriptorTable.pDescriptorRanges = &descRange;
	rootParams[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	samplerDesc.ShaderRegister = 0;
	samplerDesc.RegisterSpace = 0;
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
	// 「頂点バッファからデータ（Input Layout）を受け取ります」という宣言フラグだけ立てる
	rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	// 設定したパラメータをRoot Signatureに登録
	rootSigDesc.NumParameters = 4;
	rootSigDesc.pParameters = rootParams;
	rootSigDesc.NumStaticSamplers = 1;
	rootSigDesc.pStaticSamplers = &samplerDesc;

	ComPtr<ID3DBlob> rootSigBlob;
	// 設計図（rootSigDesc）をGPUが読めるバイナリデータ（Blob）に変換（シリアライズ）する
	D3D12SerializeRootSignature(
		&rootSigDesc,
		D3D_ROOT_SIGNATURE_VERSION_1,
		&rootSigBlob,
		&errorBlob
	);

	ComPtr<ID3D12RootSignature> rootSignature;
	// バイナリデータから実際のRoot Signatureオブジェクトを生成
	graphicsEngine.GetDevice()->CreateRootSignature(
		0,
		rootSigBlob->GetBufferPointer(),
		rootSigBlob->GetBufferSize(),
		IID_PPV_ARGS(&rootSignature)
	);

	// Pipeline State Object (PSO) の作成
	// Input Layout（頂点データの構造）の定義
	// C++側のデータがどういう構造になっているかをGPUに教える
	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{
			"POSITION",                              // セマンティクス名（HLSLと一致させる）
			0,                                       // インデックス
			DXGI_FORMAT_R32G32B32_FLOAT,             // float3 (x, y, z)
			0,                                       // 入力スロット
			0,                                       // 先頭からのオフセット（0バイト目）
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		},
		{
			"NORMAL",
			0,
			DXGI_FORMAT_R32G32B32_FLOAT,
			0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		},
		{
			"COLOR",                                 // セマンティクス名
			0,
			DXGI_FORMAT_R32G32B32A32_FLOAT,          // float4 (r, g, b, a)
			0,
			D3D12_APPEND_ALIGNED_ELEMENT,            // オフセット（POSITION(12バイト)の後ろ）
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		},
		{
			"TEXCOORD",
			0,
			DXGI_FORMAT_R32G32_FLOAT,
			0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		},
		{
			"BLENDINDICES",
			0,
			DXGI_FORMAT_R32G32B32A32_UINT,
			0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		},
		{
			"BLENDWEIGHT",
			0,
			DXGI_FORMAT_R32G32B32A32_FLOAT,
			0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		},
	};

	// PSOの設計図を作成
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.VS.pShaderBytecode = vsBlob->GetBufferPointer();
	psoDesc.VS.BytecodeLength = vsBlob->GetBufferSize();
	psoDesc.PS.pShaderBytecode = psBlob->GetBufferPointer();
	psoDesc.PS.BytecodeLength = psBlob->GetBufferSize();
	psoDesc.InputLayout.pInputElementDescs = inputLayout;
	psoDesc.InputLayout.NumElements = _countof(inputLayout);
	psoDesc.pRootSignature = rootSignature.Get();

	// ラスタライザ設定（三角形をどう塗りつぶすか）
	psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID; // 中身を塗りつぶす
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;  // カリングなし（裏面も描画）

	// ブレンド設定（描いた色をどう合成するか。今回は単純に上書き）
	psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	// 深度ステンシルステートの設定
	// ピクセルを描画する際、奥にあるものを隠す（深度テスト）設定
	psoDesc.DepthStencilState.DepthEnable = TRUE;                            // 深度テストを有効化
	psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;   // 深度値をバッファに書き込む
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;        // 今あるピクセルより「手前」なら描画する
	psoDesc.DepthStencilState.StencilEnable = FALSE;                         // ステンシル（型抜き）テストは無効

	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT; // 深度バッファのフォーマット

	// その他の設定
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; // 三角形を描画する
	psoDesc.NumRenderTargets = 1;                                      // 描画先は1つ
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;             // SwapChainのフォーマットと一致させる
	psoDesc.SampleDesc.Count = 1;                                      // MSAAなし

	// 実際のPSOオブジェクトを生成
	ComPtr<ID3D12PipelineState> pipelineState;
	graphicsEngine.GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState));

	const char* skyboxFiles[6] = {
		"Texture/Skybox/px.png", "Texture/Skybox/nx.png", "Texture/Skybox/py.png",
		"Texture/Skybox/ny.png", "Texture/Skybox/pz.png", "Texture/Skybox/nz.png"
	};

	Skybox skybox;
	skybox.Initialize(&graphicsEngine, rootSignature.Get(), skyboxFiles);

	AssimpModel model;
	if (!model.Initialize("Model/Bot.fbx", &graphicsEngine)) {
		return -1;
	}

	QuadModel quadModel;
	quadModel.Initialize(&graphicsEngine);

	Actor actor1(&model);
	actor1.Initialize(&graphicsEngine);
	actor1.AddComponent<AnimatorComponent>();
	actor1.SetPosition(-1.5f, 0.0f, 0.0f);
	actor1.SetScale(0.005f, 0.005f, 0.005f);

	Actor actor2(&model);
	actor2.Initialize(&graphicsEngine);
	actor2.AddComponent<AnimatorComponent>();
	actor2.SetPosition(0.0f, 0.0f, 0.0f);
	actor2.SetScale(0.005f, 0.005f, 0.005f);

	Actor actor3(&model);
	actor3.Initialize(&graphicsEngine);
	actor3.AddComponent<AnimatorComponent>();
	actor3.SetPosition(1.5f, 0.0f, 0.0f);
	actor3.SetScale(0.005f, 0.005f, 0.005f);

	Actor quadActor(&quadModel);
	quadActor.Initialize(&graphicsEngine);
	quadActor.SetPosition(0.0f, 0.0f, 0.0f);
	quadActor.SetRotation(XMConvertToRadians(90.0f), 0.0f, 0.0f);
	quadActor.SetScale(5.0f, 5.0f, 1.0f);

	graphicsEngine.WaitForGPU();

	// --- 定数バッファの作成 ---
	// 定数バッファのサイズは256バイトの倍数でなければならない
	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
	UINT cbSize = (sizeof(SceneConstantBuffer) + 255) & ~255;
	ComPtr<ID3D12Resource> constantBuffer;

	D3D12_RESOURCE_DESC cbDesc = {};
	cbDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	cbDesc.Width = cbSize;
	cbDesc.Height = 1;
	cbDesc.DepthOrArraySize = 1;
	cbDesc.MipLevels = 1;
	cbDesc.SampleDesc.Count = 1;
	cbDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	graphicsEngine.GetDevice()->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&cbDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&constantBuffer)
	);

	// 毎フレーム書き込むため、ずっとMap（CPUからアクセス可能）にしておく（Persistent Mapping）
	void* cbMappedData = nullptr;
	constantBuffer->Map(0, nullptr, &cbMappedData);

	// Viewport と Scissor Rect の設定
	// 描画する領域（画面全体）を指定する
	D3D12_VIEWPORT viewport = {};
	viewport.Width = 1280.0f;
	viewport.Height = 720.0f;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	D3D12_RECT scissorRect = {};
	scissorRect.right = 1280;
	scissorRect.bottom = 720;

#pragma endregion DX12Initialize

	ULONGLONG startTime = GetTickCount64();

	float fovAngleY = 45.0f * (3.14159265f / 180.0f); // 縦の視野角（45度）
	float aspectRatio = (float)WindowWidth / (float)WindowHeight;

	Camera camera;
	camera.SetPosition(0.0f, 0.5f, -2.0f);
	camera.SetPerspective(fovAngleY, aspectRatio, 0.1f, 100.0f);

	Time::Initialize();

	ImGuiManager imguiManager;
	imguiManager.Initialize(hwnd, &graphicsEngine);

	// メッセージループ
	MSG msg = {};
	while (true)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT)
				break;

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {
			Time::Update();

			// フレームの準備
			graphicsEngine.BeginFrame();
			imguiManager.BeginFrame();

			ImGui::ShowDemoWindow();

			// データの更新（定数バッファ）
			static float angle = 0.0f;
			angle += 0.01f;

			camera.Update();

			SceneConstantBuffer sceneCBData = {};
			sceneCBData.view = XMMatrixTranspose(camera.GetViewMatrix());		// View行列: カメラの位置と向き
			sceneCBData.proj = XMMatrixTranspose(camera.GetProjectionMatrix()); // Projection行列: 遠近法（パース）の設定
			memcpy(cbMappedData, &sceneCBData, sizeof(SceneConstantBuffer));

			actor1.SetRotation(0.0f, angle, 0.0f);
			//actor2.SetRotation(0.0f, angle * 2.0f, 0.0f);
			actor3.SetRotation(0.0f, -angle, 0.0f);

			actor1.Update();
			actor2.Update();
			actor3.Update();
			quadActor.Update();

			ULONGLONG currentTime = GetTickCount64();
			float timeInSeconds = (currentTime - startTime) / 1000.0f;

			// 描画コマンドの記録
			graphicsEngine.GetCommandList()->SetGraphicsRootSignature(rootSignature.Get());
			graphicsEngine.GetCommandList()->SetPipelineState(pipelineState.Get());
			graphicsEngine.GetCommandList()->RSSetViewports(1, &viewport);
			graphicsEngine.GetCommandList()->RSSetScissorRects(1, &scissorRect);

			graphicsEngine.GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			// Root Signatureの0番目のパラメータ（CBV）に定数バッファをセット
			graphicsEngine.GetCommandList()->SetGraphicsRootConstantBufferView(0, constantBuffer->GetGPUVirtualAddress());

			// 描画実行
			skybox.Draw(&graphicsEngine);

			// 次のモデルを描画するために、標準のパイプラインに戻す
			graphicsEngine.GetCommandList()->SetPipelineState(pipelineState.Get());

			actor1.Draw(&graphicsEngine);
			actor2.Draw(&graphicsEngine);
			actor3.Draw(&graphicsEngine);
			quadActor.Draw(&graphicsEngine);

			imguiManager.Draw(&graphicsEngine);

			graphicsEngine.EndFrame();
		}
	}

	imguiManager.Finalize();

	return 0;
}