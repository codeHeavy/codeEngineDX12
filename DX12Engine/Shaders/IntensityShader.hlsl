
float pixelWidth = 1.0/1024;
float pixelHeight = 1.0/720;
int blurAmount = 5;

cbuffer constants : register(b0)
{
	int intensity;
}

Texture2D image;
RWTexture2D<float4> output;

[numthreads(16, 16, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	// Return the average
	output[DTid.xy] = image[DTid.xy] * intensity;
}