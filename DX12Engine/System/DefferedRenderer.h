#pragma once
#include "../stdafx.h"
#include "Vertex.h"
#include "DirectxHelper.h"
#include "ConstantBuffer.h"
#include "ShaderManager.h"
#include "GameObject.h"
#include "Camera.h"

class DefferedRenderer
{
private:
	const static int numRTV = 3;
	ID3D12Device1* device;

	ID3D12Resource* viewCB;
	ID3D12Resource* lightCB;
	ID3D12Resource* rtvTextures[numRTV];
	ID3D12Resource* depthStencilTexture;

	ID3D12PipelineState* pipelineStateObject;
	ID3D12PipelineState* lightPassPSO;

	CDescriptorHeapWrapper cbvsrvHeap;
	CDescriptorHeapWrapper dsvHeap;
	CDescriptorHeapWrapper rtvHeap;
	CDescriptorHeapWrapper srvHeap;

	ID3D12RootSignature* rootSignature;

	UINT viewWidth;
	UINT viewHeight;
	float mClearColor[4] = { 0.0,0.0f,0.0f,1.0f };
	float mClearDepth = 1.0f;

	void CreateConstantBuffers();
	void CreateViews();
	void CreateRootSignature();
	void CreatePSO();
	void CreateLightPassPSO();
	void CreateRTV();
	void CreateDSV();

	void ApplyGBufferPSO(ID3D12GraphicsCommandList * command, bool bSetPSO);
	void ApplyLightingPSO(ID3D12GraphicsCommandList * command, bool bSetPSO);

	void Render(ID3D12GraphicsCommandList * command, GameObject* gameObj);

	DXGI_FORMAT mDsvFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	DXGI_FORMAT mRtvFormat[3] = { DXGI_FORMAT_R11G11B10_FLOAT, DXGI_FORMAT_R8G8B8A8_SNORM, DXGI_FORMAT_R8G8B8A8_UNORM };
public:
	void Init();
	DefferedRenderer(ID3D12Device1* _device, UINT width, UINT height);
	~DefferedRenderer();
};

