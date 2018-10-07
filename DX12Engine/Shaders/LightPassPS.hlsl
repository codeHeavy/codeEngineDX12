
struct DirectionalLight
{
	float4 AmbientLight;
	float4 DiffuseColor;
	float3 Direction;
};

struct PointLight
{
	float4 Color;
	float3 Position;
};

cbuffer lightData : register(b0)
{
	DirectionalLight dirLight;
	PointLight	pLight;
}

struct VertexToPixel
{
	float4 position		: SV_POSITION;
	float2 uv           : TEXCOORD0;
};

Texture2D gAlbedoTexture : register(t0);
Texture2D gNormalTexture : register(t1);
Texture2D gWorldPosTexture : register(t2);

SamplerState s1 : register(s0);

float4 main(VertexToPixel input) : SV_TARGET
{
	float3 albedo = gAlbedoTexture.Sample(s1, input.uv).rgb;
	float3 normal = gNormalTexture.Sample(s1, input.uv).rgb;
	float3 pos = gWorldPosTexture.Sample(s1, input.uv).rgb;

	float3 dirToLight = normalize(-dirLight.Direction);
	float NdotL = dot(normal, dirToLight);
	NdotL = saturate(NdotL);
	float4 lightColor = dirLight.DiffuseColor * NdotL + dirLight.AmbientLight;

	float3 finalColor = lightColor * albedo;
	return float4(finalColor,1.0f);

}