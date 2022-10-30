
#ifndef RWCoherentBuffer
#define RWCoherentBuffer(TYPE) globallycoherent RWBuffer<TYPE>
#endif

#ifndef RWCoherentStructuredBuffer
#define RWCoherentStructuredBuffer(TYPE) globallycoherent RWStructuredBuffer<TYPE>
#endif

#ifndef RWCoherentByteAddressBuffer
#define RWCoherentByteAddressBuffer globallycoherent RWByteAddressBuffer
#endif

#define PLATFORM_SUPPORTS_SM6_0_WAVE_OPERATIONS 1
#if PLATFORM_SUPPORTS_SM6_0_WAVE_OPERATIONS
#define COMPILER_SUPPORTS_WAVE_ONCE			1
#define COMPILER_SUPPORTS_WAVE_VOTE			1
#define COMPILER_SUPPORTS_WAVE_MINMAX		1
#define COMPILER_SUPPORTS_WAVE_BIT_ORAND	1
#endif

float Square(float x)
{
	return x * x;
}

float2 Square(float2 x)
{
	return x * x;
}

float3 Square(float3 x)
{
	return x * x;
}

float4 Square(float4 x)
{
	return x * x;
}

float Pow2( float x )
{
	return x*x;
}

float2 Pow2( float2 x )
{
	return x*x;
}

float3 Pow2( float3 x )
{
	return x*x;
}

float4 Pow2( float4 x )
{
	return x*x;
}

float length2(float2 v)
{
	return dot(v, v);
}
float length2(float3 v)
{
	return dot(v, v);
}
float length2(float4 v)
{
	return dot(v, v);
}

// Returns whether or not the given projection matrix is orthographic
bool IsOrthoProjection(float4x4 ViewToClip)
{
	return ViewToClip[3][3] >= 1.0f;
}