
struct VertexToPixel
{
	float4 position		: SV_POSITION;
	float2 uv           : TEXCOORD0;
};

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

cbuffer externalData : register(b0)
{
	DirectionalLight dirLight;
	PointLight pointLight;
}

Texture2D gAlbedoTexture : register(t0);
Texture2D gNormalTexture : register(t1);
Texture2D gWorldPosTexture : register(t2);

SamplerState s1 : register(s0);

float4 main(VertexToPixel input) : SV_TARGET
{
	float3 albedo = gAlbedoTexture.Sample(s1, input.uv).rgb;
	float3 normal = gNormalTexture.Sample(s1, input.uv).rgb;
	float3 pos = gWorldPosTexture.Sample(s1, input.uv).rgb;

	float3 dirToLight = normalize(pointLight.Position - pos);
	float distance = length(pos - pointLight.Position);
	float pointNdotL = dot(normal, dirToLight);
	pointNdotL = saturate(pointNdotL);

	float3 finalColor = pointLight.Color * pointNdotL;
	return float4(finalColor,1.0f);

}