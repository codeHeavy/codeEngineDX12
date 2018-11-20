#include "ComputeDispatch.h"

ComputeDispatch::ComputeDispatch(ID3D12Device1* device) : device(device)
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

	device->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));

	srvHeap.Create(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 32, true);
}

void ComputeDispatch::CreatePipelineStateObject()
{
	D3D12_COMPUTE_PIPELINE_STATE_DESC computeDesc = {};
	computeDesc.pRootSignature = rootSignature;
	computeDesc.CS = ShaderManager::LoadShader(L"ComputeShaderDefault.cso");

	device->CreateComputePipelineState(&computeDesc, IID_PPV_ARGS(&computePSO));
}

void ComputeDispatch::SetShader(ID3D12GraphicsCommandList * commandList)
{
	commandList->SetComputeRootSignature(rootSignature);
	commandList->SetComputeRootDescriptorTable(0, srvHeap.hGPU(0));
	commandList->SetPipelineState(computePSO);
}

//void ComputeDispatch::SetTextureUAV(ID3D12GraphicsCommandList* commandList, Texture* textureUAV)
//{
//	ID3D12DescriptorHeap* ppHeaps[] = { textureUAV->GetTextureDescriptorHeap()->pDescriptorHeap.Get() };
//	commandList->SetDescriptorHeaps(1, ppHeaps);
//	commandList->SetComputeRootDescriptorTable(1, textureUAV->GetGPUDescriptorHandle());
//}
//
void ComputeDispatch::SetSRV(ID3D12Resource* textureSRV, int index)
{
	DirectX::CreateShaderResourceView(device, textureSRV, srvHeap.hCPU(index), false);

	//ID3D12DescriptorHeap* ppHeaps[] = { textureSRV->GetTextureDescriptorHeap()->pDescriptorHeap.Get() };
	//commandList->SetDescriptorHeaps(1, ppHeaps);
	//commandList->SetComputeRootDescriptorTable(0, textureSRV->GetGPUDescriptorHandle());
}

void ComputeDispatch::Dispatch(ID3D12GraphicsCommandList* commandList, int x, int y, int z)
{
	commandList->Dispatch(x, y, z);
}