#pragma once
#include <d3d12.h>
#include <string>
#include "WICTextureLoader.h"
class Texture
{
private:
	ID3D12Device1 * device;
	ID3D12GraphicsCommandList * commandList;						// Store commands
	ID3D12Resource* textureBuffer;
	ID3D12Resource* textureBufferUploadHeap;
	D3D12_RESOURCE_DESC textureDesc;	// Load texture from file
	std::wstring path;
public:
	Texture(std::wstring path, ID3D12Device1 * device, ID3D12GraphicsCommandList * commandList);
	Texture(std::wstring path, ID3D12Device1 * device, ID3D12GraphicsCommandList * commandList, DirectX::ResourceUploadBatch &resourceBatch);
	~Texture();

	HRESULT LoadTexture(std::wstring path);
	ID3D12Resource* GetTexture();
	ID3D12Resource* GetUploadHeap();
	DXGI_FORMAT GetFormat();
};

