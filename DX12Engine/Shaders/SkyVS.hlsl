struct VertexInput
{
	float4 pos		: POSITION;
	float2 uv		: TEXCOORD;
	float3 normal	: NORMAL;
	float3 tangent	: TANGENT;
};

struct VertexToPixel
{
	float4 position		: SV_POSITION;
	float3 uv			: TEXCOORD;
};

cbuffer ConstantBuffer : register(b0)
{
	float4x4 viewProjectionMatrix;
	float4x4 worldMatrix;
};

VertexToPixel main(VertexInput input)
{
	// Set up output
	VertexToPixel output;

	// Make a view matrix with NO translation
	//matrix viewNoMovement = view;
	//viewNoMovement._41 = 0;
	//viewNoMovement._42 = 0;
	//viewNoMovement._43 = 0;

	// Calculate output position
	//matrix viewProj = mul(viewNoMovement, projection);
	output.position = mul(input.pos, viewProjectionMatrix);

	// Ensure our polygons are at max depth
	output.position.z = output.position.w;

	// Use the cube's vertex position as a direction in space
	// from the origin (center of the cube)
	output.uv = input.pos;

	return output;
}