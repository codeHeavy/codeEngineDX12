#pragma once
#include "../stdafx.h"

class ShaderManager
{
public:
	
	static D3D12_SHADER_BYTECODE CompileVSShader(std::wstring VertexShaderHLSL);
	static D3D12_SHADER_BYTECODE CompilePSShader(std::wstring PixelShaderHLSL);
};