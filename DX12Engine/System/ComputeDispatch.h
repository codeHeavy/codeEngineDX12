#pragma once
#include "../stdafx.h"
#include "ShaderManager.h"
#include "DirectXHelpers.h"
#include "DirectxHelper.h"
#include "../Texture.h"
class ComputeDispatch
{
private:
	ID3D12Device1* device;
	ID3D12RootSignature* rootSignature;
	ID3D12PipelineState* computePSO;

	CDescriptorHeapWrapper srvHeap;
	//CDescriptorHeapWrapper uavHeap;
public:
	ComputeDispatch() {};
	ComputeDispatch(ID3D12Device1* device);
	~ComputeDispatch();
	void CreateRootSignature();
	void CreatePipelineStateObject();

	void SetShader(ID3D12GraphicsCommandList* commandList);
	void SetTextureUAV(ID3D12GraphicsCommandList* commandList, Texture* textureUAV) {};
	void SetSRV(ID3D12Resource* textureSRV, int index);
	//void SetUAV(ID3D12Resource* textureSRV, int index);
	void Dispatch(ID3D12GraphicsCommandList* commandList, int x = 1, int y = 1, int z = 1);
};

