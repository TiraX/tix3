/*
	TiX Engine v3.0 Copyright (C) 2018~2021
	By ZhaoShuai tirax.cn@gmail.com
*/

// NaniteLearning Sample
#include "Common.hlsli"
#include "NaniteDefinitions.h"
#include "NaniteAttributeDecode.h"

#include "NaniteDebugInfo.h"

#define DEBUG_INFO (1)

struct FTriRange
{
	uint Start;
	uint Num;
};

FTriRange GetTriangleRange(FCluster Cluster, bool bHasRasterBin, uint3 RasterBin)
{
	FTriRange Range;
	if (bHasRasterBin)
	{
		Range.Start = RasterBin.y;
		Range.Num = RasterBin.z - RasterBin.y;
	}
	else
	{
		Range.Start = 0;
		Range.Num = Cluster.NumTris;
	}
	return Range;
}


struct VSOut
{
	float4 PointClipPixel						: TEXCOORD0;
	float3 Normal								: TEXCOORD1;
	float2 UV									: TEXCOORD2;
	float4 Position								: SV_Position;	 // Reading SV_Position in the pixel shader limits launch rate on some hardware. Interpolate manually instead.
};

RWTexture2D<UlongType>	OutVisBuffer64 : register(u0);
#if DEBUG_INFO
RWStructuredBuffer<FNaniteTessDebugInfo>	DebugInfo : register(u1);
#endif

VSOut CommonRasterizerVS(FNaniteView NaniteView, FVisibleCluster VisibleCluster, FCluster Cluster, float3 P, uint PixelValue)
{
	VSOut Out;

	const float3 PointLocal = P;

	float3 WorldPositionOffset = 0.0f;
	//#if NANITE_VERTEX_PROGRAMMABLE
	//	const FInstanceDynamicData InstanceDynamicData = CalculateInstanceDynamicData(NaniteView, InstanceData);
	//	WorldPositionOffset = EvaluateWorldPositionOffset(NaniteView, InstanceData, InstanceDynamicData, Cluster, VertIndex, PointLocal);
	//#endif

	const float4x4 LocalToTranslatedWorld = {
		1,0,0,0,
		0,1,0,0,
		0,0,1,0,
		0,0,0,1
	};
	//LWCMultiplyTranslation(InstanceData.LocalToWorld, NaniteView.PreViewTranslation);
	const float3 PointRotated = LocalToTranslatedWorld[0].xyz * PointLocal.xxx + LocalToTranslatedWorld[1].xyz * PointLocal.yyy + LocalToTranslatedWorld[2].xyz * PointLocal.zzz;
	const float3 PointTranslatedWorld = PointRotated + LocalToTranslatedWorld[3].xyz + WorldPositionOffset;
	float4 PointClip = mul(float4(PointTranslatedWorld, 1), NaniteView.TranslatedWorldToClip);
	// #if VIRTUAL_TEXTURE_TARGET
	// 	/*
	// 	float2 vUV = PointClip.xy * float2(0.5, -0.5) + 0.5 * PointClip.w;
	// 	float2 vPixels = vUV * NaniteView.ViewSizeAndInvSize.xy;
	// 	float2 LocalPixels = vPixels - VisibleCluster.vPage * VSM_PAGE_SIZE * PointClip.w;
	// 	float2 LocalUV = LocalPixels / ( 4 * VSM_PAGE_SIZE );
	// 	float2 LocalClip = LocalUV * float2(2, -2) + float2(-1, 1) * PointClip.w;
	// 	PointClip.xy = LocalClip;
	// 	*/
	// 	PointClip.xy = NaniteView.ClipSpaceScaleOffset.xy * PointClip.xy + NaniteView.ClipSpaceScaleOffset.zw * PointClip.w;

	// 	// Offset 0,0 to be at vPage for a 0, VSM_PAGE_SIZE * VSM_RASTER_WINDOW_PAGES viewport.
	// 	PointClip.xy += PointClip.w * ( float2(-2, 2) / VSM_RASTER_WINDOW_PAGES ) * VisibleCluster.vPage;

	// 	Out.ViewRect.xy = 0;	// Unused by pixel shader
	// 	Out.ViewRect.zw = VisibleCluster.vPageEnd * VSM_PAGE_SIZE + VSM_PAGE_SIZE;
	// #elif NANITE_MULTI_VIEW
	// 	PointClip.xy = NaniteView.ClipSpaceScaleOffset.xy * PointClip.xy + NaniteView.ClipSpaceScaleOffset.zw * PointClip.w;
	// 	Out.ViewRect = NaniteView.ViewRect;
	// #endif

		// Calculate PointClipPixel coordinates that bring us directly to absolute pixel positions after w divide
	float4 PointClipPixel = float4(PointClip.xy * float2(0.5f, -0.5f) + 0.5f * PointClip.w, PointClip.zw);
	// #if VIRTUAL_TEXTURE_TARGET
	// 	PointClipPixel.xy *= (VSM_RASTER_WINDOW_PAGES * VSM_PAGE_SIZE);
	// 	PointClipPixel.xy += VisibleCluster.vPage * (VSM_PAGE_SIZE * PointClipPixel.w);
	// #elif NANITE_MULTI_VIEW
	// 	PointClipPixel.xy *= HardwareViewportSize;	// Offset handled by ClipSpaceScaleOffset
	// #else
	PointClipPixel.xy *= NaniteView.ViewSizeAndInvSize.xy;
	PointClipPixel.xy += NaniteView.ViewRect.xy * PointClipPixel.w;
	//#endif
	Out.PointClipPixel = PointClipPixel;

	//Out.PixelValue_ViewId_Mip_ArrayIndex_LevelOffset = uint3(PixelValue, 0u, 0u);

	// #if VIRTUAL_TEXTURE_TARGET
	// 	const bool bStaticPage = ShouldCacheInstanceAsStatic(InstanceData, NaniteView);
	// 	const uint ArrayIndex = bStaticPage ? GetVirtualShadowMapStaticArrayIndex() : 0;
	// 	Out.PixelValue_ViewId_Mip_ArrayIndex_LevelOffset.y = VisibleCluster.ViewId | (NaniteView.TargetMipLevel << 16) | (ArrayIndex << 24);
	// 	Out.PixelValue_ViewId_Mip_ArrayIndex_LevelOffset.z = CalcPageTableLevelOffset(NaniteView.TargetLayerIndex, NaniteView.TargetMipLevel).LevelOffset;
	// #elif NANITE_MULTI_VIEW
	// 	Out.PixelValue_ViewId_Mip_ArrayIndex_LevelOffset.y = VisibleCluster.ViewId;
	// #endif
	Out.Position = PointClip;

	const bool bNearClip = ((NaniteView.Flags & NANITE_VIEW_FLAG_NEAR_CLIP) != 0u);
	if (!bNearClip)
	{

		// Shader workaround to avoid HW depth clipping. Should be replaced with rasterizer state ideally.
		Out.Position.z = 0.5f * Out.Position.w;
	}

	// #if BARYCENTRIC_MODE_INTRINSICS
	// 	Out.VertexID = VertIndex;
	// #endif

	return Out;
}

