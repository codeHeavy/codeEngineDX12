#pragma once
#include "../stdafx.h"
#include "Vertex.h"
#include "DirectxHelper.h"
#include "ConstantBuffer.h"
#include "ShaderManager.h"
#include "GameObject.h"
#include "Camera.h"
#include <string>

enum GBufferRenderTargetOrder
{
	ALBEDO_INDEX = 0,
	NORMAL_INDEX,
	WORLDPOS_INDEX,
	ROUGHNESS_INDEX,
	METALNESS_INDEX,
	LIGHTSHAPE_INDEX,
	QUAD_INDEX,
	RTV_NUM
};

class DefferedRenderer
{
private:
	const static int numRTV = 7;
	ID3D12Device1* device;

	ID3D12Resource* viewCB;
	ID3D12Resource* lightCB;
	ID3D12Resource* rtvTextures[numRTV];
	ID3D12Resource* depthStencilTexture;

	ID3D12PipelineState* pipelineStateObject;
	ID3D12PipelineState* lightPassPSO;
	ID3D12PipelineState* lightPassShapePSO;
	ID3D12PipelineState* skyBoxPSO;
	ID3D12PipelineState* prefilterEnvMapPSO;
	ID3D12PipelineState* quadPSO;

	CDescriptorHeapWrapper cbvsrvHeap;
	CDescriptorHeapWrapper dsvHeap;
	CDescriptorHeapWrapper rtvHeap;
	CDescriptorHeapWrapper srvHeap;

	CDescriptorHeapWrapper cbHeap;
	CDescriptorHeapWrapper pcbHeap;
	PSConstantBuffer pBuffer;

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
	void CreateScreenQuadPSO();
	void CreateLightPassPSOShape(std::wstring shapeShader);
	void CreateRTV();
	void CreateDSV();
	void SkyboxPSO();

	UINT8* constantBufferGPUAddress;				// pointer to memory location
	UINT8* constantBufferGPUAddressLight;				// pointer to memory location
	UINT8* constantBufferGPUAddressShape;				// pointer to memory location
	ConstantBuffer cbPerObj;
	ConstantBuffer pCb;
	ConstantBuffer skyCb;
	static const int ConstantBufferPerObjectAlignedSize = (sizeof(ConstantBuffer) + 255) & ~255;
	static const int PixelConstantBufferSize = (sizeof(PSConstantBuffer) + 255) & ~255;

	DXGI_FORMAT mDsvFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	DXGI_FORMAT mRtvFormat[7] = {	DXGI_FORMAT_R8G8B8A8_UNORM, //Albedo
									DXGI_FORMAT_R8G8B8A8_SNORM, //Normal
									DXGI_FORMAT_R32G32B32A32_FLOAT, //WorldPos
									DXGI_FORMAT_R11G11B10_FLOAT, //Roughness
									DXGI_FORMAT_R8_UNORM,	//Metalness
									DXGI_FORMAT_R8G8B8A8_UNORM, //Lightpass
									DXGI_FORMAT_R32G32B32A32_FLOAT }; //Final
	GameObject* gameObj;
	Camera* camera;
	Mesh* sphereMesh;
	Mesh* cubeMesh;
public:
	DefferedRenderer(ID3D12Device1* _device, UINT width, UINT height);
	~DefferedRenderer();

	void Init(ID3D12GraphicsCommandList* command);
	void ApplyGBufferPSO(ID3D12GraphicsCommandList * command, bool bSetPSO, GameObject* gameObj, Camera* camera, const PSConstantBuffer& pixelCb,int textureIndex);
	void ApplyLightingPSO(ID3D12GraphicsCommandList * command, bool bSetPSO, const PSConstantBuffer& pixelCb);
	void ApplyLightingShapePSO(ID3D12GraphicsCommandList * command, bool bSetPSO, const PSConstantBuffer& pixelCb);
	void Render(ID3D12GraphicsCommandList * command);
	void RenderLightShape(ID3D12GraphicsCommandList * command, const PSConstantBuffer& pixelCb);
	void SetSRV(ID3D12Resource* textureSRV, DXGI_FORMAT format, int index);
	void SetCubeSRV(ID3D12Resource* textureSRV, int index);
	void DrawLightPass(ID3D12GraphicsCommandList * commandList);
	void RenderSkybox(ID3D12GraphicsCommandList * command, D3D12_CPU_DESCRIPTOR_HANDLE &rtvHandle, int skyboxIndex);
	void SetPBRTextures(ID3D12Resource* irradianceTextureCube, ID3D12Resource* prefilterTextureCube, ID3D12Resource* brdf);
	void DrawResult(ID3D12GraphicsCommandList* commandList, D3D12_CPU_DESCRIPTOR_HANDLE & rtvHandle);
};

