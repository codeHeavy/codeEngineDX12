// simple pixel shader
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

cbuffer lightData : register(b0)
{
	DirectionalLight light;
}

float4 main(VS_OUTPUT input) : SV_TARGET
{
	input.normal = normalize(input.normal);
	// Diffuse Light (directional)
	float3 dirToLight = -normalize(light.Direction);
	float NdotL = dot(input.normal, dirToLight);		// calculate N dot L which gives the light amount
	NdotL = saturate(NdotL);							// Clamp between 0 and 1
	float4 lightValue = light.DiffuseColor * NdotL + light.AmbientLight;
	return lightValue * t1.Sample(s1, input.texCoord);
}