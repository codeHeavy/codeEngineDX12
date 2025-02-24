#include "DefferedRenderer.h"
#include "DirectXHelpers.h"

DefferedRenderer::DefferedRenderer(ID3D12Device1* _device, UINT width, UINT height):
	device(_device), viewWidth(width), viewHeight(height)
{
	
}


DefferedRenderer::~DefferedRenderer()
{
	SAFE_RELEASE(viewCB);
	SAFE_RELEASE(lightCB);
	for (int i = 0; i < numRTV; ++i)
		SAFE_RELEASE(rtvTextures[numRTV]);
	SAFE_RELEASE(depthStencilTexture);

	SAFE_RELEASE(pipelineStateObject);
	SAFE_RELEASE( lightPassPSO);
	SAFE_RELEASE( lightPassShapePSO);
	SAFE_RELEASE(skyBoxPSO);
	SAFE_RELEASE(quadPSO);

	cbvsrvHeap.pDH->Release();
	dsvHeap.pDH->Release();
	rtvHeap.pDH->Release();
	srvHeap.pDH->Release();
	
	cbHeap.pDH->Release();
	pcbHeap.pDH->Release();

	SAFE_RELEASE(rootSignature);

	constantBufferGPUAddress		= nullptr;				// pointer to memory location
	constantBufferGPUAddressLight	= nullptr;				// pointer to memory location
	constantBufferGPUAddressShape	= nullptr;				// pointer to memory location

	delete sphereMesh;
	delete cubeMesh;
}

void DefferedRenderer::Init(ID3D12GraphicsCommandList* command)
{
	CreateConstantBuffers();
	CreateViews();
	CreateRootSignature();
	CreatePSO();
	SkyboxPSO();
	CreateLightPassPSO();
	CreateScreenQuadPSO();
	CreateLightPassPSOShape(L"ShapeVS.hlsl");
	CreateRTV();
	CreateDSV();
	sphereMesh = new Mesh("Assets/Models/sphere.obj", device, command);
	cubeMesh = new Mesh("Assets/Models/cube.obj", device, command);
}
void DefferedRenderer::CreateConstantBuffers()
{
	CD3DX12_HEAP_PROPERTIES heapProperty(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC resourceDesc;
	ZeroMemory(&resourceDesc, sizeof(resourceDesc));
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Alignment = 0;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;
	resourceDesc.MipLevels = 1;
	resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.Width = 1024 * 128;
	resourceDesc.Height = 1;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	device->CreateCommittedResource(&heapProperty, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&viewCB));

	resourceDesc.Width = 1024 * 128;
	device->CreateCommittedResource(&heapProperty, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&lightCB));
}