VSOut CommonRasterizerVS(FNaniteView NaniteView, FVisibleCluster VisibleCluster, FCluster Cluster, uint VertIndex, uint PixelValue)
{
	VSOut Out;

	const float3 PointLocal = DecodePosition(VertIndex, Cluster);
	return CommonRasterizerVS(NaniteView, VisibleCluster, Cluster, PointLocal, PixelValue);
}


// 3D random number generator inspired by PCGs (permuted congruential generator)
// Using a **simple** Feistel cipher in place of the usual xor shift permutation step
// @param v = 3D integer coordinate
// @return three elements w/ 16 random bits each (0-0xffff).
// ~8 ALU operations for result.x    (7 mad, 1 >>)
// ~10 ALU operations for result.xy  (8 mad, 2 >>)
// ~12 ALU operations for result.xyz (9 mad, 3 >>)
uint3 Rand3DPCG16(int3 p)
{
	// taking a signed int then reinterpreting as unsigned gives good behavior for negatives
	uint3 v = uint3(p);

	// Linear congruential step. These LCG constants are from Numerical Recipies
	// For additional #'s, PCG would do multiple LCG steps and scramble each on output
	// So v here is the RNG state
	v = v * 1664525u + 1013904223u;

	// PCG uses xorshift for the final shuffle, but it is expensive (and cheap
	// versions of xorshift have visible artifacts). Instead, use simple MAD Feistel steps
	//
	// Feistel ciphers divide the state into separate parts (usually by bits)
	// then apply a series of permutation steps one part at a time. The permutations
	// use a reversible operation (usually ^) to part being updated with the result of
	// a permutation function on the other parts and the key.
	//
	// In this case, I'm using v.x, v.y and v.z as the parts, using + instead of ^ for
	// the combination function, and just multiplying the other two parts (no key) for 
	// the permutation function.
	//
	// That gives a simple mad per round.
	v.x += v.y*v.z;
	v.y += v.z*v.x;
	v.z += v.x*v.y;
	v.x += v.y*v.z;
	v.y += v.z*v.x;
	v.z += v.x*v.y;

	// only top 16 bits are well shuffled
	return v >> 16u;
}

#define HWRasterizeRS \
    "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), " \
	"RootConstants(num32BitConstants=10, b0)," \
	"DescriptorTable(SRV(t0, numDescriptors=4), UAV(u0, numDescriptors=2)) "


struct PrimitiveAttributes
{
	// Use uint4 to prevent compiler from erroneously packing per-vertex and per-prim attributes together
	// .x = Cluster Index
	// .y = Triangle Index
	// .z = View Width
	// .w = View Height
	nointerpolation uint4 PackedData : TEXCOORD7;
};

// an AS group deal with 64 triangles max, use 2 as groups to deal with 1 cluster
#define AS_GROUP_SIZE 64
// PNTriangles control points
struct CT_PNTriangle
{
	float P[30];
};
struct TessFactor
{
	uint Factor[4];	// xyz=edge_factor; w=inside_factor;
};
struct Payload
{
    CT_PNTriangle CT[AS_GROUP_SIZE];
	TessFactor TF[AS_GROUP_SIZE];

