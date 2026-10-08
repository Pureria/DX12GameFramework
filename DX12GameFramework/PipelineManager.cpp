#include "pch.h"
#include "PipelineManager.h"
#include "GraphicsEngine.h"

ComPtr<ID3DBlob> PipelineManager::CompileShader(const std::wstring& filePath, const char* entryPoint, const char* target)
{
    ComPtr<ID3DBlob> shaderBlob;
    ComPtr<ID3DBlob> errorBlob;

    HRESULT hr = D3DCompileFromFile(
        filePath.c_str(),
        nullptr,
        nullptr,
        entryPoint,
        target,
        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
        0,
        &shaderBlob,
        &errorBlob);

    if (FAILED(hr)) {
        if (errorBlob) {
            printf("Shader Compile Error (%ls): %s\n", filePath.c_str(), (char*)errorBlob->GetBufferPointer());
        }
        return nullptr;
    }

    return shaderBlob;
}

bool PipelineManager::Initialize(GraphicsEngine* engine)
{
    // プロジェクト全体で使いまわす共通のRoot Signatureを作成
    _graphicsEngine = engine;

    /*
    * b0(カメラ)
    * b1（マテリアル・オブジェクト）
    * b2（ルート定数）
    * t0（テクスチャ）
    */
    D3D12_ROOT_PARAMETER rootParams[4];

    // b0; 定数バッファ（カメラ行列など）
    rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParams[0].Descriptor.ShaderRegister = 0;
    rootParams[0].Descriptor.RegisterSpace = 0;
    rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    // b1: 定数バッファ（オブジェクトのローカル行列など）
    rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParams[1].Descriptor.ShaderRegister = 1;
    rootParams[1].Descriptor.RegisterSpace = 0;
    rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    // b2: ルート定数（マテリアルインデックスなどの即値渡し）
    rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    rootParams[2].Constants.ShaderRegister = 2;
    rootParams[2].Constants.RegisterSpace = 0;
    rootParams[2].Constants.Num32BitValues = 1;
    rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    //t0: テクスチャ
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

    //s0: テクスチャサンプラー（貼り付け方の設定）
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


    // 設計図（Root Signature）の構築
    D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
    rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    rootSigDesc.NumParameters = 4;
    rootSigDesc.pParameters = rootParams;
    rootSigDesc.NumStaticSamplers = 1;
    rootSigDesc.pStaticSamplers = &samplerDesc;

    ComPtr<ID3DBlob> rootSigBlob;
    ComPtr<ID3DBlob> errorBlob;

    HRESULT hr = D3D12SerializeRootSignature(
        &rootSigDesc,
        D3D_ROOT_SIGNATURE_VERSION_1,
        &rootSigBlob,
        &errorBlob);

    if (FAILED(hr)) { return false; }

    ComPtr<ID3D12RootSignature> rootSig;
    hr = _graphicsEngine->GetDevice()->CreateRootSignature(
        0,
        rootSigBlob->GetBufferPointer(),
        rootSigBlob->GetBufferSize(),
        IID_PPV_ARGS(&rootSig));

    if (FAILED(hr)) { return false; }

    _rootSignatures["Default"] = rootSig;

    return true;
}

bool PipelineManager::CreatePipeline(const std::string& pipelineName, const PipelineDesc& desc)
{
    // シェーダーコンパイル
    ComPtr<ID3DBlob> vsBlob = CompileShader(desc.vsFilePath, "VSMain", "vs_5_0");
    ComPtr<ID3DBlob> psBlob = CompileShader(desc.psFilePath, "PSMain", "ps_5_0");

    if (!vsBlob || !psBlob) {
        return false;
    }

    auto itRoot = _rootSignatures.find(desc.rootSignatureName);
    if (itRoot == _rootSignatures.end()) {
        printf("Error: Root Signature '%s' not found!\n", desc.rootSignatureName.c_str());
        return false;
    }
    ID3D12RootSignature* pTargetRootSig = itRoot->second.Get();

    // PSOの設計図を作成
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = pTargetRootSig;
    psoDesc.VS.pShaderBytecode = vsBlob->GetBufferPointer();
    psoDesc.VS.BytecodeLength = vsBlob->GetBufferSize();
    psoDesc.PS.pShaderBytecode = psBlob->GetBufferPointer();
    psoDesc.PS.BytecodeLength = psBlob->GetBufferSize();

    psoDesc.InputLayout.pInputElementDescs = desc.inputLayout;
    psoDesc.InputLayout.NumElements = desc.numElements;

    // ラスタライザ設定（三角形をどう塗りつぶすか）
    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    psoDesc.RasterizerState.CullMode = desc.cullMode;
    psoDesc.RasterizerState.DepthClipEnable = TRUE;

    psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    // ブレンド設定
    if (desc.isTransparent) {
        psoDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
        psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        psoDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
        psoDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
        psoDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    }

    // 深度ステンシル設定
    psoDesc.DepthStencilState.DepthEnable = desc.depthEnable;
    psoDesc.DepthStencilState.DepthWriteMask = desc.depthWrite ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
    psoDesc.DepthStencilState.DepthFunc = desc.depthFunc;
    psoDesc.DepthStencilState.StencilEnable = FALSE;

    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc.Count = 1;

    // PSOの生成
    ComPtr<ID3D12PipelineState> pipelineState;
    HRESULT hr = _graphicsEngine->GetDevice()->CreateGraphicsPipelineState(
        &psoDesc,
        IID_PPV_ARGS(&pipelineState));

    if (FAILED(hr)) {
        return false;
    }

    // 辞書に登録
    PipelineData data;
    data.pso = pipelineState;
    data.pRootSignature = pTargetRootSig;
    _pipelines[pipelineName] = data;

    return true;
}

void PipelineManager::SetPipeline(ID3D12GraphicsCommandList* commandList, const std::string& pipelineName)
{
    if (_currentPipelineName == pipelineName) {
        return; // すでに同じものがセットされているので何もしない
    }

    auto it = _pipelines.find(pipelineName);
    if (it != _pipelines.end()) {
        PipelineData& data = it->second;

        if (_currentRootSignature != data.pRootSignature) {
            commandList->SetGraphicsRootSignature(data.pRootSignature);
            _currentRootSignature = data.pRootSignature;

        }

        commandList->SetPipelineState(data.pso.Get());
        _currentPipelineName = pipelineName;
    }
    else {
        printf("Error: Pipeline '%s' not found!\n", pipelineName.c_str());
    }
}

void PipelineManager::SetRootSignatureOnly(ID3D12GraphicsCommandList* commandList, const std::string& rootSignatureName)
{
    auto it = _rootSignatures.find(rootSignatureName);
    if (it != _rootSignatures.end()) {
        if (_currentRootSignature != it->second.Get()) {
            commandList->SetGraphicsRootSignature(it->second.Get());
            _currentRootSignature = it->second.Get();
        }
    }
    else {
        printf("Error: Root Signature '%s' not found!\n", rootSignatureName.c_str());
    }
}

void PipelineManager::ResetCurrentPipeline()
{
    _currentRootSignature = nullptr;
    _currentPipelineName = "";
}