void DefferedRenderer::CreateViews()
{
	cbvsrvHeap.Create(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 32, true);
	cbHeap.Create(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 32, true);
	pcbHeap.Create(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 32, true);
	//Camera CBV
	D3D12_CONSTANT_BUFFER_VIEW_DESC	descBuffer;
	descBuffer.BufferLocation = viewCB->GetGPUVirtualAddress();
	descBuffer.SizeInBytes = ConstantBufferPerObjectAlignedSize;	//Constant buffer must be larger than 256 bytes

	const int numCBsForNow = 32;
	for (int i = 0; i < numCBsForNow; ++i)
	{
		descBuffer.BufferLocation = viewCB->GetGPUVirtualAddress() + i * ConstantBufferPerObjectAlignedSize;
		device->CreateConstantBufferView(&descBuffer, cbHeap.hCPU(i));
	}
	//device->CreateConstantBufferView(&descBuffer, cbvsrvHeap.hCPU(0));
	//descBuffer.SizeInBytes = viewCB->GetGPUVirtualAddress() + ((sizeof(ConstantBuffer) + 255) & ~255);	//Constant buffer must be larger than 256 bytes
	//device->CreateConstantBufferView(&descBuffer, cbvsrvHeap.hCPU(0));
	
	//Light CBV
	descBuffer.BufferLocation = lightCB->GetGPUVirtualAddress();
	descBuffer.SizeInBytes = PixelConstantBufferSize;
	//device->CreateConstantBufferView(&descBuffer, pcbHeap.hCPU(1));
	for (int i = 0; i < numCBsForNow; ++i)
	{
		descBuffer.BufferLocation = lightCB->GetGPUVirtualAddress() + i * PixelConstantBufferSize;
		device->CreateConstantBufferView(&descBuffer, pcbHeap.hCPU(i));
	}

	//------------------------
	ZeroMemory(&cbPerObj, sizeof(cbPerObj));
	//ZeroMemory(&pCb, sizeof(pCb));
	CD3DX12_RANGE readRange(0, 0);    // We do not intend to read from this resource on the CPU. (End is less than or equal to begin)
	viewCB->Map(0, &readRange, reinterpret_cast<void**>(&constantBufferGPUAddress));
	//viewCB->Map(0, &readRange, reinterpret_cast<void**>(&constantBufferGPUAddressShape));
	lightCB->Map(0, &readRange, reinterpret_cast<void**>(&constantBufferGPUAddressLight));
	// constant buffers must be 256 bytes aligned
	/*memcpy(constantBufferGPUAddress, &cbPerObj, sizeof(ConstantBuffer));
	memcpy(constantBufferGPUAddress + ConstantBufferPerObjectAlignedSize, &cbPerObj, sizeof(cbPerObj));*/
}

