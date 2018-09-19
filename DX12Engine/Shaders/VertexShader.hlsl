// Simple vertex shader

struct VS_INPUT
{
	float4 pos : POSITION;
	float2 texCoord : TEXCOORD;
	float3 normal : NORMAL;
};

struct VS_OUTPUT
{
	float4 pos : SV_POSITION;
	float2 texCoord : TEXCOORD;
	float3 normal : NORMAL;
};

cbuffer ConstantBuffer : register(b0)
{
	float4x4 worldViewProjectionMatrix;
}


VS_OUTPUT main( VS_INPUT input )
{
	VS_OUTPUT output;
	output.normal = input.normal;
	output.pos = mul(input.pos, worldViewProjectionMatrix);
	output.texCoord = input.texCoord;

	return output;
}