	uint MSTable[1024];

	uint VisibleClusterIndex;
	uint TriangleOffset;
};

// There is an compile errrrror i dont know how to fix
 void CalcPNTriangleControlPoints() {}

 // TODO: Find better ways to calc tessellation.
 static const uint3 k_triangles[24] =
 {
	 uint3(3, 1, 0), uint3(3, 0, 2), uint3(3, 2, 4), uint3(3, 5, 1),
	 uint3(6, 3, 4), uint3(7, 5, 3), uint3(8, 7, 3), uint3(8, 3, 6),
	 uint3(7, 11, 5), uint3(10, 5, 11), uint3(6, 4, 13), uint3(9, 13, 4),
	 uint3(8, 11, 7), uint3(8, 6, 13), uint3(11, 8, 12), uint3(12, 8, 13),
	 uint3(17, 13, 9), uint3(18, 10, 11), uint3(14, 11, 12), uint3(14, 16, 11),
	 uint3(14, 12, 13), uint3(15, 14, 13), uint3(16, 18, 11), uint3(17, 15, 13),
 };
 static const float3 k_barycentric[19] =
 {
	 float3(0, 1, 0), float3(0, 0.75, 0.25), float3(0.25, 0.75, 0), float3(0.166721, 0.666567, 0.166721),
	 float3(0.5, 0.5, 0), float3(0, 0.5, 0.5), float3(0.416692, 0.416620, 0.166694), float3(0.166694, 0.416620, 0.416692),
	 float3(0.333359, 0.333287, 0.333359), float3(0.75, 0.25, 0), float3(0, 0.25, 0.75), float3(0.166712, 0.166659, 0.666631),
	 float3(0.416673, 0.166657, 0.416673), float3(0.666628, 0.166656, 0.166718), float3(0.500000, 0.000000, 0.500000), float3(0.75, 0, 0.25),
	 float3(0.25, 0.000000, 0.75), float3(1, 0, 0), float3(0, 0, 1),
 };

groupshared Payload s_Payload;
groupshared uint s_TotalTessellated;

// This is a heuristic that maps a bounding sphere on relevant edges/primitive to post projection space and uses that to scale the tessellation on the edges/primitive.
//
// Returns float4 where:
// X - 0->1 Edge tessellation factor
// Y - 1->2 Edge tessellation factor
// Z - 2->0 Edge tessellation factor
// W - inside tessellation factor
float4 CalculateCompositeTessellationFactors(float3 Control0, float3 Control1, float3 Control2)
{
#if USE_ADAPTIVE_TESSELLATION_FACTOR
#if 1
	half MaxDisplacement = GetMaterialMaxDisplacement();

	// Frustum cull
	int3 ClipFlag = 0;
	ClipFlag  = GetClipFlag( Control0, MaxDisplacement );
	ClipFlag |= GetClipFlag( Control1, MaxDisplacement );
	ClipFlag |= GetClipFlag( Control2, MaxDisplacement );
	if( any( ClipFlag != 3 ) )
	{
		return 0;
	}
#endif

	float3 Edge0 = ( Control0 - Control1 );
	float3 Edge1 = ( Control1 - Control2 );
	float3 Edge2 = ( Control2 - Control0 );

	float3 ToMidpoint0 = 0.5 * ( Control0 + Control1 ) - ResolvedView.TranslatedWorldCameraOrigin;
	float3 ToMidpoint1 = 0.5 * ( Control1 + Control2 ) - ResolvedView.TranslatedWorldCameraOrigin;
	float3 ToMidpoint2 = 0.5 * ( Control2 + Control0 ) - ResolvedView.TranslatedWorldCameraOrigin;

	// Use spherical projection instead of planer
	float4 CompositeFactors = float4(
		sqrt( dot( Edge1, Edge1 ) / dot( ToMidpoint1, ToMidpoint1 ) ),
		sqrt( dot( Edge2, Edge2 ) / dot( ToMidpoint2, ToMidpoint2 ) ),
		sqrt( dot( Edge0, Edge0 ) / dot( ToMidpoint0, ToMidpoint0 ) ),
		1 );
	CompositeFactors.w = 0.333 * ( CompositeFactors.x + CompositeFactors.y + CompositeFactors.z );

	// The adaptive tessellation factor is 0.5 * ResolvedView.ViewToClip[1][1] * ViewSizeY / PixelsPerEdge and CompositeFactors is 2 * PercentageOfScreen.  
	return View.AdaptiveTessellationFactor * CompositeFactors;
#else
	return float4( 1.0,1.0,1.0,1.0 );
#endif
}

uint CalcTessInsideCount(uint n)
{
	if ((n & 1) == 0)
	{
		// even
		return n * n * 3 / 2;
	}
	else
	{
		// odd
		return (n / 2) * 3 * (n + 1) + 1;
	}
}