void DefferedRenderer::CreateRootSignature()
{
	//Init descriptor tables
	CD3DX12_DESCRIPTOR_RANGE range[3];
	//view dependent CBV
	range[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
	//light dependent CBV
	range[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
	//G-Buffer inputs
	range[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 12, 0);

	CD3DX12_ROOT_PARAMETER rootParameters[3];
	rootParameters[0].InitAsDescriptorTable(1, &range[0], D3D12_SHADER_VISIBILITY_VERTEX);
	rootParameters[1].InitAsDescriptorTable(1, &range[1], D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameters[2].InitAsDescriptorTable(1, &range[2], D3D12_SHADER_VISIBILITY_ALL);

	CD3DX12_ROOT_SIGNATURE_DESC descRootSignature;
	descRootSignature.Init(3, rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS);

	CD3DX12_STATIC_SAMPLER_DESC StaticSamplers[1];
	StaticSamplers[0].Init(0, D3D12_FILTER_ANISOTROPIC);
	descRootSignature.NumStaticSamplers = 1;
	descRootSignature.pStaticSamplers = StaticSamplers;


	Microsoft::WRL::ComPtr<ID3DBlob> rootSigBlob, errorBlob;

	D3D12SerializeRootSignature(&descRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &rootSigBlob, &errorBlob);

	device->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
}

void DefferedRenderer::CreatePSO()
{
	// Create Input layout
	D3D12_INPUT_ELEMENT_DESC inputLayout[] =
	{
		{ "POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
		{ "TEXCOORD" , 0, DXGI_FORMAT_R32G32_FLOAT,0,12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT,0,20,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT,0,32,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 }
	};

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};

	inputLayoutDesc.NumElements = sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
	inputLayoutDesc.pInputElementDescs = inputLayout;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC descPipelineState;
	ZeroMemory(&descPipelineState, sizeof(descPipelineState));
	descPipelineState.VS = ShaderManager::CompileVSShader(L"VertexShader.hlsl");
	descPipelineState.PS = ShaderManager::CompilePSShader(L"DeferredPixelShader.hlsl");
	descPipelineState.InputLayout = inputLayoutDesc;
	descPipelineState.pRootSignature = rootSignature;
	descPipelineState.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	descPipelineState.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	descPipelineState.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	descPipelineState.SampleMask = UINT_MAX;
	descPipelineState.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	descPipelineState.NumRenderTargets = numRTV;
	descPipelineState.RTVFormats[0] = mRtvFormat[0];
	descPipelineState.RTVFormats[1] = mRtvFormat[1];
	descPipelineState.RTVFormats[2] = mRtvFormat[2];
	descPipelineState.RTVFormats[3] = mRtvFormat[3];
	descPipelineState.DSVFormat = mDsvFormat;
	descPipelineState.SampleDesc.Count = 1;

	device->CreateGraphicsPipelineState(&descPipelineState, IID_PPV_ARGS(&pipelineStateObject));
}

void DefferedRenderer::CreateLightPassPSO()
{
	HRESULT hr;
	D3D12_GRAPHICS_PIPELINE_STATE_DESC descPipelineState;
	ZeroMemory(&descPipelineState, sizeof(descPipelineState));

	descPipelineState.VS = ShaderManager::CompileVSShader(L"ScreenQuadVS.hlsl");
	descPipelineState.PS = ShaderManager::CompilePSShader(L"LightPassPS.hlsl");
	descPipelineState.InputLayout.pInputElementDescs = nullptr;
	descPipelineState.InputLayout.NumElements = 0;// _countof(inputLayout);
	descPipelineState.pRootSignature = rootSignature;
	descPipelineState.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	descPipelineState.DepthStencilState.DepthEnable = false;
	descPipelineState.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	descPipelineState.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	descPipelineState.RasterizerState.DepthClipEnable = false;
	descPipelineState.SampleMask = UINT_MAX;
	descPipelineState.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	descPipelineState.NumRenderTargets = 1;
	descPipelineState.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	descPipelineState.SampleDesc.Count = 1;

	hr = device->CreateGraphicsPipelineState(&descPipelineState, IID_PPV_ARGS(&lightPassPSO));
}

void DefferedRenderer::CreateScreenQuadPSO()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC descPipelineState;
	ZeroMemory(&descPipelineState, sizeof(descPipelineState));
	descPipelineState.VS = ShaderManager::LoadShader(L"ScreenQuadVS.cso");
	descPipelineState.PS = ShaderManager::LoadShader(L"ScreenQuadPS.cso");
	descPipelineState.InputLayout.pInputElementDescs = nullptr;
	descPipelineState.InputLayout.NumElements = 0;// _countof(inputLayout);
	descPipelineState.pRootSignature = rootSignature;
	descPipelineState.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	descPipelineState.DepthStencilState.DepthEnable = false;
	descPipelineState.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	descPipelineState.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	descPipelineState.RasterizerState.DepthClipEnable = false;
	descPipelineState.SampleMask = UINT_MAX;
	descPipelineState.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	descPipelineState.NumRenderTargets = 1;
	descPipelineState.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	descPipelineState.SampleDesc.Count = 1;
	device->CreateGraphicsPipelineState(&descPipelineState, IID_PPV_ARGS(&quadPSO));
}

void DefferedRenderer::CreateLightPassPSOShape(std::wstring shapeShader)
{
	D3D12_INPUT_ELEMENT_DESC inputLayout[] =
	{
		{ "POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0   },
		{ "TEXCOORD" , 0, DXGI_FORMAT_R32G32_FLOAT,0,12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT,0,20,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0  },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT,0,32,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 }
	};

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};

	inputLayoutDesc.NumElements = sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
	inputLayoutDesc.pInputElementDescs = inputLayout;

	HRESULT hr;
	D3D12_GRAPHICS_PIPELINE_STATE_DESC descPipelineState;
	ZeroMemory(&descPipelineState, sizeof(descPipelineState));

	auto rasterizer = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	rasterizer.CullMode = D3D12_CULL_MODE_NONE;

	descPipelineState.VS = ShaderManager::CompileVSShader(L"ShapeVS.hlsl");
	descPipelineState.PS = ShaderManager::CompilePSShader(L"LightShapePS.hlsl");
	descPipelineState.InputLayout = inputLayoutDesc;
	descPipelineState.pRootSignature = rootSignature;
	descPipelineState.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	descPipelineState.DepthStencilState.DepthEnable = false;
	descPipelineState.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	descPipelineState.RasterizerState = rasterizer;
	descPipelineState.RasterizerState.DepthClipEnable = false;
	descPipelineState.SampleMask = UINT_MAX;
	descPipelineState.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	descPipelineState.NumRenderTargets = 1;
	descPipelineState.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	descPipelineState.SampleDesc.Count = 1;

	hr = device->CreateGraphicsPipelineState(&descPipelineState, IID_PPV_ARGS(&lightPassShapePSO));
}

void DefferedRenderer::CreateRTV()
{
	HRESULT hr;

	rtvHeap.Create(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, numRTV);
	CD3DX12_HEAP_PROPERTIES heapProperty(D3D12_HEAP_TYPE_DEFAULT);

	D3D12_RESOURCE_DESC resourceDesc;
	ZeroMemory(&resourceDesc, sizeof(resourceDesc));
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Alignment = 0;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;
	resourceDesc.MipLevels = 1;

	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.Width = viewWidth;
	resourceDesc.Height = viewHeight;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	D3D12_CLEAR_VALUE clearVal;
	clearVal.Color[0] = mClearColor[0];
	clearVal.Color[1] = mClearColor[1];
	clearVal.Color[2] = mClearColor[2];
	clearVal.Color[3] = mClearColor[3];

	for (int i = 0; i < numRTV; i++) {
		resourceDesc.Format = mRtvFormat[i];
		clearVal.Format = mRtvFormat[i];
		device->CreateCommittedResource(&heapProperty, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_RENDER_TARGET, &clearVal, IID_PPV_ARGS(&rtvTextures[i]));
	}
	device->CreateCommittedResource(&heapProperty, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_RENDER_TARGET, &clearVal, IID_PPV_ARGS(&finalTexture));
	finalTexture = rtvTextures[QUAD_INDEX];
	
	D3D12_RENDER_TARGET_VIEW_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.Texture2D.MipSlice = 0;
	desc.Texture2D.PlaneSlice = 0;

	desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	for (int i = 0; i < numRTV; i++) {
		desc.Format = mRtvFormat[i];
		device->CreateRenderTargetView(rtvTextures[i], &desc, rtvHeap.hCPU(i));
	}

	//Create SRV for RTs
	D3D12_SHADER_RESOURCE_VIEW_DESC descSRV;

	ZeroMemory(&descSRV, sizeof(descSRV));
	descSRV.Texture2D.MipLevels = resourceDesc.MipLevels;
	descSRV.Texture2D.MostDetailedMip = 0;
	descSRV.Format = resourceDesc.Format;
	descSRV.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	descSRV.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	srvHeap.Create(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 32, true);

	for (int i = 0; i < numRTV; i++) {
		descSRV.Format = mRtvFormat[i];
		device->CreateShaderResourceView(rtvTextures[i], &descSRV, cbvsrvHeap.hCPU(i));
	}
}

void DefferedRenderer::CreateDSV()
{
	HRESULT hr;
	dsvHeap.Create(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1);

	CD3DX12_HEAP_PROPERTIES heapProperty(D3D12_HEAP_TYPE_DEFAULT);

	D3D12_RESOURCE_DESC resourceDesc;
	ZeroMemory(&resourceDesc, sizeof(resourceDesc));
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Alignment = 0;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;
	resourceDesc.MipLevels = 1;
	resourceDesc.Format = mDsvFormat;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.Width = (UINT)viewWidth;
	resourceDesc.Height = (UINT)viewHeight;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE clearVal;
	clearVal = { mDsvFormat , mClearDepth };

	hr = device->CreateCommittedResource(&heapProperty, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearVal, IID_PPV_ARGS(&depthStencilTexture));
	D3D12_DEPTH_STENCIL_VIEW_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.Texture2D.MipSlice = 0;
	desc.Format = resourceDesc.Format;
	desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	desc.Flags = D3D12_DSV_FLAG_NONE;

	device->CreateDepthStencilView(depthStencilTexture, &desc, dsvHeap.hCPU(0));

	D3D12_SHADER_RESOURCE_VIEW_DESC descSRV;

	ZeroMemory(&descSRV, sizeof(descSRV));
	descSRV.Texture2D.MipLevels = resourceDesc.MipLevels;
	descSRV.Texture2D.MostDetailedMip = 0;
	descSRV.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	descSRV.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	descSRV.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;


	device->CreateShaderResourceView(depthStencilTexture, &descSRV, cbvsrvHeap.hCPU(RTV_NUM));
}

void DefferedRenderer::ApplyGBufferPSO(ID3D12GraphicsCommandList * command, bool bSetPSO, GameObject* _gameObj, Camera* _camera, const PSConstantBuffer& pixelCb,int textureIndex)
{
	ID3D12DescriptorHeap* ppHeaps[] = { srvHeap.pDH.Get() };
	gameObj = _gameObj;
	camera = _camera;
	command->SetPipelineState(pipelineStateObject);

	for (int i = 0; i < numRTV; i++)
		command->ClearRenderTargetView(rtvHeap.hCPU(i), mClearColor, 0, nullptr);

	command->ClearDepthStencilView(dsvHeap.hCPUHeapStart, D3D12_CLEAR_FLAG_DEPTH, mClearDepth, 0xff, 0, nullptr);

	command->OMSetRenderTargets(numRTV, &rtvHeap.hCPUHeapStart, true, &dsvHeap.hCPUHeapStart);
	command->SetDescriptorHeaps(1, ppHeaps);
	command->SetGraphicsRootSignature(rootSignature);
	command->SetGraphicsRootDescriptorTable(2, srvHeap.hGPU(textureIndex));

	ID3D12DescriptorHeap* ppHeap2[] = { pcbHeap.pDH.Get() };
	command->SetDescriptorHeaps(1, ppHeap2);
	command->SetGraphicsRootDescriptorTable(1, pcbHeap.hGPU(0));
	memcpy(constantBufferGPUAddressLight, &pixelCb, sizeof(PSConstantBuffer));
}

void DefferedRenderer::ApplyLightingPSO(ID3D12GraphicsCommandList * command, bool bSetPSO, const PSConstantBuffer& pixelCb)
{
	for (int i = 0; i < numRTV; i++)
		command->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(rtvTextures[i], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));

	command->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(rtvTextures[QUAD_INDEX], D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));
	command->SetPipelineState(lightPassPSO);

	command->OMSetRenderTargets(1, &rtvHeap.hCPU(QUAD_INDEX), true, nullptr);

	ID3D12DescriptorHeap* ppHeap2[] = { pcbHeap.pDH.Get() };
	command->SetDescriptorHeaps(1, ppHeap2);
	command->SetGraphicsRootDescriptorTable(1, pcbHeap.hGPU(0));
	memcpy(constantBufferGPUAddressLight, &pixelCb, sizeof(PSConstantBuffer));
	
	ID3D12DescriptorHeap* ppHeaps[] = { cbvsrvHeap.pDH.Get() };
	command->SetDescriptorHeaps(1, ppHeaps);
	command->SetGraphicsRootDescriptorTable(2, cbvsrvHeap.hGPU(0));
}

