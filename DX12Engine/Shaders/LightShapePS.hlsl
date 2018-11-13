#define POINT_INTENSITY 0.5
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
	float Range;
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
Texture2D gRoughnessTexture : register(t3);
Texture2D gMetalnessTexture : register(t4);

SamplerState s1 : register(s0);


float Attenuate(float3 position, float range, float3 worldPos)
{
	float dist = distance(position, worldPos);

	float att = saturate(1.0f - (dist * dist / (range * range)));

	return att * att;
}

static const float F0_NON_METAL = 0.04f;

static const float MIN_ROUGHNESS = 0.0000001f;

static const float PI = 3.14159265359f;

float DiffusePBR(float3 normal, float3 dirToLight)
{
	return saturate(dot(normal, dirToLight));
}

float SpecDistribution(float3 n, float3 h, float roughness)
{
	float NdotH = saturate(dot(n, h));
	float NdotH2 = NdotH * NdotH;
	float a = roughness * roughness;
	float a2 = max(a * a, MIN_ROUGHNESS); 

	float denomToSquare = NdotH2 * (a2 - 1) + 1;

	return a2 / (PI * denomToSquare * denomToSquare);
}


float3 Fresnel(float3 v, float3 h, float3 f0)
{
	float VdotH = saturate(dot(v, h));
	return f0 + (1 - f0) * pow(1 - VdotH, 5);
}

float GeometricShadowing(float3 n, float3 v, float3 h, float roughness)
{
	float k = pow(roughness + 1, 2) / 8.0f;
	float NdotV = saturate(dot(n, v));

	return NdotV / (NdotV * (1 - k) + k);
}


float3 MicrofacetBRDF(float3 n, float3 l, float3 v, float roughness, float metalness, float3 specColor)
{
	float3 h = normalize(v + l);

	float D = SpecDistribution(n, h, roughness);
	float3 F = Fresnel(v, h, specColor); 
	float G = GeometricShadowing(n, v, h, roughness) * GeometricShadowing(n, l, h, roughness);

	return (D * F * G) / (4 * max(dot(n, v), dot(n, l)));
}

float3 DiffuseEnergyConserve(float diffuse, float3 specular, float metalness)
{
	return diffuse * ((1 - saturate(specular)) * (1 - metalness));
}

float3 PointLightPBR(PointLight light, float3 normal, float3 worldPos, float3 camPos, float roughness, float metalness, float3 surfaceColor, float3 specularColor)
{
	float3 toLight = normalize(light.Position - worldPos);
	float3 toCam = normalize(camPos - worldPos);

	float atten = Attenuate(light.Position, light.Range, worldPos);
	float diff = DiffusePBR(normal, toLight);
	float3 spec = MicrofacetBRDF(normal, toLight, toCam, roughness, metalness, specularColor);

	float3 balancedDiff = DiffuseEnergyConserve(diff, spec, metalness);

	return (balancedDiff * surfaceColor + spec)  * atten *  light.Color;
}
float4 calculatePointLight(float3 normal, float3 worldPos, PointLight light)
{
	float3 dirToPointLight = normalize(light.Position - worldPos);
	float distance = length(worldPos - light.Position);
	float pointNdotL = dot(normal, dirToPointLight);
	pointNdotL = saturate(pointNdotL) * POINT_INTENSITY;
	float attenuation = Attenuate(light.Position, light.Range, worldPos);
	return (light.Color * pointNdotL);
}

float4 main(VertexToPixel input) : SV_TARGET
{
	int3 sampleIndices = int3(input.position.xy, 0);

	float3 normal	= gNormalTexture.Load(sampleIndices).rgb;
	float3 pos		= gWorldPosTexture.Load(sampleIndices).rgb;
	float3 albedo	= gAlbedoTexture.Load(sampleIndices).rgb;
	// PBR RTV
	float roughness = gRoughnessTexture.Load(sampleIndices).r;
	float metal		= gMetalnessTexture.Load(sampleIndices).r;
	normal = normalize(normal);
	
	//float3 dirToLight	= normalize(pos - pointLight.Position);
	//float pointNdotL	= dot(normal, -dirToLight);
	//pointNdotL			= saturate(pointNdotL);
	float3 specColor	= lerp(F0_NON_METAL.rrr, albedo.rgb, metal);
	float4 finalColor	= float4(PointLightPBR(pointLight, normal, pos, CamPos, roughness, metal, albedo.rgb, specColor),1);
	//float4 finalColor = calculatePointLight(normal,pos,pointLight);// pointLight.Color * pointNdotL;
	float3 gammaCorrect = pow(finalColor, 1.0 / 2.2);
	return (finalColor );// *float4(albedo, 1.0));

}