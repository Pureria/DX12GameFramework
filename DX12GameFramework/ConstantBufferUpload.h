#pragma once
#include <d3d12.h>
#include <wrl.h>
#include "define.h"

//お試しで作ってみた

template <typename T>
class ConstantBufferUpload
{
private:
	ComPtr<ID3D12Resource> _buffer;
	T* _mappedData = nullptr;

	static constexpr UINT GetAlignedSize() {
		return (sizeof(T) + 255) & ~255;
	}

public:

	ConstantBuffer() = default;
	~ConstantBuffer() {
		if (_buffer) {
			_buffer->Unmap(0, nullptr);
		}
	}

	void Initialize(ID3D12Device* device) {
		UINT bufferSize = GetAlignedSize();

		D3D12_HEAP_PROPERTIES heapProp = { D3D12_HEAP_TYPE_UPLOAD };

		D3D12_RESOURCE_DESC resourceDesc = {};
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resourceDesc.Width = bufferSize;
		resourceDesc.Height = 1;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.MipLevels = 1;
		resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

		device->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);
		_buffer->Map(0, nullptr, reinterpret_cast<void**>(&_mappedData));
	}

	void Update(const T& data) {
		if (_mappedData) {
			*_mappedData = data;
		}
	}

	ID3D12Resource* GetResource() const { return _buffer.Get(); }
};