#pragma once
#include "../stdafx.h"
#include "ShaderManager.h"
class ComputeDispatch
{
private:
	ID3D12Device1* device;
	ID3D12RootSignature* rootSignature;
public:
	ComputeDispatch();
	~ComputeDispatch();
	void CreateRootSignature();
	void CreatePipelineStateObject();
};

