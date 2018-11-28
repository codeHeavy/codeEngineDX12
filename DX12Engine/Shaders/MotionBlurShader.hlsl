
cbuffer constants : register(b0)
{
	float4x4 blurAmount;
}

Texture2D image;
Texture2D worldPos;
RWTexture2D<float4> output;
SamplerState s : register(s0);
[numthreads(16, 16, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	float4 a = image[DTid.xy];
	float4 b = worldPos[DTid.xy];

	// Current viewport position
	float4 currentPos = H;
	// Use the world position, and transform by the previous view-
	// projection matrix.
	float4 previousPos = mul(worldPos, prevViewProj);
	// Convert to nonhomogeneous points [-1,1] by dividing by w.
	previousPos /= previousPos.w;
	// Use this frame's position and last frame's to compute the pixel
	// velocity.
	float2 velocity = (currentPos - previousPos) / 2.f;

	output[DTid.xy] = b;
}