void DefferedRenderer::ApplyLightingShapePSO(ID3D12GraphicsCommandList * command, bool bSetPSO, const PSConstantBuffer& pixelCb)
{
	for (int i = 0; i < numRTV; i++)
		command->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(rtvTextures[i], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));

	command->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(rtvTextures[LIGHTSHAPE_INDEX], D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));
	
	//command->ClearRenderTargetView(rtvHeap.hCPU(numRTV-1), mClearColor, 0, nullptr);
	//command->OMSetRenderTargets(1, &rtvHeap.hCPU(numRTV-1), true, nullptr);
	
	command->ClearRenderTargetView(rtvHeap.hCPU(LIGHTSHAPE_INDEX), mClearColor, 0, nullptr);
	command->OMSetRenderTargets(1, &rtvHeap.hCPU(LIGHTSHAPE_INDEX), true, nullptr);
	command->SetPipelineState(lightPassShapePSO);

	ID3D12DescriptorHeap* ppHeap2[] = { pcbHeap.pDH.Get() };
	command->SetDescriptorHeaps(1, ppHeap2);
	command->SetGraphicsRootDescriptorTable(1, pcbHeap.hGPU(0));
	
	ID3D12DescriptorHeap* ppHeaps[] = { cbvsrvHeap.pDH.Get() };
	command->SetDescriptorHeaps(1, ppHeaps);
	command->SetGraphicsRootDescriptorTable(2, cbvsrvHeap.hGPU(0));
}

