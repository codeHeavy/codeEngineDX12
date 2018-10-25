
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

cbuffer lightData : register(b0)
{
	DirectionalLight dirLight;
	PointLight	pLight;
	float4 CamPos;
}

struct VertexToPixel
{
	float4 position		: SV_POSITION;
	float2 uv           : TEXCOORD0;
};

Texture2D gAlbedoTexture : register(t0);
Texture2D gNormalTexture : register(t1);
Texture2D gWorldPosTexture : register(t2);
Texture2D gRoughnessTexture : register(t3);
Texture2D gMetalnessTexture : register(t4);

Texture2D gLightShapePass: register(t5);

SamplerState s1 : register(s0);

float4 main(VertexToPixel input) : SV_TARGET
{
	int3 sampleIndices = int3(input.position.xy, 0);

	float3 normal = gNormalTexture.Load(sampleIndices).xyz;
	float3 pos = gWorldPosTexture.Load(sampleIndices).xyz;
	float3 albedo = gAlbedoTexture.Load(sampleIndices).xyz;
	float3 shapeLight = gLightShapePass.Load(sampleIndices).xyz;
	
	// PBR RTV
	float roughness = gRoughnessTexture.Sample(s1, input.uv).r;
	float metal = gMetalnessTexture.Sample(s1, input.uv).r;


	float3 dirToLight = normalize(dirLight.Direction);
	float NdotL = dot(normal, -dirToLight);
	NdotL = saturate(NdotL);
	float4 lightColor = dirLight.DiffuseColor * NdotL + dirLight.AmbientLight;

	// Specular
	float3 dirToCam = normalize( CamPos - pos );
	float3 pLightDir = normalize( pLight.Position - pos);
	// incoming light dir
	float3 refl = reflect(-pLightDir, normal);
	float spec = pow(saturate(dot(dirToCam, refl)), 128);

	float3 finalColor = (lightColor + spec.xxxx) * albedo;
	return float4(finalColor + shapeLight,1.0f);

}