// TODO: Improve tess factor algorithm use CalculateCompositeTessellationFactors() from UE4
uint CalcTessellationCount(FNaniteView NaniteView, FVisibleCluster VisibleCluster, FCluster Cluster, float3 p0, float3 p1, float3 p2, out uint4 tess_factor)
{
	VSOut V[3];
	V[0] = CommonRasterizerVS(NaniteView, VisibleCluster, Cluster, p0, 0);
	V[1] = CommonRasterizerVS(NaniteView, VisibleCluster, Cluster, p1, 0);
	V[2] = CommonRasterizerVS(NaniteView, VisibleCluster, Cluster, p2, 0);

	float2 ScreenPos0 = V[0].PointClipPixel.xy;
	float2 ScreenPos1 = V[1].PointClipPixel.xy;
	float2 ScreenPos2 = V[2].PointClipPixel.xy;

	float tess01f = round(length(ScreenPos0 - ScreenPos1) / 100);
	float tess02f = round(length(ScreenPos0 - ScreenPos2) / 100);
	float tess12f = round(length(ScreenPos1 - ScreenPos2) / 100);

	float tess_centerf = round(0.333 * (tess01f + tess02f + tess12f));

	tess_factor.x = (uint)(max(1.0, tess01f));
	tess_factor.y = (uint)(max(1.0, tess02f));
	tess_factor.z = (uint)(max(1.0, tess12f));
	tess_factor.w = (uint)(max(1.0, tess_centerf));

	if (tess_factor.w == 1)
	{
		return max(max(tess_factor.x, tess_factor.y), tess_factor.z);
	}
	else
	{
		uint tess_segs = tess_factor.w - 2;
		return CalcTessInsideCount( tess_segs ) + tess_segs * 3 + tess_factor.x + tess_factor.y + tess_factor.z;
	}
}

uint PackTessInfo(uint TriangleIndex, uint TessOffset, uint TessCount)
{
	uint v = 0;
	v |= TriangleIndex;
	v |= (TessOffset << 6);
	v |= (TessCount << 11);
	return v;
}

uint3 UnpackTessInfo(uint Value)
{
	uint3 result;
	result.x = Value & 0x3f;	// Triangle Index
	result.y = (Value >> 6) & 0x1f;	// Tess Offset
	result.z = (Value >> 11) & 0x1f;	// Tess Count
	return result;
}

void WriteToTable(uint Value, uint Index)
{
	uint offset = Index / 2;
	if ((Index & 1) == 0)
	{
		s_Payload.MSTable[offset] &= 0x0000ffff;
		s_Payload.MSTable[offset] |= (Value << 16);
	}
	else
	{
		s_Payload.MSTable[offset] &= 0xffff0000;
		s_Payload.MSTable[offset] |= (Value & 0xffff);
	}
}

uint ReadFromTable(uint Index)
{
	uint offset = Index / 2;
	if ((Index & 1) == 0)
	{
		return s_Payload.MSTable[offset] >> 16;
	}
	else
	{
		return s_Payload.MSTable[offset] & 0xffff;
	}
}

