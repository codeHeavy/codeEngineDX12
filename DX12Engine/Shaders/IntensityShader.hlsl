
cbuffer constants : register(b0)
{
	int intensity;
}

Texture2D image;
RWTexture2D<float4> output;

[numthreads(16, 16, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	output[DTid.xy] = image[DTid.xy] * intensity;
}