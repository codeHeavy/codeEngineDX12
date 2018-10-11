
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
	float padding;
};

struct PointLight
{
	float4 Color;
	float3 Position;
	float padding;
};

cbuffer externalData : register(b0)
{
	DirectionalLight dirLight;
	PointLight pointLight;
	float4 CamPos;
}

Texture2D gAlbedoTexture : register(t0);
Texture2D gNormalTexture : register(t1);
Texture2D gWorldPosTexture : register(t2);

SamplerState s1 : register(s0);

float4 main(VertexToPixel input) : SV_TARGET
{
	int3 sampleIndices = int3(input.position.xy, 0);

	float3 normal	= gNormalTexture.Load(sampleIndices).xyz  ;
	float3 pos		= gWorldPosTexture.Load(sampleIndices).xyz;
	float3 albedo	= gAlbedoTexture.Load(sampleIndices).xyz  ;
	normal = normalize(normal);
	
	float3 dirToLight	= normalize(pos - pointLight.Position);
	float pointNdotL	= dot(normal, -dirToLight);
	pointNdotL			= saturate(pointNdotL);

	float4 finalColor = pointLight.Color * pointNdotL;
	return (finalColor * float4(albedo,1.0));

}