[RootSignature(HWRasterizeRS)]
[NumThreads(AS_GROUP_SIZE, 1, 1)]
void HWRasterizeAS(	
	uint GroupThreadID : SV_GroupThreadID,
	uint GroupId : SV_GroupID, 
	uint GroupIndex : SV_GroupIndex)
{
	if (GroupIndex == 0)
		s_TotalTessellated = 0;
	
	GroupMemoryBarrierWithGroupSync();	

	// an AS group deal with 64 triangles max, use 2 as_groups to deal with 1 cluster
	uint VisibleIndex = GroupId / 2;
	uint TriangleOffset = ((GroupId & 1) == 0) ? 0 : 64;
	//uint VisibleIndex = GroupId;
	//uint TriangleOffset = 0;
	
	VisibleIndex = (DecodeInfo.MaxVisibleClusters - 1) - VisibleIndex;

	s_Payload.VisibleClusterIndex = VisibleIndex;
	s_Payload.TriangleOffset = TriangleOffset;

	FVisibleCluster VisibleCluster = GetVisibleCluster(VisibleIndex, 0);	
	FCluster Cluster = GetCluster(VisibleCluster.PageIndex, VisibleCluster.ClusterIndex);
	FTriRange TriRange = GetTriangleRange(Cluster, false, uint3(0,0,0));

	FNaniteView NaniteView = GetNaniteView(VisibleCluster.ViewId);

	uint TrianglesToTess = TriangleOffset == 0 ? min(TriRange.Num, 64u) : (TriRange.Num <= 64 ? 0 : TriRange.Num - 64);
	//uint TrianglesToTess = TriangleOffset == 0 ? 64 : TriRange.Num - 64;
	//uint TrianglesToTess = 64;// TriRange.Num;
	uint VisibleTriangles = 0;
	[branch]
	if (GroupIndex + TriangleOffset < TriRange.Num)
	{
		uint TriangleIndex = TriRange.Start + GroupThreadID + TriangleOffset;

		uint3 TriangleIndices = ReadTriangleIndices(Cluster, TriangleIndex);
		float3 p0 = DecodePosition( TriangleIndices.x, Cluster );
		float3 p1 = DecodePosition( TriangleIndices.y, Cluster );
		float3 p2 = DecodePosition( TriangleIndices.z, Cluster );

		FNaniteRawAttributeData AttrData[3];
		GetRawAttributeDataN(AttrData, Cluster, TriangleIndices, 3, 1);

		float3 N[3];
		N[0] = AttrData[0].TangentZ;
		N[1] = AttrData[1].TangentZ;
		N[2] = AttrData[2].TangentZ;

		// Force this func inline, due to:
		// error GB91F0BD6: TGSM pointers must originate from an unambiguous TGSM global variable.
		//CalcPNTriangleControlPoints(p0, p1, p2, N[0], N[1], N[2], s_Payload.CT[GroupIndex]);
		float3 n0 = N[0];
		float3 n1 = N[1];
		float3 n2 = N[2];

		float3 b003 = p0;
		float3 b030 = p1;
		float3 b300 = p2;
		float3 n002 = n0;
		float3 n020 = n1;
		float3 n200 = n2;
		float3 b210 = ((b003 * 2.f) + b030 - (dot((b030 - b003), n002) * n002)) / 3.f;
		float3 b120 = ((b030 * 2.f) + b003 - (dot((b003 - b030), n020) * n020)) / 3.f;
		float3 b021 = ((b030 * 2.f) + b300 - (dot((b300 - b030), n020) * n020)) / 3.f;
		float3 b012 = ((b300 * 2.f) + b030 - (dot((b030 - b300), n200) * n200)) / 3.f;
		float3 b102 = ((b300 * 2.f) + b003 - (dot((b003 - b300), n200) * n200)) / 3.f;
		float3 b201 = ((b003 * 2.f) + b300 - (dot((b300 - b003), n002) * n002)) / 3.f;

		s_Payload.CT[GroupIndex].P[0 * 3 + 0] = b210.x;
		s_Payload.CT[GroupIndex].P[0 * 3 + 1] = b210.y;
		s_Payload.CT[GroupIndex].P[0 * 3 + 2] = b210.z;
		s_Payload.CT[GroupIndex].P[1 * 3 + 0] = b120.x;
		s_Payload.CT[GroupIndex].P[1 * 3 + 1] = b120.y;
		s_Payload.CT[GroupIndex].P[1 * 3 + 2] = b120.z;
		s_Payload.CT[GroupIndex].P[2 * 3 + 0] = b021.x;
		s_Payload.CT[GroupIndex].P[2 * 3 + 1] = b021.y;
		s_Payload.CT[GroupIndex].P[2 * 3 + 2] = b021.z;
		s_Payload.CT[GroupIndex].P[3 * 3 + 0] = b012.x;
		s_Payload.CT[GroupIndex].P[3 * 3 + 1] = b012.y;
		s_Payload.CT[GroupIndex].P[3 * 3 + 2] = b012.z;
		s_Payload.CT[GroupIndex].P[4 * 3 + 0] = b102.x;
		s_Payload.CT[GroupIndex].P[4 * 3 + 1] = b102.y;
		s_Payload.CT[GroupIndex].P[4 * 3 + 2] = b102.z;
		s_Payload.CT[GroupIndex].P[5 * 3 + 0] = b201.x;
		s_Payload.CT[GroupIndex].P[5 * 3 + 1] = b201.y;
		s_Payload.CT[GroupIndex].P[5 * 3 + 2] = b201.z;

		float3 E = (b210 + b120 + b021 + b012 + b102 + b201) / 6.f;
		float3 V = (b003 + b030 + b300) / 3.f;
		float3 b111 = E + ((E - V) / 2.f);
		s_Payload.CT[GroupIndex].P[6 * 3 + 0] = b111.x;
		s_Payload.CT[GroupIndex].P[6 * 3 + 1] = b111.y;
		s_Payload.CT[GroupIndex].P[6 * 3 + 2] = b111.z;

		float v12 = 2.f * dot(b030 - b003, n002 + n020) / dot(b030 - b003, b030 - b003);
		float3 n110 = normalize(n002 + n020 - (b030 - b003) * v12);
		float v23 = 2.f * dot(b300 - b030, n020 + n200) / dot(b300 - b030, b300 - b030);
		float3 n011 = normalize(n020 + n200 - (b300 - b030) * v23);
		float v31 = 2.f * dot(b003 - b300, n200 + n002) / dot(b003 - b300, b003 - b300);
		float3 n101 = normalize(n200 + n002 - (b003 - b300) * v31);
		s_Payload.CT[GroupIndex].P[7 * 3 + 0] = n110.x;
		s_Payload.CT[GroupIndex].P[7 * 3 + 1] = n110.y;
		s_Payload.CT[GroupIndex].P[7 * 3 + 2] = n110.z;
		s_Payload.CT[GroupIndex].P[8 * 3 + 0] = n011.x;
		s_Payload.CT[GroupIndex].P[8 * 3 + 1] = n011.y;
		s_Payload.CT[GroupIndex].P[8 * 3 + 2] = n011.z;
		s_Payload.CT[GroupIndex].P[9 * 3 + 0] = n101.x;
		s_Payload.CT[GroupIndex].P[9 * 3 + 1] = n101.y;
		s_Payload.CT[GroupIndex].P[9 * 3 + 2] = n101.z;

		uint4 tess_factor;
		uint NumTessellated = CalcTessellationCount(NaniteView, VisibleCluster, Cluster, p0, p1, p2, tess_factor);
		s_Payload.TF[GroupIndex].Factor[0] = tess_factor.x;
		s_Payload.TF[GroupIndex].Factor[1] = tess_factor.y;
		s_Payload.TF[GroupIndex].Factor[2] = tess_factor.z;
		s_Payload.TF[GroupIndex].Factor[3] = tess_factor.w;
#if DEBUG_INFO
		DebugInfo[GroupIndex + TriangleOffset].TessFactor = tess_factor;
		DebugInfo[GroupIndex + TriangleOffset].TessedCount = NumTessellated; 
#endif
		uint groups = (uint)ceil(float(NumTessellated)/32.0);
		uint start;
		InterlockedAdd(s_TotalTessellated, groups, start);
		for(int i = 0; i < groups; i ++)
		{
			uint count_in_group = min(32, NumTessellated - (i * 32));
			uint info = PackTessInfo(GroupIndex, i, count_in_group - 1);
			//WriteToTable(info, start + i);
		}
	}
	DispatchMesh(TrianglesToTess, 1, 1, s_Payload);
}

