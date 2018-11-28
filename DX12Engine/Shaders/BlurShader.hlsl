
cbuffer constants : register(b0)
{
	int blurAmount;
}

Texture2D image;
RWTexture2D<float4> output;
SamplerState s : register(s0);
[numthreads(16, 16, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	float2 uv = float2(0, 0);
	uv.x = DTid.x / 1024.f;
	uv.y = DTid.y / 720.f;

	float pixelWidth = 1.0 / 1024;
	float pixelHeight = 1.0 / 720;
	// Keep track of the total color
	float4 totalColor = float4(0, 0, 0, 0);
	uint numSamples = 0;

	for (int x = -blurAmount; x <= blurAmount; x++)
	{
		for (int y = -blurAmount; y <= blurAmount; y++)
		{
			float2 nuv = uv + float2(x * pixelWidth, y * pixelHeight);
			totalColor += image.SampleLevel(s, nuv, 0);

			numSamples++;
		}
	}

	output[DTid.xy] = totalColor / numSamples;

}