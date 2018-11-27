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
	CDescriptorHeapWrapper uavHeap;
	Microsoft::WRL::ComPtr<ID3D12Resource> textureUAV;
	UINT viewWidth;
	UINT viewHeight;
	std::wstring shaderName;
public:
	ComputeDispatch() {};
	ComputeDispatch(ID3D12Device1* device, UINT width, UINT height, std::wstring shader);
	~ComputeDispatch();
	void Init();
	void CreateRootSignature();
	void CreatePipelineStateObject();

	void SetSRV(ID3D12Resource* textureSRV, int index);
	void SetUAV(int index);
	void SetConstant(int index, float value);
	void Dispatch(ID3D12GraphicsCommandList* commandList, int constValue);
	CDescriptorHeapWrapper& GetResultDescriptor();
};

