
Texture2D image;
RWTexture2D<float4> output;

[numthreads(16, 16, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	float4 pixel = image[DTid.xy];
	pixel.rgb = pixel.r * 0.3 + pixel.g * 0.59 + pixel.b *0.11;
	output[DTid.xy] = pixel;
}