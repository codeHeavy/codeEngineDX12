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
	float3 CamPos;
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


// cosTheta = dot(N,V) , n = normal & v = dir to cam
float3 FresnelSchlickRoughness(float cosTheta, float3 F0, float roughness)
{
	float invRough = 1.0f - roughness;
	return F0 + (max(float3(invRough, invRough, invRough), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

float3 MicrofacetBRDF(float3 n, float3 l, float3 v, float roughness, float metalness, float3 specColor, float3 F0)
{
	float3 h = normalize(v + l);

	float D = SpecDistribution(n, h, roughness);
	float3 F = Fresnel(v, h, specColor); // This ranges from F0_NON_METAL to actual specColor based on metalness
	F = FresnelSchlickRoughness(max(dot(n, v), 0.0), F0, roughness);
	float G = GeometricShadowing(n, v, h, roughness) * GeometricShadowing(n, l, h, roughness);
	return (D * F * G) / (4 * max(dot(n, v), dot(n, l)));
}

float3 DiffuseEnergyConserve(float diffuse, float3 specular, float metalness)
{
	return diffuse * ((1 - saturate(specular)) * (1 - metalness));
}

float3 PointLightPBR(	PointLight light, float3 normal, float3 worldPos, float3 camPos,
						float roughness, float metalness, float3 surfaceColor, float3 specularColor,
						float3 irradiance, float3 prefilteredColor, float2 brdf)
{
	// calculate reflectance at normal incidence; if dia-electric (like plastic) use F0 
	// of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)    
	float3 F0 = float3(0.04, 0.04, 0.04);
	F0 = lerp(F0, surfaceColor, metalness);

	float3 toLight = normalize(light.Position - worldPos);
	float3 toCam = normalize(camPos - worldPos);

	float atten = Attenuate(light.Position, light.Range, worldPos);
	float diff = DiffusePBR(normal, toLight);
	float3 spec = MicrofacetBRDF(normal, toLight, toCam, roughness, metalness, specularColor, F0);
	float3 balancedDiff = DiffuseEnergyConserve(diff, spec, metalness);

	float3 kS = FresnelSchlickRoughness(max(dot(normal, toCam), 0.0), F0, roughness);
	float3 kD = float3(1.0f, 1.0f, 1.0f) - kS;
	kD *= (1.0f - metalness);

	// Light calc
	//float3 lightVal = (surfaceColor * kD / PI + spec) * balancedDiff * light.Color.rgb * atten;
	float3 lightVal = (balancedDiff * surfaceColor + spec) * light.Color.rgb * atten;
	
	float3 diffuse = irradiance * surfaceColor;
	float3 specular = prefilteredColor * (kS * brdf.x + brdf.y);
	//float ao = 1.0f;
	float3 ambient = (kD * diffuse + specular); // multiply by ao = 1
	float3 lightColor = ambient + lightVal;
	// Tone mapping
	lightColor = lightColor / (lightColor + float4(1.f, 1.f, 1.f, 1.f));
	return lightColor;
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


Texture2D gAlbedoTexture		: register(t0);
Texture2D gNormalTexture		: register(t1);
Texture2D gWorldPosTexture		: register(t2);
Texture2D gRoughnessTexture		: register(t3);
Texture2D gMetalnessTexture		: register(t4);
Texture2D gLightShapePass		: register(t5);
// t6 = quad texture
Texture2D gDepth				: register(t7);
TextureCube irradianceTexture	: register(t8);
Texture2D brdfTexture			: register(t9);
TextureCube prefilterTexture	: register(t10);
SamplerState s1 : register(s0);

float3 PrefilteredColor(float3 viewDir, float3 normal, float roughness, TextureCube prefilter)
{
	const float MAX_REF_LOD = 4.0f;
	float3 R = reflect(-viewDir, normal);
	float3 prefilteredColor = prefilter.SampleLevel(s1, R, roughness * MAX_REF_LOD).rgb;
	return prefilteredColor;
}
float2 EnvBrdf(float3 normal, float3 viewDir, float roughness, Texture2D brdftex)
{
	float2 uv = float2(max(dot(normal, viewDir), 0.0f), roughness);
	return brdftex.Sample(s1, uv).rg;
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

	float3 irradiance = irradianceTexture.Sample(s1, normal).rgb;
	float3 viewDir = normalize(CamPos.xyz - pos);
	float3 prefilter = PrefilteredColor(viewDir, normal, roughness, prefilterTexture);
	float2 brdf = EnvBrdf(normal, viewDir, roughness, brdfTexture);
	//float3 dirToLight	= normalize(pos - pointLight.Position);
	//float pointNdotL	= dot(normal, -dirToLight);
	//pointNdotL			= saturate(pointNdotL);
	float3 specColor	= lerp(F0_NON_METAL.rrr, albedo.rgb, metal);
	float4 finalColor	= float4(PointLightPBR(pointLight, normal, pos, CamPos, roughness, metal, albedo.rgb, specColor, irradiance, prefilter, brdf),1);
	//float4 finalColor = calculatePointLight(normal,pos,pointLight);// pointLight.Color * pointNdotL;
	float3 gammaCorrect = pow(finalColor, 1.0 / 2.2);
	
	return (finalColor );// *float4(albedo, 1.0));

}