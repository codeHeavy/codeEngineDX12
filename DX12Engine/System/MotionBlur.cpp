#include "MotionBlur.h"

MotionBlur::MotionBlur(ID3D12Device1* device,Camera* cam, UINT width, UINT height, std::wstring shader) : device(device), camera(cam), viewWidth(width), viewHeight(height), shaderName(shader)
{
	Init();
}

void MotionBlur::Init()
{
	CreateRootSignature();
	CreateConstantBuffers();
	CreatePipelineStateObject();
}

MotionBlur::~MotionBlur()
{
	SAFE_RELEASE(rootSignature);
	SAFE_RELEASE(computePSO);
}

void MotionBlur::CreateRootSignature()
{
	//Init descriptor tables
	CD3DX12_DESCRIPTOR_RANGE range[3];
	range[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 8, 0);
	range[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
	range[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);

	CD3DX12_ROOT_PARAMETER rootParameters[3];
	rootParameters[0].InitAsDescriptorTable(1, &range[0], D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[1].InitAsDescriptorTable(1, &range[1], D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[2].InitAsConstantBufferView(0);

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
	srvHeap.Create(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 8, true);
	uavHeap.Create(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 8, true);
}

void MotionBlur::CreatePipelineStateObject()
{
	D3D12_COMPUTE_PIPELINE_STATE_DESC computeDesc = {};
	computeDesc.pRootSignature = rootSignature;
	computeDesc.CS = ShaderManager::LoadShader(shaderName.c_str());

	device->CreateComputePipelineState(&computeDesc, IID_PPV_ARGS(&computePSO));
}

void MotionBlur::SetSRV(ID3D12Resource* textureSRV, int index)
{
	DirectX::CreateShaderResourceView(device, textureSRV, srvHeap.hCPU(index), false);
}

void MotionBlur::SetUAV(int index)
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

	

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;
	uavDesc.Texture2D.PlaneSlice = 0;
	//uavDesc.Buffer.NumElements = 1;
	//uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;

	device->CreateUnorderedAccessView(textureUAV.Get(), nullptr, &uavDesc, uavHeap.hCPU(index));
}

void MotionBlur::Dispatch(ID3D12GraphicsCommandList* commandList, int constValue)
{
	//commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(NULL));
	ID3D12DescriptorHeap* ppHeaps1[] = { srvHeap.pDH.Get() };
	ID3D12DescriptorHeap* ppHeaps2[] = { uavHeap.pDH.Get() };
	ID3D12DescriptorHeap* ppHeaps3[] = { cbHeap.pDH.Get() };
	commandList->SetComputeRootSignature(rootSignature);

	commandList->SetDescriptorHeaps(1, ppHeaps1);
	commandList->SetComputeRootDescriptorTable(0, srvHeap.hGPU(0)); // Result input

	commandList->SetDescriptorHeaps(1, ppHeaps2);
	commandList->SetComputeRootDescriptorTable(1, uavHeap.hGPU(0));

	commandList->SetDescriptorHeaps(1, ppHeaps3);
	commandList->SetGraphicsRootDescriptorTable(2, cbHeap.hGPU(0));

	commandList->SetPipelineState(computePSO);

	commandList->Dispatch(viewWidth / 16, viewHeight / 16, 1);
}

CDescriptorHeapWrapper& MotionBlur::GetResultDescriptor()
{
	return uavHeap;
}

void MotionBlur::CreateConstantBuffers()
{
	cbHeap.Create(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 8, true);

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

	device->CreateCommittedResource(&heapProperty, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&constBuffer));

	D3D12_CONSTANT_BUFFER_VIEW_DESC	descBuffer;
	descBuffer.BufferLocation = constBuffer->GetGPUVirtualAddress();
	descBuffer.SizeInBytes = ConstantBufferPerObjectAlignedSize;	//Constant buffer must be larger than 256 bytes

	descBuffer.BufferLocation = constBuffer->GetGPUVirtualAddress() + ConstantBufferPerObjectAlignedSize;
	device->CreateConstantBufferView(&descBuffer, cbHeap.hCPU(0));

	ZeroMemory(&constants, sizeof(constants));
	CD3DX12_RANGE readRange(0, 0);    // We do not intend to read from this resource on the CPU. (End is less than or equal to begin)
	constBuffer->Map(0, &readRange, reinterpret_cast<void**>(&constantBufferGPUAddress));
}

void MotionBlur::SetConstBuffers(XMFLOAT4X4 prevVPMat)
{
	XMStoreFloat4x4(&constants.preViewProjection, XMMatrixTranspose(XMLoadFloat4x4(&prevVPMat)));	// store transposed wvp matrix in constant buffer

	memcpy(constantBufferGPUAddress, &constants, sizeof(MotionBlurConst));

}