
cbuffer constants : register(b0)
{
	int tint;
}

Texture2D image;
RWTexture2D<float4> output;
SamplerState s : register(s0);
[numthreads(16, 16, 1)]
void main(int3 groupThreadID : SV_GroupThreadID, uint3 DTid : SV_DispatchThreadID )
{
	int value = tint - 1;
	value += 0.10;
	if (value == 0)
		value = 1;
	output[DTid.xy] = float4(value, 0, 0, 0.3) * image[DTid.xy];
}