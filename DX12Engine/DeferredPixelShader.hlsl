
Texture2D t1 : register(t0);
SamplerState s1 : register(s0);

struct VS_OUTPUT
{
	float4 pos : SV_POSITION;
	float2 texCoord : TEXCOORD;
	float3 normal : NORMAL;
	float3 worldPos : POSITION;
};

struct DirectionalLight
{
	float4 AmbientLight;
	float4 DiffuseColor;
	float3 Direction;
};

struct PS_Output
{
	float3 albedo: SV_TARGET0;
	float4 normal: SV_TARGET1;
	float4 worldPos: SV_TARGET2;
};

cbuffer lightData : register(b0)
{
	DirectionalLight light;
}

PS_Output main(VS_OUTPUT input) : SV_TARGET
{
	PS_Output output;
	input.normal = normalize(input.normal);
	
	output.albedo = t1.Sample(s1, input.texCoord);
	output.normal = float4(normalize(input.normal), 1.0f);
	output.worldPos = float4(input.worldPos, 0.0f);
	
	return output;
}