#define TRIANGLE_TESSELLATED 24
[RootSignature(HWRasterizeRS)]
[numthreads(TRIANGLE_TESSELLATED, 1, 1)]
[outputtopology("triangle")]
void HWRasterizeMS(
	uint GroupThreadID : SV_GroupThreadID,
	uint GroupID : SV_GroupID,
    in payload Payload payload,
	out vertices VSOut OutVertices[TRIANGLE_TESSELLATED],
	out indices uint3 OutTriangles[TRIANGLE_TESSELLATED],
	out primitives PrimitiveAttributes OutPrimitives[TRIANGLE_TESSELLATED]
)
{

	uint3 RasterBin;

	uint VisibleIndex = payload.VisibleClusterIndex;

	FVisibleCluster VisibleCluster = GetVisibleCluster(VisibleIndex, 0);
	//FInstanceSceneData InstanceData = GetInstanceSceneData(VisibleCluster.InstanceId, false);

	FNaniteView NaniteView = GetNaniteView(VisibleCluster.ViewId);

//#if NANITE_VERTEX_PROGRAMMABLE
//	ResolvedView = ResolveView(NaniteView);
//#endif

	FCluster Cluster = GetCluster(VisibleCluster.PageIndex, VisibleCluster.ClusterIndex);
	uint TriangleIndex = GroupID + payload.TriangleOffset;
	uint3 TriangleIndices = ReadTriangleIndices(Cluster, TriangleIndex);

	bool NeedTess = Cluster.MipLevel == 0;
	uint NumVerts = NeedTess ? 19 : 3;
	uint NumTri = NeedTess ? 24 : 1;

	SetMeshOutputCounts(NumVerts, NumTri);

	[branch]
	if (NeedTess)
	{
		OutTriangles[GroupThreadID] = k_triangles[GroupThreadID];

		if (GroupThreadID < 19)
		{
			// calc PN Triangle points
			float3 p0 = DecodePosition(TriangleIndices.x, Cluster);
			float3 p1 = DecodePosition(TriangleIndices.y, Cluster);
			float3 p2 = DecodePosition(TriangleIndices.z, Cluster);

			FNaniteRawAttributeData AttrData[3];
			GetRawAttributeDataN(AttrData, Cluster, TriangleIndices, 3, 1);

			float3 N[3];
			N[0] = AttrData[0].TangentZ;
			N[1] = AttrData[1].TangentZ;
			N[2] = AttrData[2].TangentZ;

			float3 n0 = N[0];
			float3 n1 = N[1];
			float3 n2 = N[2];

			// Get barycoords
			float3 uvw = k_barycentric[GroupThreadID];
			float U = uvw.x;
			float V = uvw.y;
			float W = uvw.z;

			float UU = U * U;
			float VV = V * V;
			float WW = W * W;
			float UU3 = UU * 3.f;
			float VV3 = VV * 3.f;
			float WW3 = WW * 3.f;

			float3 b210, b120, b021, b012, b102, b201, b111;
			b210.x = payload.CT[GroupID].P[0 * 3 + 0];
			b210.y = payload.CT[GroupID].P[0 * 3 + 1];
			b210.z = payload.CT[GroupID].P[0 * 3 + 2];
			b120.x = payload.CT[GroupID].P[1 * 3 + 0];
			b120.y = payload.CT[GroupID].P[1 * 3 + 1];
			b120.z = payload.CT[GroupID].P[1 * 3 + 2];
			b021.x = payload.CT[GroupID].P[2 * 3 + 0];
			b021.y = payload.CT[GroupID].P[2 * 3 + 1];
			b021.z = payload.CT[GroupID].P[2 * 3 + 2];
			b012.x = payload.CT[GroupID].P[3 * 3 + 0];
			b012.y = payload.CT[GroupID].P[3 * 3 + 1];
			b012.z = payload.CT[GroupID].P[3 * 3 + 2];
			b102.x = payload.CT[GroupID].P[4 * 3 + 0];
			b102.y = payload.CT[GroupID].P[4 * 3 + 1];
			b102.z = payload.CT[GroupID].P[4 * 3 + 2];
			b201.x = payload.CT[GroupID].P[5 * 3 + 0];
			b201.y = payload.CT[GroupID].P[5 * 3 + 1];
			b201.z = payload.CT[GroupID].P[5 * 3 + 2];
			b111.x = payload.CT[GroupID].P[6 * 3 + 0];
			b111.y = payload.CT[GroupID].P[6 * 3 + 1];
			b111.z = payload.CT[GroupID].P[6 * 3 + 2];

			// update Position
			float3 P =
				p0 * WW * W +
				p1 * UU * U +
				p2 * VV * V +
				b210 * WW3 * U +
				b120 * W * UU3 +
				b201 * WW3 * V +
				b021 * UU3 * V +
				b102 * W * VV3 +
				b012 * U * VV3 +
				b111 * 6.f * W * U * V;

			float3 n110, n011, n101;
			n110.x = payload.CT[GroupID].P[7 * 3 + 0];
			n110.y = payload.CT[GroupID].P[7 * 3 + 1];
			n110.z = payload.CT[GroupID].P[7 * 3 + 2];
			n011.x = payload.CT[GroupID].P[8 * 3 + 0];
			n011.y = payload.CT[GroupID].P[8 * 3 + 1];
			n011.z = payload.CT[GroupID].P[8 * 3 + 2];
			n101.x = payload.CT[GroupID].P[9 * 3 + 0];
			n101.y = payload.CT[GroupID].P[9 * 3 + 1];
			n101.z = payload.CT[GroupID].P[9 * 3 + 2];

			// update Normal
			float3 Normal =
				n0 * WW +
				n1 * UU +
				n2 * VV +
				n110 * W * U +
				n011 * U * V +
				n101 * W * V;
			Normal = normalize(Normal);
			OutVertices[GroupThreadID] = CommonRasterizerVS(NaniteView, VisibleCluster, Cluster, P, 0);
			OutVertices[GroupThreadID].Normal = Normal;

			// update uv
			OutVertices[GroupThreadID].UV = AttrData[1].TexCoords[0] * uvw.x + AttrData[2].TexCoords[0] * uvw.y + AttrData[0].TexCoords[0] * uvw.z;

			PrimitiveAttributes Attributes;
			Attributes.PackedData.x = VisibleIndex;
			Attributes.PackedData.y = TriangleIndex;
			uint3 color = Rand3DPCG16(Cluster.GroupIndex.xxx);
			uint PackedColor = (color.x & 0xff) | ((color.y & 0xff) << 8) | ((color.z & 0xff) << 16);
			Attributes.PackedData.z = PackedColor;//asuint(NaniteView.ViewSizeAndInvSize.x);
			Attributes.PackedData.w = 0;//asuint(NaniteView.ViewSizeAndInvSize.y);
			OutPrimitives[GroupThreadID] = Attributes;
		}
	}
	else
	{
		if (GroupThreadID == 0)
			OutTriangles[0] = uint3(0, 1, 2);

		if (GroupThreadID < 3)
		{
			OutVertices[GroupThreadID] = CommonRasterizerVS(NaniteView, VisibleCluster, Cluster, TriangleIndices[GroupThreadID], 0);

			FNaniteRawAttributeData AttrData = GetRawAttributeData(Cluster, TriangleIndices[GroupThreadID], 1);
			OutVertices[GroupThreadID].Normal = AttrData.TangentZ;
			OutVertices[GroupThreadID].UV = AttrData.TexCoords[0];

			PrimitiveAttributes Attributes;
			Attributes.PackedData.x = VisibleIndex;
			Attributes.PackedData.y = TriangleIndex;
			uint3 color = Rand3DPCG16(Cluster.GroupIndex.xxx);
			uint PackedColor = (color.x & 0xff) | ((color.y & 0xff) << 8) | ((color.z & 0xff) << 16);
			Attributes.PackedData.z = PackedColor;//asuint(NaniteView.ViewSizeAndInvSize.x);
			Attributes.PackedData.w = 0;//asuint(NaniteView.ViewSizeAndInvSize.y);
			OutPrimitives[GroupThreadID] = Attributes;
		}
	}
}

