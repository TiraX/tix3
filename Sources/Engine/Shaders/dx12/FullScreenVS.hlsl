// A constant buffer that stores the three basic column-major matrices for composing geometry.
#include "FullScreenRS.hlsli"

cbuffer ModelViewProjectionConstantBuffer : register(b0)
{
	matrix ViewProjection;
	float3 ViewDir;
	float3 ViewPos;
};

// Per-vertex data used as input to the vertex shader.
struct VertexShaderInput
{
	float3 pos : POSITION;
	float2 uv : TEXCOORD;
};

// Per-pixel color data passed through the pixel shader.
struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD;
};

// Simple shader to do vertex processing on the GPU.
[RootSignature(FullScreen_RootSig)]
PixelShaderInput main(VertexShaderInput input)
{
	PixelShaderInput output;
	float4 pos = float4(input.pos, 1.0f);

	// vertex input position already in projected space.
	output.pos = pos;

	output.uv = input.uv;

	return output;
}