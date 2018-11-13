
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
float3 FresnelSchlickRoughness(float3 v, float3 h, float f0, float roughness)
{
	float VdotH = saturate(dot(v, h));
	float r1 = 1.0f - roughness;
	return f0 + (max(float3(r1, r1, r1), f0) - f0) * pow(1 - VdotH, 5.0f);
}
float3 MicrofacetBRDF(float3 n, float3 l, float3 v, float roughness, float metalness, float3 specColor, out float3 kS)
{
	float3 h = normalize(v + l);

	float D = SpecDistribution(n, h, roughness);
	float3 F = Fresnel(v, h, specColor); // This ranges from F0_NON_METAL to actual specColor based on metalness
	float G = GeometricShadowing(n, v, h, roughness) * GeometricShadowing(n, l, h, roughness);
	kS = F;
	return (D * F * G) / (4 * max(dot(n, v), dot(n, l)));
}

float3 DiffuseEnergyConserve(float diffuse, float3 specular, float metalness)
{
	return diffuse * ((1 - saturate(specular)) * (1 - metalness));
}

float3 AmbientPBR(float3 kD, float metalness, float3 diffuse, float ao)
{
	kD *= (1.0f - metalness);
	return (kD * diffuse) * ao;
}

float3 DirLightPBR(DirectionalLight light, float3 normal, float3 worldPos, float3 camPos, float roughness, float metalness, float3 surfaceColor, float3 specularColor, float3 irradiance)
{
	float3 toLight = normalize(-light.Direction);
	float3 toCam = normalize(camPos - worldPos);

	float diff = DiffusePBR(normal, toLight);
	float3 kS = float3(0.f, 0.f, 0.f);
	float3 spec = MicrofacetBRDF(normal, toLight, toCam, roughness, metalness, specularColor, kS);

	float3 balancedDiff = DiffuseEnergyConserve(diff, spec, metalness);

	float ao = 1.0f;
	float3 kD = float3(1.0f, 1.0f, 1.0f) - kS;
	float3 diffuse = irradiance * surfaceColor;
	float3 ambient = AmbientPBR(kD, metalness, diffuse, ao);
	return (balancedDiff * surfaceColor + spec) * 1 * light.DiffuseColor.rgb + ambient;

	return (balancedDiff * surfaceColor + spec) * light.DiffuseColor.rgb;//* irradiance;
}

struct VertexToPixel
{
	float4 position		: SV_POSITION;
	float2 uv           : TEXCOORD0;
};

Texture2D gAlbedoTexture		: register(t0);
Texture2D gNormalTexture		: register(t1);
Texture2D gWorldPosTexture		: register(t2);
Texture2D gRoughnessTexture		: register(t3);
Texture2D gMetalnessTexture		: register(t4);
Texture2D gLightShapePass		: register(t5);
Texture2D gDepth				: register(t6);
TextureCube irradianceTexture	: register(t7);

SamplerState s1 : register(s0);

float4 main(VertexToPixel input) : SV_TARGET
{
	int3 sampleIndices = int3(input.position.xy, 0);

	float3 normal = gNormalTexture.Sample(s1, input.uv).xyz;
	float3 pos = gWorldPosTexture.Sample(s1, input.uv).xyz;
	float3 albedo = gAlbedoTexture.Sample(s1, input.uv).xyz;
	float3 shapeLight = gLightShapePass.Sample(s1, input.uv).xyz;
	
	// PBR RTV
	float roughness = gRoughnessTexture.Sample(s1, input.uv).r;
	float metal = gMetalnessTexture.Sample(s1, input.uv).r;

	float3 irradiance = irradianceTexture.Sample(s1, normal).rgb;
	//float3 dirToLight = normalize(dirLight.Direction);
	//float NdotL = dot(normal, -dirToLight);
	//NdotL = saturate(NdotL);
	float3 specColor = lerp(F0_NON_METAL.rrr, albedo.rgb, metal);
	float4 lightColor = float4(DirLightPBR(dirLight, normal, pos, CamPos, roughness, metal, albedo.rgb, specColor,irradiance),1);//dirLight.DiffuseColor * NdotL + dirLight.AmbientLight;
	lightColor = lightColor / (lightColor + float4(1.f, 1.f, 1.f,1.f));
	float3 finalColor = (lightColor  + shapeLight);
	float3 gammaCorrect = pow(finalColor, 1.0 / 2.2);
	return float4((finalColor),1.0f);

}