struct SVisBufferWriteParameters
{
	uint2 PixelPos;
	uint PixelValue;
	float DeviceZ;

//#if VIRTUAL_TEXTURE_TARGET
//	WritePixelPageTranslation PageTranslation;
//
//#if ENABLE_EARLY_Z_TEST
//	uint3 PhysicalPixelPos;
//#endif
//#endif
};

SVisBufferWriteParameters InitializeVisBufferWriteParameters(
	uint2 PixelPos,
	//WritePixelPageTranslation PageTranslation,
	uint PixelValue,
	float DeviceZ
)
{
	SVisBufferWriteParameters Out = (SVisBufferWriteParameters)0;
	Out.PixelPos = PixelPos;
	Out.PixelValue = PixelValue;
	Out.DeviceZ = DeviceZ;

//#if VIRTUAL_TEXTURE_TARGET
//	Out.PageTranslation = PageTranslation;
//#endif

	return Out;
}

void WritePixel(
	RWTexture2D<UlongType> OutBuffer,
	uint PixelValue,
	uint2 PixelPos,
	float DeviceZ
)
{
	// When near clipping is disabled we need to move the geometry 
	DeviceZ = saturate(DeviceZ);

	const uint DepthInt = asuint(DeviceZ);

//#if DEPTH_ONLY
//	InterlockedMax(OutDepthBuffer[PixelPos], DepthInt);
//#elif COMPILER_SUPPORTS_UINT64_IMAGE_ATOMICS
	
	// tix: make it simple first
	OutBuffer[PixelPos] = uint2(PixelValue, PixelValue);
	//const UlongType Pixel = PackUlongType(uint2(PixelValue, DepthInt));
	//ImageInterlockedMaxUInt64(OutBuffer, PixelPos, Pixel);
//#else
//#error UNKNOWN_ATOMIC_PLATFORM
//#endif
}

