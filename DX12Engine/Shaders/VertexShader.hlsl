// Simple vertex shader
float4 main( float4 pos : POSITION ) : SV_POSITION
{
	return float4(pos,1.0f);
}