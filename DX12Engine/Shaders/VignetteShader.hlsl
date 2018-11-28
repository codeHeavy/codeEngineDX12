
#define VignetteRatio 6.00

Texture2D image;
RWTexture2D<float4> output;
SamplerState s : register(s0);
[numthreads(16, 16, 1)]
void main(int3 groupThreadID : SV_GroupThreadID, uint3 DTid : SV_DispatchThreadID)
{
	float2 uv = float2(0, 0);
	uv.x = DTid.x / 1024.f;
	uv.y = DTid.y / 720.f;

	float4 color = image[DTid.xy];
	uv = -uv * uv + uv;
	color.rgb = saturate(((image[DTid.xy].x / image[DTid.xy].y)*(image[DTid.xy].x / image[DTid.xy].y) * VignetteRatio * uv.x + uv.y) * 4.0) * color.rgb;


	output[DTid.xy] = color;

}