void DefferedRenderer::Render(ID3D12GraphicsCommandList * commandList)
{
	ID3D12DescriptorHeap* ppHeaps[] = { cbHeap.pDH.Get() };
	commandList->SetDescriptorHeaps(1, ppHeaps);
	auto a = gameObj->GetWorldMatrix();

	XMMATRIX viewMat = XMLoadFloat4x4(&camera->GetViewMatrix());					// load view matrix
	XMMATRIX projMat = XMLoadFloat4x4(&camera->GetProjectionMatrix());				// load projection matrix
	XMMATRIX wvpMat = XMMatrixTranspose(XMLoadFloat4x4(&gameObj->GetWorldMatrix())) * viewMat * projMat; // create wvp matrix
	XMStoreFloat4x4(&cbPerObj.worldViewProjectionMatrix, XMMatrixTranspose( wvpMat));	// store transposed wvp matrix in constant buffer
	XMStoreFloat4x4(&cbPerObj.worldMatrix, XMLoadFloat4x4(&gameObj->GetWorldMatrix()));	// store transposed world matrix in constant buffer

	memcpy(constantBufferGPUAddress, &cbPerObj, sizeof(ConstantBuffer));
	commandList->SetGraphicsRootDescriptorTable(0, cbHeap.hGPU(0));

	commandList->IASetVertexBuffers(0, 1, &gameObj->GetMesh()->GetvBufferView());
	commandList->IASetIndexBuffer(&gameObj->GetMesh()->GetiBufferView());
	commandList->DrawIndexedInstanced(gameObj->GetMesh()->GetNumIndices(), 1, 0, 0, 0);
}