void WriteToVisBuffer(SVisBufferWriteParameters Params)
{
//#if VIRTUAL_TEXTURE_TARGET
//	// TODO: Support Nanite visualization in this path
//
//#if ENABLE_EARLY_Z_TEST
//// We should have already pre-calculated the physical pixel position
//	WriteDepthTextureArray(Params.PhysicalPixelPos, Params.DeviceZ);
//#else
//	WriteDepthTextureArray(Params.PixelPos, Params.PageTranslation, Params.DeviceZ);
//#endif
//#else
	WritePixel(OutVisBuffer64, Params.PixelValue, Params.PixelPos, Params.DeviceZ);

//#if VISUALIZE
//	const uint2 VisualizeValues = GetVisualizeValues();
//	WritePixel(OutDbgBuffer64, VisualizeValues.x, Params.PixelPos, Params.DeviceZ);
//	InterlockedAdd(OutDbgBuffer32[Params.PixelPos], VisualizeValues.y);
//#endif
//#endif	// VIRTUAL_TEXTURE_TARGET
}

[RootSignature(HWRasterizeRS)]
float4 HWRasterizePS(VSOut In
#if NANITE_MESH_SHADER	
 	, PrimitiveAttributes Primitive
#endif
) : SV_Target0
{
	float4 SvPosition = float4(In.PointClipPixel.xyz / In.PointClipPixel.w, In.PointClipPixel.w);
	uint2 PixelPos = (uint2)SvPosition.xy;

	//const uint ViewId = BitFieldExtractU32(In.PixelValue_ViewId_Mip_ArrayIndex_LevelOffset.y, 16, 0);
	
	//const FNaniteView NaniteView = GetNaniteView(ViewId);
	
	float DeviceZ = SvPosition.z;
	float MaterialMask = 1.0f; // TODO: PROG_RASTER


#if NANITE_MESH_SHADER
 	// In.PixelValue will be 0 here because mesh shaders will pass down the following indices through per-primitive attributes.
 	uint VisibleClusterIndex = Primitive.PackedData.x;
 	uint TriIndex = Primitive.PackedData.y;
#else
	uint VisibleClusterIndex = In.PixelValue_ViewId_Mip_ArrayIndex_LevelOffset.x;
	uint TriIndex = In.PixelValue_ViewId_Mip_ArrayIndex_LevelOffset.y;
#endif

	uint PixelValue = ((VisibleClusterIndex + 1) << 7) | TriIndex;
	SVisBufferWriteParameters WriteParams = InitializeVisBufferWriteParameters(PixelPos/*PageTranslation*/, PixelValue, DeviceZ);
	//[branch]
	//if (MaterialMask >= 0)
	{
		//WriteToVisBuffer(WriteParams);
	}

 	// In.PixelValue will be 0 here because mesh shaders will pass down the following indices through per-primitive attributes.
	uint PackedColor = Primitive.PackedData.z;
	uint3 DebugColor = uint3(PackedColor & 0xff, (PackedColor & 0xff00) >> 8, (PackedColor & 0xff0000) >> 16);
	float3 C = (float3)DebugColor / 255.0;
	//return float4(C.xyz, 1.0);

	if (VisibleClusterIndex != 0xFFFFFFFF)
	{
		FVisibleCluster VisibleCluster = GetVisibleCluster( VisibleClusterIndex );

		FCluster Cluster = GetCluster(VisibleCluster.PageIndex, VisibleCluster.ClusterIndex);

		uint3 TriIndices = uint3(0, 0, 0);
		const bool bCalcTriIndices = true;
		if (bCalcTriIndices)
		{
			TriIndices = ReadTriangleIndices(Cluster, TriIndex);
		}

		float3 Normal = normalize(In.Normal);
		//C = Normal * 0.5 + 0.5;
	}
	// debug show uv
	C = float3(In.UV, 0.0);
	return float4(C.xyz, 1.0);
}