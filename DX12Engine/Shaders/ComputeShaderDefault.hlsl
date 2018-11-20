
Texture2D image;
RWTexture2D<float4> output;
[numthreads(1, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	output[DTid.xy] = image[DTid.xy];
}