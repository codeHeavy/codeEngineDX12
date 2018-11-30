#pragma once
#include "../stdafx.h"
#include "ShaderManager.h"
#include "DirectXHelpers.h"
#include "DirectxHelper.h"
#include "../Texture.h"
#include "ConstantBuffer.h"
#include "Camera.h"

class MotionBlur
{
private:
	ID3D12Device1 * device;
	ID3D12RootSignature* rootSignature;
	ID3D12PipelineState* computePSO;
	Camera* camera;

	ID3D12Resource* constBuffer;
	MotionBlurConst constants;
	UINT8* constantBufferGPUAddress;				// pointer to memory location

	CDescriptorHeapWrapper srvHeap;
	CDescriptorHeapWrapper uavHeap;
	CDescriptorHeapWrapper cbHeap;
	Microsoft::WRL::ComPtr<ID3D12Resource> textureUAV;
	UINT viewWidth;
	UINT viewHeight;
	std::wstring shaderName;
	static const int ConstantBufferPerObjectAlignedSize = (sizeof(MotionBlurConst) + 255) & ~255;
public:
	MotionBlur() {};
	MotionBlur(ID3D12Device1* device,Camera* cam, UINT width, UINT height, std::wstring shader);
	~MotionBlur();
	void Init();
	void CreateRootSignature();
	void CreatePipelineStateObject();
	void CreateConstantBuffers();

	void SetSRV(ID3D12Resource* textureSRV, int index);
	void SetUAV(int index);
	void SetConstBuffers(XMFLOAT4X4 prevVPMa);
	void Dispatch(ID3D12GraphicsCommandList* commandList, int constValue);
	CDescriptorHeapWrapper& GetResultDescriptor();
};

