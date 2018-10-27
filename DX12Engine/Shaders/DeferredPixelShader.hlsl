//#include "Lighting.hlsli"

Texture2D t1 : register(t0);
Texture2D normalTexture : register(t1);
Texture2D roughnessTexture : register(t2);
Texture2D metalTexture	: register(t3);
SamplerState s1 : register(s0);

struct VS_OUTPUT
{
	float4 pos : SV_POSITION;
	float2 texCoord : TEXCOORD;
	float3 normal : NORMAL;
	float3 worldPos : POSITION;
	float3 tangent : TANGENT;
};

float3 NormalMapping1(float2 uv, float3 normal, float3 tangent)
{
	float3 normalFromTexture = normalTexture.Sample(s1, uv).xyz;
	float3 unpackedNormal = normalFromTexture * 2.0f - 1.0f;
	float3 N = normal;
	float3 T = normalize(tangent - N * dot(tangent, N));
	float3 B = cross(N, T);
	float3x3 TBN = float3x3(T, B, N);
	return normalize(mul(unpackedNormal, TBN));
}

struct PS_Output
{
	float4 albedo: SV_TARGET0;
	float4 normal: SV_TARGET1;
	float4 worldPos: SV_TARGET2;
	float4 roughness: SV_TARGET3;
	float4 metalness: SV_TARGET4;
};

PS_Output main(VS_OUTPUT input) : SV_TARGET
{
	PS_Output output;
	input.normal = normalize(input.normal);
	float3 normal = NormalMapping1(input.texCoord, input.normal, input.tangent);
	output.albedo = t1.Sample(s1, input.texCoord);
	output.normal = float4(normalize(normal), 1.0f);
	output.worldPos = float4(input.worldPos, 0.0f);

	//PBR values
	float roughness = roughnessTexture.Sample(s1, input.texCoord).r;
	float metal = metalTexture.Sample(s1, input.texCoord).r;
	output.roughness = float4(roughness, 0, 0, 0);
	output.metalness = float4(metal, 0.f, 0.f, 0);

	return output;
}
