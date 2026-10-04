#pragma once
#define NOMINMAX

// C++標準・Windows
#include <Windows.h>
#include <cstdio>
#include <vector>
#include <string>
#include <map>
#include <cmath>
#include <chrono>

// DirectX 12
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

// Assimp
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// リンクするライブラリ
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
