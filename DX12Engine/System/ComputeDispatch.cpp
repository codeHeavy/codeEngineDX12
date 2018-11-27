#include "ComputeDispatch.h"

ComputeDispatch::ComputeDispatch(ID3D12Device1* device, UINT width, UINT height, std::wstring shader) : device(device), viewWidth(width), viewHeight(height), shaderName(shader)
{
	Init();
}

void ComputeDispatch::Init()
{
	CreateRootSignature();
	CreatePipelineStateObject();
}

ComputeDispatch::~ComputeDispatch()
{
	SAFE_RELEASE(rootSignature);
	SAFE_RELEASE(computePSO);
}

void ComputeDispatch::CreateRootSignature()
{
	//Init descriptor tables
	CD3DX12_DESCRIPTOR_RANGE range[2];
	range[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
	range[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
	
	CD3DX12_ROOT_PARAMETER rootParameters[3];
	rootParameters[0].InitAsDescriptorTable(1, &range[0], D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[1].InitAsDescriptorTable(1, &range[1], D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[2].InitAsConstants(4, 0);

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

	//device->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
	
	DirectX::CreateRootSignature(device, &descRootSignature, &rootSignature);
}

void ComputeDispatch::CreatePipelineStateObject()
{
	D3D12_COMPUTE_PIPELINE_STATE_DESC computeDesc = {};
	computeDesc.pRootSignature = rootSignature;
	computeDesc.CS = ShaderManager::LoadShader(shaderName.c_str());

	device->CreateComputePipelineState(&computeDesc, IID_PPV_ARGS(&computePSO));
}

void ComputeDispatch::SetSRV(ID3D12Resource* textureSRV, int index)
{
	srvHeap.Create(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 8, true);
	DirectX::CreateShaderResourceView(device, textureSRV, srvHeap.hCPU(index), false);
}

void ComputeDispatch::SetUAV( int index)
{
	HRESULT hr;
	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = viewWidth;
	texDesc.Height = viewHeight;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	hr = device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE, &texDesc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(&textureUAV));
	
	uavHeap.Create(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 8, true);

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;
	uavDesc.Texture2D.PlaneSlice = 0;
	//uavDesc.Buffer.NumElements = 1;
	//uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;

	device->CreateUnorderedAccessView(textureUAV.Get(), nullptr, &uavDesc, uavHeap.hCPU(index));
}

void ComputeDispatch::SetConstant(int index, float value)
{
}

void ComputeDispatch::Dispatch(ID3D12GraphicsCommandList* commandList, int constValue)
{
	//commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(NULL));
	ID3D12DescriptorHeap* ppHeaps1[] = { srvHeap.pDH.Get() };
	ID3D12DescriptorHeap* ppHeaps2[] = { uavHeap.pDH.Get() };
	commandList->SetComputeRootSignature(rootSignature);
	commandList->SetDescriptorHeaps(1,ppHeaps1);
	commandList->SetComputeRootDescriptorTable(0, srvHeap.hGPU(0));
	commandList->SetComputeRoot32BitConstants(2, 1, &constValue,0);
	commandList->SetDescriptorHeaps(1, ppHeaps2);
	commandList->SetComputeRootDescriptorTable(1, uavHeap.hGPU(0));
	commandList->SetPipelineState(computePSO);

	commandList->Dispatch(viewWidth, viewHeight, 1);
}

CDescriptorHeapWrapper& ComputeDispatch::GetResultDescriptor()
{
	return uavHeap;
}