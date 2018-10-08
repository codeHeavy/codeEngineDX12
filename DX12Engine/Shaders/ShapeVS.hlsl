
struct VS_INPUT
{
	float4 pos : POSITION;
	float2 texCoord : TEXCOORD;
	float3 normal : NORMAL;
};

struct VertexToPixel
{
	float4 position		: SV_POSITION;
	float2 uv           : TEXCOORD0;
};

cbuffer ConstantBuffer : register(b0)
{
	float4x4 worldViewProjectionMatrix;
	float4x4 worldMatrix;
}

VertexToPixel main(VS_INPUT input)
{
	VertexToPixel output;
	output.position = mul(input.pos, worldViewProjectionMatrix);
	output.uv = input.texCoord;
	return output;
}