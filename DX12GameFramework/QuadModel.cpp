#include "pch.h"
#include "QuadModel.h"
#include "GraphicsEngine.h"

QuadModel::QuadModel()
    : _materialColors()
    , _meshResource()
    , _dummyTextureBuffer(nullptr)
    , _uploadTextureBuffer(nullptr)
    , _srvHeap(nullptr)
{
}

QuadModel::~QuadModel()
{
}

void QuadModel::Initialize(GraphicsEngine* engine)
{
    std::vector<Vertex> vertices = {
              // position           normal           color             uv         boneIDs    boneWeights
        { { -1.0f, -1.0f, 0.0f }, {0,0,-1}, {1.0f,1.0f,1.0f,1.0f}, {0.0f, 1.0f}, {0,0,0,0}, { 1.0f,0,0,0} }, // 左下
        { { -1.0f,  1.0f, 0.0f }, {0,0,-1}, {1.0f,1.0f,1.0f,1.0f}, {0.0f, 0.0f}, {0,0,0,0}, { 1.0f,0,0,0} }, // 左上
        { {  1.0f, -1.0f, 0.0f }, {0,0,-1}, {1.0f,1.0f,1.0f,1.0f}, {1.0f, 1.0f}, {0,0,0,0}, { 1.0f,0,0,0} }, // 右下
        { {  1.0f,  1.0f, 0.0f }, {0,0,-1}, {1.0f,1.0f,1.0f,1.0f}, {1.0f, 0.0f}, {0,0,0,0}, { 1.0f,0,0,0} }  // 右上
    };

    std::vector<uint16_t> indices = {
        0, 1, 2,
        2, 1, 3,
    };

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
        IID_PPV_ARGS(&_meshResource.vertexBuffer));

    void* mappedData = nullptr;
    _meshResource.vertexBuffer->Map(0, nullptr, &mappedData);
    memcpy(mappedData, vertices.data(), vertexBufferSize);
    _meshResource.vertexBuffer->Unmap(0, nullptr);

    _meshResource.vbView.BufferLocation = _meshResource.vertexBuffer->GetGPUVirtualAddress();
    _meshResource.vbView.SizeInBytes = vertexBufferSize;
    _meshResource.vbView.StrideInBytes = sizeof(Vertex);

    UINT indexBufferSize = static_cast<UINT>(indices.size() * sizeof(uint16_t));
    _meshResource.indexCount = static_cast<UINT>(indices.size());

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
        IID_PPV_ARGS(&_meshResource.indexBuffer));

    void* mappedIndexData = nullptr;
    _meshResource.indexBuffer->Map(0, nullptr, &mappedIndexData);
    memcpy(mappedIndexData, indices.data(), indexBufferSize);
    _meshResource.indexBuffer->Unmap(0, nullptr);

    _meshResource.ibView.BufferLocation = _meshResource.indexBuffer->GetGPUVirtualAddress();
    _meshResource.ibView.SizeInBytes = indexBufferSize;
    _meshResource.ibView.Format = DXGI_FORMAT_R16_UINT;


    _meshResource.materialIndex = 0;
    _materialColors.resize(1);
    _materialColors[0] = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

    D3D12_HEAP_PROPERTIES defaultHeap = { D3D12_HEAP_TYPE_DEFAULT };
    D3D12_HEAP_PROPERTIES uploadHeap = { D3D12_HEAP_TYPE_UPLOAD };

    int texWidth = 50;
    int texHeight = 50;
    std::vector<uint32_t> dummyTexture;
    dummyTexture.resize(texWidth * texHeight);

    int checkSize = 10;
    for (int y = 0; y < texHeight; y++) {
        for (int x = 0; x < texWidth; x++) {
            uint32_t color;
            if (((x / checkSize) + (y / checkSize)) % 2 == 0) {
                color = 0xFFFFFFFF;
            }
            else {
                color = 0xFF98FF98;
            }

            dummyTexture[(y * texWidth) + x] = color;
        }
    }

    // テクスチャ作成
    D3D12_RESOURCE_DESC dummyTexDesc = {};
    dummyTexDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    dummyTexDesc.Width = texWidth;
    dummyTexDesc.Height = texHeight;
    dummyTexDesc.DepthOrArraySize = 1;
    dummyTexDesc.MipLevels = 1;
    dummyTexDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    dummyTexDesc.SampleDesc.Count = 1;

    engine->GetDevice()->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &dummyTexDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&_dummyTextureBuffer));

    UINT dummyRowPitch = (texWidth * 4 + 255) & ~255;
    UINT64 uploadBufferSize = static_cast<UINT64>(dummyRowPitch * texHeight);

    D3D12_RESOURCE_DESC upDesc = dummyTexDesc;
    upDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    upDesc.Width = uploadBufferSize;
    upDesc.Height = 1;
    upDesc.DepthOrArraySize = 1;
    upDesc.Format = DXGI_FORMAT_UNKNOWN;
    upDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    engine->GetDevice()->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &upDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&_uploadTextureBuffer));
    
    void* mapped = nullptr;
    _uploadTextureBuffer->Map(0, nullptr, &mapped);
    for (int y = 0; y < texHeight; y++) {
        memcpy(reinterpret_cast<uint8_t*>(mapped) + (y * dummyRowPitch), &dummyTexture[y * texWidth], texWidth * sizeof(uint32_t));
    }
    _uploadTextureBuffer->Unmap(0, nullptr);

    D3D12_TEXTURE_COPY_LOCATION dst = { _dummyTextureBuffer.Get(), D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX, 0 };
    D3D12_TEXTURE_COPY_LOCATION src = { _uploadTextureBuffer.Get(), D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT, 0 };
    src.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    src.PlacedFootprint.Footprint.Width = texWidth;
    src.PlacedFootprint.Footprint.Height = texHeight;
    src.PlacedFootprint.Footprint.Depth = 1;
    src.PlacedFootprint.Footprint.RowPitch = dummyRowPitch;

    engine->GetCommandList()->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = _dummyTextureBuffer.Get();
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

    engine->GetCommandList()->ResourceBarrier(1, &barrier);

    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.NumDescriptors = 1;
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    engine->GetDevice()->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&_srvHeap));

    D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = _srvHeap->GetCPUDescriptorHandleForHeapStart();
    UINT srvDescriptorSize = engine->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;

    engine->GetDevice()->CreateShaderResourceView(_dummyTextureBuffer.Get(), &srvDesc, srvHandle);
}

void QuadModel::Draw(GraphicsEngine* engine)
{
    auto cmdList = engine->GetCommandList();

    if (_srvHeap) {
        ID3D12DescriptorHeap* descriptorHeaps[] = { _srvHeap.Get() };
        cmdList->SetDescriptorHeaps(1, descriptorHeaps);
    }

    UINT srvDescriptorSize = engine->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    cmdList->IASetVertexBuffers(0, 1, &_meshResource.vbView);
    cmdList->IASetIndexBuffer(&_meshResource.ibView);
    cmdList->SetGraphicsRootDescriptorTable(3, _srvHeap->GetGPUDescriptorHandleForHeapStart());

    cmdList->SetGraphicsRoot32BitConstants(2, 1, &_meshResource.materialIndex, 0);

    cmdList->DrawIndexedInstanced(_meshResource.indexCount, 1, 0, 0, 0);
}
