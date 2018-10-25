
Texture2D t1 : register(t0);
Texture2D normalTexture : register(t1);
SamplerState s1 : register(s0);

struct VS_OUTPUT
{
	float4 pos : SV_POSITION;
	float2 texCoord : TEXCOORD;
	float3 normal : NORMAL;
	float3 worldPos : POSITION;
	float3 tangent : TANGENT;
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

float3 calculateNormalFromMap(float2 uv, float3 normal, float3 tangent)
{
	float3 normalFromTexture = normalTexture.Sample(s1, uv).xyz;
	float3 unpackedNormal = normalFromTexture * 2.0f - 1.0f;
	float3 N = normal;
	float3 T = normalize(tangent - N * dot(tangent, N));
	float3 B = cross(N, T);
	float3x3 TBN = float3x3(T, B, N);
	return normalize(mul(unpackedNormal, TBN));
}

PS_Output main(VS_OUTPUT input) : SV_TARGET
{
	PS_Output output;
	input.normal = normalize(input.normal);
	float3 normal = calculateNormalFromMap(input.texCoord, input.normal, input.tangent);
	output.albedo = t1.Sample(s1, input.texCoord);
	output.normal = float4(normalize(normal), 1.0f);
	output.worldPos = float4(input.worldPos, 0.0f);
	
	return output;
}