void DefferedRenderer::RenderLightShape(ID3D12GraphicsCommandList * command, const PSConstantBuffer& pixelCb)
{
	ID3D12DescriptorHeap* ppHeaps[] = { cbHeap.pDH.Get() };
	command->SetDescriptorHeaps(1, ppHeaps);

	GameObject sphereObject(sphereMesh);
	sphereObject.SetPosition(pixelCb.pLight.Position);
	sphereObject.SetScale(XMFLOAT3(5, 5, 5));
	sphereObject.UpdateWorldMatrix();

	XMMATRIX viewMat = XMLoadFloat4x4(&camera->GetViewMatrix());					// load view matrix
	XMMATRIX projMat = XMLoadFloat4x4(&camera->GetProjectionMatrix());				// load projection matrix
	XMMATRIX wvpMat = XMMatrixTranspose(XMLoadFloat4x4(&sphereObject.GetWorldMatrix())) * viewMat * projMat; // create wvp matrix
	XMStoreFloat4x4(&pCb.worldViewProjectionMatrix, XMMatrixTranspose(wvpMat));	// store transposed wvp matrix in constant buffer
	XMStoreFloat4x4(&pCb.worldMatrix, XMLoadFloat4x4(&sphereObject.GetWorldMatrix()));	// store transposed world matrix in constant buffer

	memcpy(constantBufferGPUAddress + ConstantBufferPerObjectAlignedSize, &pCb, sizeof(ConstantBuffer));
	command->SetGraphicsRootDescriptorTable(0, cbHeap.hGPU(1));

	command->IASetVertexBuffers(0, 1, &sphereObject.GetMesh()->GetvBufferView());
	command->IASetIndexBuffer(&sphereObject.GetMesh()->GetiBufferView());
	command->DrawIndexedInstanced(sphereObject.GetMesh()->GetNumIndices(), 1, 0, 0, 0);

	command->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(rtvTextures[LIGHTSHAPE_INDEX], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
}

void DefferedRenderer::SetSRV(ID3D12Resource* textureSRV, DXGI_FORMAT format, int index)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	CreateShaderResourceView(device, textureSRV, srvHeap.hCPU(index), false);
	//device->CreateShaderResourceView(textureSRV, &srvDesc, srvHeap.hCPU(index));
}

