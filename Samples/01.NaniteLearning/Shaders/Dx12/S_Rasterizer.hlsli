/*
	TiX Engine v3.0 Copyright (C) 2018~2021
	By ZhaoShuai tirax.cn@gmail.com
*/

// NaniteLearning Sample
#include "Common.hlsli"
#include "NaniteDefinitions.h"
#include "NaniteAttributeDecode.h"


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
	nointerpolation uint3 PixelValue_ViewId_Mip_ArrayIndex_LevelOffset : TEXCOORD1;
//#if NANITE_MULTI_VIEW
//	nointerpolation int4 ViewRect				: TEXCOORD2;
//#endif
//#if VERTEX_TO_TRIANGLE_MASKS
//#if NANITE_VERT_REUSE_BATCH
//	CUSTOM_INTERPOLATION uint2 ToTriangleMask_TriRangeStart : TEXCOORD3;
//#else
//	CUSTOM_INTERPOLATION uint4 ToTriangleMasks	: TEXCOORD3;
//#endif
//#endif

// #if BARYCENTRIC_MODE_INTRINSICS
// 	CUSTOM_INTERPOLATION uint VertexID			: TEXCOORD4;
// #elif BARYCENTRIC_MODE_SV_BARYCENTRICS && PIXELSHADER
// 	float3 Barycentrics							: SV_Barycentrics;
// #elif BARYCENTRIC_MODE_EXPORT
// 	float2 BarycentricsUV						: TEXCOORD4;
// #endif

//#if !PIXELSHADER
	float4 Position								: SV_Position;	 // Reading SV_Position in the pixel shader limits launch rate on some hardware. Interpolate manually instead.
//#endif
};


RWTexture2D<UlongType>	OutVisBuffer64 : register(u0);

