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

	// Ranged-based attenuation
	float att = saturate(1.0f - (dist * dist / (range * range)));

	// Soft falloff
	return att * att;
}
// === PHYSICALLY BASED LIGHTING ====================================

// PBR Constants:

// The fresnel value for non-metals (dielectrics)
// Page 9: "F0 of nonmetals is now a constant 0.04"
// http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf
// Also slide 65 of http://blog.selfshadow.com/publications/s2014-shading-course/hoffman/s2014_pbs_physics_math_slides.pdf
static const float F0_NON_METAL = 0.04f;

// Need a minimum roughness for when spec distribution function denominator goes to zero
static const float MIN_ROUGHNESS = 0.0000001f; // 6 zeros after decimal

static const float PI = 3.14159265359f;

// Lambert diffuse BRDF - Same as the basic lighting!
float DiffusePBR(float3 normal, float3 dirToLight)
{
	return saturate(dot(normal, dirToLight));
}



// GGX (Trowbridge-Reitz)
//
// a - Roughness
// h - Half vector
// n - Normal
// 
// D(h, n) = a^2 / pi * ((n dot h)^2 * (a^2 - 1) + 1)^2
float SpecDistribution(float3 n, float3 h, float roughness)
{
	// Pre-calculations
	float NdotH = saturate(dot(n, h));
	float NdotH2 = NdotH * NdotH;
	float a = roughness * roughness;
	float a2 = max(a * a, MIN_ROUGHNESS); // Applied after remap!

										  // ((n dot h)^2 * (a^2 - 1) + 1)
	float denomToSquare = NdotH2 * (a2 - 1) + 1;
	// Can go to zero if roughness is 0 and NdotH is 1

	// Final value
	return a2 / (PI * denomToSquare * denomToSquare);
}



// Fresnel term - Schlick approx.
// 
// v - View vector
// h - Half vector
// f0 - Value when l = n (full specular color)
//
// F(v,h,f0) = f0 + (1-f0)(1 - (v dot h))^5
float3 Fresnel(float3 v, float3 h, float3 f0)
{
	// Pre-calculations
	float VdotH = saturate(dot(v, h));

	// Final value
	return f0 + (1 - f0) * pow(1 - VdotH, 5);
}



// Geometric Shadowing - Schlick-GGX (based on Schlick-Beckmann)
// - k is remapped to a / 2, roughness remapped to (r+1)/2
//
// n - Normal
// v - View vector
//
// G(l,v,h)
float GeometricShadowing(float3 n, float3 v, float3 h, float roughness)
{
	// End result of remapping:
	float k = pow(roughness + 1, 2) / 8.0f;
	float NdotV = saturate(dot(n, v));

	// Final value
	return NdotV / (NdotV * (1 - k) + k);
}



// Microfacet BRDF (Specular)
//
// f(l,v) = D(h)F(v,h)G(l,v,h) / 4(n dot l)(n dot v)
// - part of the denominator are canceled out by numerator (see below)
//
// D() - Spec Dist - Trowbridge-Reitz (GGX)
// F() - Fresnel - Schlick approx
// G() - Geometric Shadowing - Schlick-GGX
float3 MicrofacetBRDF(float3 n, float3 l, float3 v, float roughness, float metalness, float3 specColor)
{
	// Other vectors
	float3 h = normalize(v + l);

	// Grab various functions
	float D = SpecDistribution(n, h, roughness);
	float3 F = Fresnel(v, h, specColor); // This ranges from F0_NON_METAL to actual specColor based on metalness
	float G = GeometricShadowing(n, v, h, roughness) * GeometricShadowing(n, l, h, roughness);

	// Final formula
	// Denominator dot products partially canceled by G()!
	// See page 16: http://blog.selfshadow.com/publications/s2012-shading-course/hoffman/s2012_pbs_physics_math_notes.pdf
	return (D * F * G) / (4 * max(dot(n, v), dot(n, l)));
}

// Calculates diffuse color based on energy conservation
//
// diffuse - Diffuse color
// specular - Specular color (including light color)
// metalness - surface metalness amount
//
// Metals should have an albedo of (0,0,0)...mostly
// See slide 65: http://blog.selfshadow.com/publications/s2014-shading-course/hoffman/s2014_pbs_physics_math_slides.pdf
float3 DiffuseEnergyConserve(float diffuse, float3 specular, float metalness)
{
	return diffuse * ((1 - saturate(specular)) * (1 - metalness));
}

float3 PointLightPBR(PointLight light, float3 normal, float3 worldPos, float3 camPos, float roughness, float metalness, float3 surfaceColor, float3 specularColor)
{
	// Calc light direction
	float3 toLight = normalize(light.Position - worldPos);
	float3 toCam = normalize(camPos - worldPos);

	// Calculate the light amounts
	float atten = Attenuate(light.Position, light.Range, worldPos);
	float diff = DiffusePBR(normal, toLight);
	float3 spec = MicrofacetBRDF(normal, toLight, toCam, roughness, metalness, specularColor);

	// Calculate diffuse with energy conservation
	// (Reflected light doesn't diffuse)
	float3 balancedDiff = DiffuseEnergyConserve(diff, spec, metalness);

	// Combine
	return (balancedDiff * surfaceColor + spec)  * atten*  1 /* * light.Intensity */ * light.Color;
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