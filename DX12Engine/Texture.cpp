#include "Texture.h"
#include "System/ImageLoader.h"


Texture::Texture(std::wstring path, ID3D12Device1 * device, ID3D12GraphicsCommandList * commandList) : path(path), device(device), commandList(commandList)
{
	LoadTexture(path);
}


Texture::~Texture()
{
	SAFE_RELEASE(textureBuffer);
	SAFE_RELEASE(textureBufferUploadHeap);
}

HRESULT Texture::LoadTexture(std::wstring path)
{
	HRESULT hr;

	int imageBytesPerRaw;
	BYTE* imageData;
	int imageSize = LoadImageDataFromFile(&imageData, textureDesc, path.c_str(), imageBytesPerRaw);
	// Check if image exists
	if (imageSize <= 0)
	{
		return false;
	}

	// create default heap for texture
	hr = device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE, &textureDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&textureBuffer)
	);
	if (FAILED(hr))
	{
		return false;
	}
	textureBuffer->SetName(L"Texture buffer resource heap");

	// get texture heap size which is 256 byte aligned
	UINT64 textureUploadBufferSize;
	device->GetCopyableFootprints(&textureDesc, 0, 1, 0, nullptr, nullptr, nullptr, &textureUploadBufferSize);

	// create upload heap for texture
	hr = device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(textureUploadBufferSize), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(&textureBufferUploadHeap));
	if (FAILED(hr))
	{
		return false;
	}
	textureBufferUploadHeap->SetName(L"Texture Buffer Upload Resource Heap");

	// store data in upload heap
	D3D12_SUBRESOURCE_DATA textureData = {};
	textureData.pData = &imageData[0];
	textureData.RowPitch = imageBytesPerRaw;
	textureData.SlicePitch = imageBytesPerRaw * textureDesc.Height;

	// copy contents to default heap
	UpdateSubresources(commandList, textureBuffer, textureBufferUploadHeap, 0, 0, 1, &textureData);

	// transition the texture from default heap to pixel shader
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(textureBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
}

ID3D12Resource * Texture::GetTexture()
{
	return textureBuffer;
}

ID3D12Resource * Texture::GetUploadHeap()
{
	return textureBufferUploadHeap;
}

DXGI_FORMAT Texture::GetFormat()
{
	return textureDesc.Format;
}
