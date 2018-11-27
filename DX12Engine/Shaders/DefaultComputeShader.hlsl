
Texture2D image;
RWTexture2D<float4> output;

[numthreads(16, 16, 1)]
void main(int3 groupThreadID : SV_GroupThreadID, uint3 DTid : SV_DispatchThreadID )
{
	output[DTid.xy] = float4(1, 0, 0, 0.3) * image[DTid.xy];
}