void DefferedRenderer::SetCubeSRV(ID3D12Resource* textureSRV, int index)
{
	CreateShaderResourceView(device, textureSRV, srvHeap.hCPU(index), true);
}

void DefferedRenderer::DrawLightPass(ID3D12GraphicsCommandList * commandList)
{
	D3D12_INDEX_BUFFER_VIEW ibv;
	ibv.Format = DXGI_FORMAT_R32_UINT;
	ibv.BufferLocation = 0;
	ibv.SizeInBytes = 0;

	D3D12_VERTEX_BUFFER_VIEW vbv;
	vbv.BufferLocation = 0;
	vbv.SizeInBytes = 0;
	vbv.StrideInBytes = 0;
	commandList->IASetVertexBuffers(0, 0, &vbv);
	commandList->IASetIndexBuffer(&ibv);
	commandList->DrawInstanced(4, 1, 0, 0);

}

void DefferedRenderer::SkyboxPSO()
{
	HRESULT hr;
	D3D12_INPUT_ELEMENT_DESC inputLayout[] =
	{
		{ "POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
	{ "TEXCOORD" , 0, DXGI_FORMAT_R32G32_FLOAT,0,12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
	{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT,0,20,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
	{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT,0,32,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 }
	};

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};

	inputLayoutDesc.NumElements = sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
	inputLayoutDesc.pInputElementDescs = inputLayout;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC descPipelineState;
	ZeroMemory(&descPipelineState, sizeof(descPipelineState));
	auto depthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

	depthStencilState.DepthEnable = true;
	depthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	depthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	auto rasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	rasterizerState.DepthClipEnable = true;
	rasterizerState.CullMode = D3D12_CULL_MODE_FRONT;
	rasterizerState.FillMode = D3D12_FILL_MODE_SOLID;

	descPipelineState.VS = ShaderManager::CompileVSShader(L"SkyVS.hlsl");
	descPipelineState.PS = ShaderManager::CompilePSShader(L"SkyPS.hlsl");
	descPipelineState.InputLayout = inputLayoutDesc;
	descPipelineState.pRootSignature = rootSignature;
	descPipelineState.DepthStencilState = depthStencilState;
	descPipelineState.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	descPipelineState.RasterizerState = rasterizerState;
	descPipelineState.SampleMask = UINT_MAX;
	descPipelineState.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	descPipelineState.NumRenderTargets = 1;
	descPipelineState.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	descPipelineState.SampleDesc.Count = 1;
	descPipelineState.DSVFormat = mDsvFormat;

	hr = device->CreateGraphicsPipelineState(&descPipelineState, IID_PPV_ARGS(&skyBoxPSO));
}

void DefferedRenderer::RenderSkybox(ID3D12GraphicsCommandList * command, D3D12_CPU_DESCRIPTOR_HANDLE &rtvHandle, int skyboxIndex)
{
	command->SetPipelineState(skyBoxPSO);
	//command->OMSetRenderTargets(1, &rtvHandle, true, &dsvHeap.hCPUHeapStart);
	command->OMSetRenderTargets(1, &rtvHeap.hCPU(QUAD_INDEX), true, &dsvHeap.hCPUHeapStart);
	ID3D12DescriptorHeap* ppHeaps[] = { cbHeap.pDH.Get() };
	ID3D12DescriptorHeap* ppSrvHeaps[] = { srvHeap.pDH.Get() };
	
	GameObject cubeObj(cubeMesh);
	cubeObj.UpdateWorldMatrix();

	auto viewNoMove = camera->GetViewMatrix();
	viewNoMove._41 = 0;
	viewNoMove._42 = 0;
	viewNoMove._43 = 0;

	XMMATRIX viewMat = XMLoadFloat4x4(&viewNoMove);					// load view matrix
	XMMATRIX projMat = XMLoadFloat4x4(&camera->GetProjectionMatrix());				// load projection matrix
	XMMATRIX wvpMat = viewMat * projMat; // create wvp matrix
	XMStoreFloat4x4(&skyCb.worldViewProjectionMatrix, XMMatrixTranspose(wvpMat));	// store transposed wvp matrix in constant buffer
	XMStoreFloat4x4(&skyCb.worldMatrix, XMMatrixIdentity());	// store transposed world matrix in constant buffer
	
	memcpy(constantBufferGPUAddress + ConstantBufferPerObjectAlignedSize * 2, &skyCb, sizeof(ConstantBuffer));

	command->SetDescriptorHeaps(1, ppSrvHeaps);
	command->SetGraphicsRootDescriptorTable(2, srvHeap.hGPU(skyboxIndex)); //Set skybox texture
	command->SetDescriptorHeaps(1, ppHeaps);
	
	command->SetGraphicsRootDescriptorTable(0, cbHeap.hGPU(2)); // set constant buffer with view and projection matrices
	
	command->IASetVertexBuffers(0, 1, &cubeObj.GetMesh()->GetvBufferView());
	command->IASetIndexBuffer(&cubeObj.GetMesh()->GetiBufferView());
	command->DrawIndexedInstanced(cubeObj.GetMesh()->GetNumIndices(), 1, 0, 0, 0);
}

void DefferedRenderer::SetPBRTextures(ID3D12Resource* irradianceTextureCube, ID3D12Resource* prefilterTextureCube, ID3D12Resource* brdf)
{
	int irradianceIndex = numRTV + 1;
	int brdfTextureIndex = numRTV + 2;
	int prefilterTexIndex = numRTV + 3;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.Texture2D.MipLevels = 1;
	device->CreateShaderResourceView(irradianceTextureCube, &srvDesc, cbvsrvHeap.hCPU(irradianceIndex));

	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	device->CreateShaderResourceView(brdf, &srvDesc, cbvsrvHeap.hCPU(brdfTextureIndex));
	
	srvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.Texture2D.MipLevels = 5;
	device->CreateShaderResourceView(prefilterTextureCube, &srvDesc, cbvsrvHeap.hCPU(prefilterTexIndex));
}

void DefferedRenderer::DrawFinal(ID3D12GraphicsCommandList* commandList, D3D12_CPU_DESCRIPTOR_HANDLE & rtvHandle, CDescriptorHeapWrapper &uavHeap, bool postProcess)
{
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(rtvTextures[QUAD_INDEX], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
	commandList->ClearRenderTargetView(rtvHandle, mClearColor, 0, nullptr);
	commandList->OMSetRenderTargets(1, &rtvHandle, true, nullptr);
	commandList->SetPipelineState(quadPSO);
	if (postProcess)
	{
		ID3D12DescriptorHeap* ppHeaps[] = { uavHeap.pDH.Get() };
		commandList->SetDescriptorHeaps(1, ppHeaps);
		commandList->SetGraphicsRootDescriptorTable(2, uavHeap.hGPU(0));
	}
	else
	{
		ID3D12DescriptorHeap* ppHeaps[] = { cbvsrvHeap.pDH.Get() };
		commandList->SetDescriptorHeaps(1, ppHeaps);
		commandList->SetGraphicsRootDescriptorTable(2, cbvsrvHeap.hGPU(QUAD_INDEX));
	}
	DrawLightPass(commandList); // Draws full screen quad with null vertex buffer.
}

ID3D12Resource* DefferedRenderer::GetFinalTexture()
{
	return finalTexture;
}

ID3D12Resource * DefferedRenderer::GetWorldPosTexture()
{
	return rtvTextures[WORLDPOS_INDEX];
}
