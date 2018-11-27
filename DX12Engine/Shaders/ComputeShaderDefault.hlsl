
Texture2D image;
RWTexture2D<float4> output;
[numthreads(16, 16, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	float i = image[DTid.xy];
	output[DTid.xy] = image[DTid.xy];
}