VSOut CommonRasterizerVS(FNaniteView NaniteView, FVisibleCluster VisibleCluster, FCluster Cluster, uint VertIndex, uint PixelValue)
{
	VSOut Out;

	const float3 PointLocal = DecodePosition( VertIndex, Cluster );

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
	float4 PointClip = mul( float4( PointTranslatedWorld, 1 ), NaniteView.TranslatedWorldToClip );
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
	
	Out.PixelValue_ViewId_Mip_ArrayIndex_LevelOffset = uint3(PixelValue, 0u, 0u);

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
	"DescriptorTable(SRV(t0, numDescriptors=4), UAV(u0)) "

[RootSignature(HWRasterizeRS)]
VSOut HWRasterizeVS(
	uint VertexID		: SV_VertexID,
	uint VisibleIndex	: SV_InstanceID
	)
{
	uint3 RasterBin;

	VisibleIndex = (DecodeInfo.MaxVisibleClusters - 1) - VisibleIndex; // HW clusters are written from the top (in the visible cluster list)

	uint LocalTriIndex = VertexID / 3;
	VertexID = VertexID - LocalTriIndex * 3;

	VSOut Out;
	Out.Position = float4(0,0,0,1);

	FVisibleCluster VisibleCluster = GetVisibleCluster( VisibleIndex, 0 );
	//FInstanceSceneData InstanceData = GetInstanceSceneData( VisibleCluster.InstanceId, false );

	FNaniteView NaniteView = GetNaniteView( VisibleCluster.ViewId );

//#if NANITE_VERTEX_PROGRAMMABLE
//	ResolvedView = ResolveView(NaniteView);
//#endif

	FCluster Cluster = GetCluster(VisibleCluster.PageIndex, VisibleCluster.ClusterIndex);
	FTriRange TriRange = GetTriangleRange(Cluster, false, RasterBin);

	[branch]
	if( LocalTriIndex < TriRange.Num )
	{
		const uint TriIndex = TriRange.Start + LocalTriIndex;
		uint3 TriangleIndices = ReadTriangleIndices(Cluster, TriIndex);
		//if (ReverseWindingOrder(InstanceData))
		//{
		//	TriangleIndices = TriangleIndices.xzy;
		//}

		const uint PixelValue = ((VisibleIndex + 1) << 7) | TriIndex;
		Out = CommonRasterizerVS(NaniteView, VisibleCluster, Cluster, TriangleIndices[VertexID], PixelValue);
// #if BARYCENTRIC_MODE_EXPORT
// 		Out.BarycentricsUV = float2(
// 			VertexID == 0 ? 1 : 0,
// 			VertexID == 1 ? 1 : 0
// 		);
// #endif
		// tix: overwrite it for test
		Out.PixelValue_ViewId_Mip_ArrayIndex_LevelOffset.x = VisibleIndex;
		Out.PixelValue_ViewId_Mip_ArrayIndex_LevelOffset.y = TriIndex;

		// Calc a color value by GroupIndex for debug
		uint3 color = Rand3DPCG16(Cluster.GroupIndex.xxx);
		uint PackedColor = (color.x & 0xff) | ((color.y & 0xff) << 8) | ((color.z & 0xff) << 16);
		Out.PixelValue_ViewId_Mip_ArrayIndex_LevelOffset.z = PackedColor;
	}

	return Out;
}

#define NANITE_MESH_SHADER_TG_SIZE 128
struct PrimitiveAttributes
{
	// Use uint4 to prevent compiler from erroneously packing per-vertex and per-prim attributes together
	// .x = Cluster Index
	// .y = Triangle Index
	// .z = View Width
	// .w = View Height
	nointerpolation uint4 PackedData : TEXCOORD7;
};

#if NANITE_MESH_SHADER

#define AS_GROUP_SIZE 1
struct Payload
{
    uint VisibleClusterIndex[AS_GROUP_SIZE];
};
groupshared Payload s_Payload;
[RootSignature(HWRasterizeRS)]
[NumThreads(AS_GROUP_SIZE, 1, 1)]
void HWRasterizeAS(uint dispatchId : SV_DispatchThreadID, uint GroudId : SV_GroupID, uint GroupIndex : SV_GroupIndex)
{
	s_Payload.VisibleClusterIndex[GroupIndex] = dispatchId;	
    DispatchMesh(AS_GROUP_SIZE, 1, 1, s_Payload);
}



[RootSignature(HWRasterizeRS)]
//MESH_SHADER_TRIANGLE_ATTRIBUTES(NANITE_MESH_SHADER_TG_SIZE)
[numthreads(NANITE_MESH_SHADER_TG_SIZE, 1, 1)]
[outputtopology("triangle")]
void HWRasterizeMS(
	uint GroupThreadID : SV_GroupThreadID,
	uint GroupID : SV_GroupID,
#if USE_AS_SHADER
    in payload Payload payload,
#endif
	//MESH_SHADER_VERTEX_EXPORT(VSOut, 256),
	out vertices VSOut OutVertices[256],
	//MESH_SHADER_TRIANGLE_EXPORT(128),
	out indices uint3 OutTriangles[128],
	//MESH_SHADER_PRIMITIVE_EXPORT(PrimitiveAttributes, 128)
	out primitives PrimitiveAttributes OutPrimitives[128]
)
{

	uint3 RasterBin;

#if USE_AS_SHADER
	uint VisibleIndex = payload.VisibleClusterIndex[GroupID];
#else
	uint VisibleIndex = GroupID;
#endif
	VisibleIndex = (DecodeInfo.MaxVisibleClusters - 1) - VisibleIndex;

	FVisibleCluster VisibleCluster = GetVisibleCluster(VisibleIndex, 0);
	//FInstanceSceneData InstanceData = GetInstanceSceneData(VisibleCluster.InstanceId, false);

	FNaniteView NaniteView = GetNaniteView(VisibleCluster.ViewId);

//#if NANITE_VERTEX_PROGRAMMABLE
//	ResolvedView = ResolveView(NaniteView);
//#endif

	FCluster Cluster = GetCluster(VisibleCluster.PageIndex, VisibleCluster.ClusterIndex);
	FTriRange TriRange = GetTriangleRange(Cluster, false, RasterBin);

	SetMeshOutputCounts(Cluster.NumVerts, TriRange.Num);

	[branch]
	if (GroupThreadID < TriRange.Num)
	{
		uint TriangleIndex = TriRange.Start + GroupThreadID;

		uint3 TriangleIndices = ReadTriangleIndices(Cluster, TriangleIndex);
		//if (ReverseWindingOrder(InstanceData))
		//{
		//	TriangleIndices = TriangleIndices.xzy;
		//}

		//MESH_SHADER_WRITE_TRIANGLE(GroupThreadID, TriangleIndices);
		OutTriangles[GroupThreadID] = TriangleIndices;

		PrimitiveAttributes Attributes;
		Attributes.PackedData.x = VisibleIndex;
		Attributes.PackedData.y = TriangleIndex;
		uint3 color = Rand3DPCG16(Cluster.GroupIndex.xxx);
		uint PackedColor = (color.x & 0xff) | ((color.y & 0xff) << 8) | ((color.z & 0xff) << 16);
		Attributes.PackedData.z = PackedColor;//asuint(NaniteView.ViewSizeAndInvSize.x);
		Attributes.PackedData.w = 0;//asuint(NaniteView.ViewSizeAndInvSize.y);
		//MESH_SHADER_WRITE_PRIMITIVE(GroupThreadID, Attributes);
		OutPrimitives[GroupThreadID] = Attributes;
	}

	const uint Vertex0 = GroupThreadID + 0;
	if (Vertex0 < Cluster.NumVerts)
	{
		const uint PixelValue = 0;
		VSOut VertexOutput = CommonRasterizerVS(NaniteView, VisibleCluster, Cluster, Vertex0, PixelValue);
		//MESH_SHADER_WRITE_VERTEX(Vertex0, VertexOutput);
		OutVertices[Vertex0] = VertexOutput;
	}

#if NANITE_MESH_SHADER_TG_SIZE == 128
	const uint Vertex1 = GroupThreadID + 128;
	if (Vertex1 < Cluster.NumVerts)
	{
		const uint PixelValue = 0;
		VSOut VertexOutput = CommonRasterizerVS(NaniteView, VisibleCluster, Cluster, Vertex1, PixelValue);
		//MESH_SHADER_WRITE_VERTEX(Vertex1, VertexOutput);
		OutVertices[Vertex1] = VertexOutput;
	}
#endif
}
#endif	// NANITE_MESH_SHADER

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


void GetRawAttributeDataN(inout float3 N[3],
	FCluster Cluster,
	uint3 TriIndices,
	uint CompileTimeN
)
{

	//CompileTimeMaxTexCoords = max(1, min( 4 , CompileTimeMaxTexCoords));

	const uint DecodeInfoOffset = Cluster.PageBaseAddress + Cluster.DecodeInfoOffset;
	const uint AttributeDataOffset = Cluster.PageBaseAddress + Cluster.AttributeOffset;

	// uint i;
	// [unroll]
	// for (i = 0; i < CompileTimeN; i++)
	// {
	// 	RawAttributeData[i] = (FNaniteRawAttributeData)0;
	// }
//#line 421 "/Engine/Private/Nanite/NaniteAttributeDecode.ush"
	const uint MaxAttributeBits = CalculateMaxAttributeBits(1);



	//uint4 ColorMin = uint4(UnpackByte0(Cluster.ColorMin), UnpackByte1(Cluster.ColorMin), UnpackByte2(Cluster.ColorMin), UnpackByte3(Cluster.ColorMin));
	//const uint4 NumComponentBits = UnpackToUint4(Cluster.ColorBits, 4);

	FBitStreamReaderState AttributeStream[3];
	[unroll]
	for (uint i = 0; i < CompileTimeN; i++)
	{
		AttributeStream[i] = BitStreamReader_Create_Aligned(AttributeDataOffset, TriIndices[i] * Cluster.BitsPerAttribute, MaxAttributeBits);
		const uint NormalBits = BitStreamReader_Read_RO( ClusterPageData , AttributeStream[i], 2 *  9 , 2 *  9 );
		N[i] = UnpackNormal(NormalBits,  9 );

		//const uint4 ColorDelta = BitStreamReader_Read4_RO( Nanite.ClusterPageData , AttributeStream[i], NumComponentBits,  8 );
		//RawAttributeData[i].Color = float4(ColorMin + ColorDelta) * (1.0f / 255.0f);
	}

	// [unroll]
	// for (uint TexCoordIndex = 0; TexCoordIndex < CompileTimeMaxTexCoords; ++TexCoordIndex)
	// {
	// 	const uint2 UVPrec = uint2(BitFieldExtractU32(Cluster.UV_Prec, 4, TexCoordIndex * 8), BitFieldExtractU32(Cluster.UV_Prec, 4, TexCoordIndex * 8 + 4));

	// 	uint2 UVBits[3];
	// 	[unroll]
	// 	for (uint i = 0; i < CompileTimeN; i++)
	// 	{
	// 		UVBits[i] = BitStreamReader_Read2_RO( Nanite.ClusterPageData , AttributeStream[i], UVPrec,  15 );
	// 	}

	// 	[branch]
	// 	if (TexCoordIndex < Cluster.NumUVs)
	// 	{
	// 		FUVRange UVRange = GetUVRange( Nanite.ClusterPageData , DecodeInfoOffset, TexCoordIndex);
	// 		[unroll]
	// 		for (uint i = 0; i < CompileTimeN; i++)
	// 		{
	// 			RawAttributeData[i].TexCoords[TexCoordIndex] = UnpackTexCoord(UVBits[i], UVRange);
	// 		}
	// 	}
	// }

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

	const uint ViewId = BitFieldExtractU32(In.PixelValue_ViewId_Mip_ArrayIndex_LevelOffset.y, 16, 0);
	
	const FNaniteView NaniteView = GetNaniteView(ViewId);
	
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
		WriteToVisBuffer(WriteParams);
	}
#if NANITE_MESH_SHADER
 	// In.PixelValue will be 0 here because mesh shaders will pass down the following indices through per-primitive attributes.
	uint PackedColor = Primitive.PackedData.z;
#else
	uint PackedColor = In.PixelValue_ViewId_Mip_ArrayIndex_LevelOffset.z;
#endif
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

		float3 N[3];
		GetRawAttributeDataN(N, Cluster, TriIndices, 3);
		//GetRawAttributeData3(RawAttributeData, Cluster, TriIndices,  0 );

		float3 Normal = normalize(N[0] + N[1] + N[2]);
		C = Normal * 0.5 + 0.5;
	}
	return float4(C.xyz, 1.0);
}