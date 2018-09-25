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
	float3 worldPos : POSITION;
};

cbuffer ConstantBuffer : register(b0)
{
	float4x4 worldViewProjectionMatrix;
	float4x4 worldMatrix;
}


VS_OUTPUT main( VS_INPUT input )
{
	VS_OUTPUT output;
	output.normal = input.normal;
	output.pos = mul(input.pos, worldViewProjectionMatrix);
	output.texCoord = input.texCoord;
	output.worldPos = mul(input.pos, worldMatrix).xyz;
	return output;
}