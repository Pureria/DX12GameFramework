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

	// 通常モデル用のInput Layout
	D3D12_INPUT_ELEMENT_DESC stdInputElementDesc[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{ "BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{ "BLENDWEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	};

	// 通常モデル用のパイプラインを生成して登録
	PipelineDesc stdDesc = {};
	stdDesc.vsFilePath = L"shaders.hlsl";
	stdDesc.psFilePath = L"shaders.hlsl";
	stdDesc.depthEnable = true;
	stdDesc.depthWrite = true;
	stdDesc.depthFunc = D3D12_COMPARISON_FUNC_LESS;
	stdDesc.cullMode = D3D12_CULL_MODE_NONE;
	stdDesc.isTransparent = false;
	stdDesc.inputLayout = stdInputElementDesc;
	stdDesc.numElements = _countof(stdInputElementDesc);
	graphicsEngine.GetPipelineManager()->CreatePipeline("Standard", stdDesc);

	// スカイボックス用のInput Layout
	D3D12_INPUT_ELEMENT_DESC skyInputElementDesc[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	};

	// スカイボックス用のパイプラインを生成して登録
	PipelineDesc skyDesc = {};
	skyDesc.vsFilePath = L"skybox.hlsl";
	skyDesc.psFilePath = L"skybox.hlsl";
	skyDesc.depthEnable = true;
	skyDesc.depthWrite = false; // 他のモデルを隠さないようにする
	skyDesc.depthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL; // Z = 1.0 に対応
	skyDesc.cullMode = D3D12_CULL_MODE_NONE; // カリングなし
	skyDesc.isTransparent = false;
	skyDesc.inputLayout = skyInputElementDesc;
	skyDesc.numElements = _countof(skyInputElementDesc);
	graphicsEngine.GetPipelineManager()->CreatePipeline("Skybox", skyDesc);

	const char* skyboxFiles[6] = {
		"Texture/Skybox/px.png", "Texture/Skybox/nx.png", "Texture/Skybox/py.png",
		"Texture/Skybox/ny.png", "Texture/Skybox/pz.png", "Texture/Skybox/nz.png"
	};

	Skybox skybox;
	skybox.Initialize(&graphicsEngine, graphicsEngine.GetPipelineManager()->GetRootSignature(), skyboxFiles);

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
	camera.Initialize(&graphicsEngine);
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
			graphicsEngine.GetCommandList()->RSSetViewports(1, &viewport);
			graphicsEngine.GetCommandList()->RSSetScissorRects(1, &scissorRect);

			graphicsEngine.GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			// カメラの定数バッファ更新
			camera.Bind(&graphicsEngine);

			// 描画実行
			skybox.Draw(&graphicsEngine);
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