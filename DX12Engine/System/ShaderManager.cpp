#include "ShaderManager.h"
D3D12_SHADER_BYTECODE ShaderManager::CompileVSShader(std::wstring shaderFileHLSL)
{
	std::wstring root = L"Shaders/";
	root.append(shaderFileHLSL);

	HRESULT hr;
	ID3DBlob* vertexShader;		//	vertex shader bytecode
	ID3DBlob * errorBuffer;		// buffer to hold error data
	hr = D3DCompileFromFile(root.c_str(), nullptr, nullptr, "main", "vs_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0, &vertexShader, &errorBuffer);
	if (FAILED(hr))
	{
		return D3D12_SHADER_BYTECODE();
	}

	// Shader bytecode structure
	D3D12_SHADER_BYTECODE vertexShaderBytecode = {};
	vertexShaderBytecode.BytecodeLength = vertexShader->GetBufferSize();
	vertexShaderBytecode.pShaderBytecode = vertexShader->GetBufferPointer();
	return vertexShaderBytecode;
}
D3D12_SHADER_BYTECODE ShaderManager::CompilePSShader(std::wstring shaderFileHLSL)
{
	std::wstring root = L"Shaders/";
	root.append(shaderFileHLSL);

	HRESULT hr;
	// Compile pixel shader
	ID3DBlob* pixelShader;
	ID3DBlob * errorBuffer;		// buffer to hold error data
	hr = D3DCompileFromFile(root.c_str(), nullptr, nullptr, "main", "ps_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &pixelShader, &errorBuffer);
	if (FAILED(hr))
	{
		return D3D12_SHADER_BYTECODE();
	}

	// pixel shader bytecode
	D3D12_SHADER_BYTECODE pixelShaderBytecode = {};
	pixelShaderBytecode.BytecodeLength = pixelShader->GetBufferSize();
	pixelShaderBytecode.pShaderBytecode = pixelShader->GetBufferPointer();
	return pixelShaderBytecode;
}

D3D12_SHADER_BYTECODE ShaderManager::LoadShader(std::wstring shaderCsoFile)
{
	std::wstring root = L"../x64/Debug/";
	root.append(shaderCsoFile);

	ID3DBlob* shaderBlob;
	D3DReadFileToBlob(root.c_str(), &shaderBlob);
	D3D12_SHADER_BYTECODE shaderBytecode = {};
	shaderBytecode.BytecodeLength = shaderBlob->GetBufferSize();
	shaderBytecode.pShaderBytecode = shaderBlob->GetBufferPointer();
	return shaderBytecode;
}