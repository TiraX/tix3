// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

//#include "../Common.ush"
#include "BitPacking.h"
//#include "../SceneData.ush"
#include "NaniteDefinitions.h"
#include "NanitePackedNaniteView.h"
#include "NaniteEnableMeshShader.h"

#ifndef DEBUG_FLAGS
    #define DEBUG_FLAGS 0
#endif

uint GetHWClusterCounterIndex(uint InRenderFlags)
{
	// tix: do not use mesh shader for now.
	// mesh shader should be 4, others 5.
#if NANITE_MESH_SHADER
	return 4;
#else
	return 5;
#endif
// Ensure rasterizer uses compile time constants.
//#ifdef NANITE_HW_COUNTER_INDEX
//	return NANITE_HW_COUNTER_INDEX;
//#else
//	// Other passes use a uniform branch to minimize permutations.
//	return CondMask(InRenderFlags & (NANITE_RENDER_FLAG_MESH_SHADER | NANITE_RENDER_FLAG_PRIMITIVE_SHADER), 4u, 5u);
//#endif
}

struct FVisibleClusterInstance
{
	uint	Flags;
	uint	ViewId;
	uint	InstanceId;
	uint	PageIndex;
	uint	ClusterInstanceIndex;
	uint2	vPage;
	uint2	vPageEnd;		// Last page to render (inclusive). Only used during SW rasterization currently
};

struct FPageHeader
{
	uint	NumClusters;
	uint	NumClusterInstances;
};

struct FCluster
{
	uint	PageBaseAddress;

	uint	NumVerts;
	uint	PositionOffset;

	uint	NumTris;
	uint	IndexOffset;

	int3	PosStart;
	uint	BitsPerIndex;
	int		PosPrecision;
	uint3	PosBits;

	// moved to cluster instance below
	//float4	LODBounds;

	//float3	BoxBoundsCenter;
	//uint	MipLevel;
	//float	LODError;
	//float	EdgeLength;

	//float3	BoxBoundsExtent;
	//uint	Flags;

	uint	AttributeOffset;
	uint	BitsPerAttribute;
	uint	DecodeInfoOffset;
	uint	NumUVs;
	uint	ColorMode;
	uint	UV_Prec;

	uint	ColorMin;
	uint	ColorBits;
	//uint	GroupIndex;		// Debug only

	// Material Slow path
	uint	MaterialTableOffset;
	uint	MaterialTableLength;

	uint	VertReuseBatchCountTableOffset;	// dword offset from page base
	uint	VertReuseBatchCountTableSize;	// number of entries, each 4-bit

	// Material Fast path
	uint	Material0Length;
	uint	Material0Index;
	uint 	Material1Length;
	uint	Material1Index;
	uint	Material2Index;

	uint4	VertReuseBatchInfo;
};

struct FClusterInstance
{
	float4	LODBounds;

	float3	BoxBoundsCenter;
	uint	MipLevel;
	uint	PageIndex;
	uint	ClusterIndex;
	float	LODError;
	float	EdgeLength;

	float3	BoxBoundsExtent;
	uint	Flags;
	uint	GroupIndex;		// Debug only
	
	float4x4 Transform;
};

struct FHierarchyNodeSlice
{
	float4	LODBounds;
	float3	BoxBoundsCenter;
	float3	BoxBoundsExtent;
	float	MinLODError;
	float	MaxParentLODError;
	uint	ChildStartReference;	// Can be node (index) or cluster (page:cluster)
	uint	NumChildren;
	uint	StartPageIndex;
	uint	NumPages;
	bool	bEnabled;
	bool	bLoaded;
	bool	bLeaf;
};

struct FInstanceDynamicData
{
	float4x4	LocalToTranslatedWorld;
	float4x4	PrevLocalToTranslatedWorld;
	bool		bHasMoved;
};

struct FNaniteView
{
	//float4x4	SVPositionToTranslatedWorld;
	//float4x4	ViewToTranslatedWorld;

	//float4x4	TranslatedWorldToView;
	float4x4	TranslatedWorldToClip;
	//float4x4	TranslatedWorldToSubpixelClip;	// Divide by w to get to absolute subpixel coordinates
	float4x4	ViewToClip;
	////FLWCMatrix	ClipToWorld;
	//float4x4	ClipToWorld;
	//
	//float4x4	PrevTranslatedWorldToView;
	//float4x4	PrevTranslatedWorldToClip;
	//float4x4	PrevViewToClip;
	////FLWCMatrix	PrevClipToWorld;
	//float4x4	PrevClipToWorld;

	int4		ViewRect;
	float4		ViewSizeAndInvSize;
	//float4		ClipSpaceScaleOffset;
	////FLWCVector3	PreViewTranslation;
	////FLWCVector3	PrevPreViewTranslation;
	////FLWCVector3	WorldCameraOrigin;
	//float3		PreViewTranslation;
	//float3		PrevPreViewTranslation;
	float4		WorldCameraOrigin;
	float3		ViewForward;
	//float3		ViewTilePosition;
	//float3		MatrixTilePosition;
	float		NearPlane;
	float		LODScale;
	float		LODScaleHW;
	//float		MinBoundsRadiusSq;
	uint		StreamingPriorityCategory;
	uint		Flags;
	//int			TargetLayerIndex;
	//int			TargetMipLevel;
	//int			TargetNumMipLevels;
	//int			TargetPrevLayerIndex;
	//float		RangeBasedCullingDistance;
	//int4		HZBTestViewRect;
};

struct FInstanceDraw
{
	uint InstanceId;
	uint ViewId;
};

struct FNaniteFullscreenVSToPS
{
#if INSTANCED_STEREO
	nointerpolation uint EyeIndex  : PACKED_EYE_INDEX;
#endif
	nointerpolation uint ViewIndex : PACKED_VIEW_INDEX;
};

// #if NANITE_USE_UNIFORM_BUFFER
// 	#define PageConstants			Nanite.PageConstants
// 	#define MaxNodes				Nanite.MaxNodes
// 	#define MaxVisibleClusters		Nanite.MaxVisibleClusters
// 	#define RenderFlags				Nanite.RenderFlags
// 	#define RayTracingCutError		Nanite.RayTracingCutError
// 	#define DebugFlags				Nanite.DebugFlags
// 	#define ClusterPageData			Nanite.ClusterPageData
// 	#define VisibleClustersSWHW		Nanite.VisibleClustersSWHW
// 	#define HierarchyBuffer			Nanite.HierarchyBuffer
// 	#define RayTracingDataBuffer	Nanite.RayTracingDataBuffer
// #elif NANITE_USE_RAYTRACING_UNIFORM_BUFFER
// 	#define PageConstants			NaniteRayTracing.PageConstants
// 	#define MaxNodes				NaniteRayTracing.MaxNodes
// 	#define MaxVisibleClusters		NaniteRayTracing.MaxVisibleClusters
// 	#define RenderFlags				NaniteRayTracing.RenderFlags
// 	#define RayTracingCutError		NaniteRayTracing.RayTracingCutError
// 	#define ClusterPageData			NaniteRayTracing.ClusterPageData
// 	#define HierarchyBuffer			NaniteRayTracing.HierarchyBuffer
// 	#define RayTracingDataBuffer	NaniteRayTracing.RayTracingDataBuffer

// 	// These parameters shouldn't be used in RT shaders
// 	//uint							DebugFlags;
// 	//ByteAddressBuffer				VisibleClustersSWHW;
// #else

struct FDecodeInfo
{
	uint4 PageConstants;
	uint MaxNodes;
	uint MaxVisibleClusters;
	uint MaxCandidateClusters;
	uint RenderFlags;
	uint DebugFlags;
	uint StartPageIndex;
};

FDecodeInfo DecodeInfo : register(b0);

ByteAddressBuffer ClusterPageData : register(t0);
ByteAddressBuffer HierarchyBuffer : register(t1);
ByteAddressBuffer VisibleClustersSWHW : register(t2);
StructuredBuffer< FPackedNaniteView > InViews : register(t3);
//StructuredBuffer<uint> RayTracingDataBuffer;
//#endif


struct FQueuePassState
{
	uint	ClusterBatchReadOffset;	// Offset in batches
	uint	ClusterWriteOffset;		// Offset in individual clusters
	uint	NodeReadOffset;
	uint	NodeWriteOffset;
	uint	NodePrevWriteOffset;	// Helper used by non-persistent culling
	int		NodeCount;				// Can temporarily be conservatively higher
};

struct FQueueState
{
	uint			TotalClusters;
	FQueuePassState PassState[2];
};


uint GetMaxClusterBatches() { return DecodeInfo.MaxCandidateClusters / NANITE_PERSISTENT_CLUSTER_CULLING_GROUP_SIZE; }



uint4 PackVisibleClusterInstance(FVisibleClusterInstance VisibleClusterInstance, bool bHasPageData)
{
	// Since cluster instance bit is 32 (will be 11 in future), use a byte more
	uint4 RawData = 0;
	uint BitPos = 0;
	WriteBits(RawData, BitPos, VisibleClusterInstance.Flags,		NANITE_NUM_CULLING_FLAG_BITS);
	WriteBits(RawData, BitPos, VisibleClusterInstance.ViewId,		NANITE_MAX_VIEWS_PER_CULL_RASTERIZE_PASS_BITS);
	WriteBits(RawData, BitPos, VisibleClusterInstance.PageIndex,	NANITE_MAX_GPU_PAGES_BITS);
	WriteBits(RawData, BitPos, VisibleClusterInstance.InstanceId,	NANITE_MAX_INSTANCES_BITS);
	WriteBits(RawData, BitPos, 0, 32 - NANITE_MAX_INSTANCES_BITS);	// Store 8 bit 0 for alignment
	WriteBits(RawData, BitPos, VisibleClusterInstance.ClusterInstanceIndex, NANITE_MAX_CLUSTER_INSTANCES_PER_PAGE_BITS);
	WriteBits(RawData, BitPos, 0, 32 - NANITE_MAX_CLUSTER_INSTANCES_PER_PAGE_BITS); // Store 21 bit 0 for alignment
	if (bHasPageData)
	{
		WriteBits(RawData, BitPos, VisibleClusterInstance.vPage.x, 13);
		WriteBits(RawData, BitPos, VisibleClusterInstance.vPage.y, 13);
		uint2 Delta = (VisibleClusterInstance.vPageEnd - VisibleClusterInstance.vPage) & 0x7;
		WriteBits(RawData, BitPos, Delta.x, 3);
		WriteBits(RawData, BitPos, Delta.y, 3);
	}
	return RawData;
}

FVisibleClusterInstance UnpackVisibleClusterInstance(uint4 RawData, bool bHasPageData = false)
{
	uint BitPos = 0;
	FVisibleClusterInstance VisibleClusterInstance;
	VisibleClusterInstance.Flags		= ReadBits( RawData, BitPos, NANITE_NUM_CULLING_FLAG_BITS );
	VisibleClusterInstance.ViewId		= ReadBits( RawData, BitPos, NANITE_MAX_VIEWS_PER_CULL_RASTERIZE_PASS_BITS );
	VisibleClusterInstance.PageIndex	= ReadBits( RawData, BitPos, NANITE_MAX_GPU_PAGES_BITS );
	VisibleClusterInstance.InstanceId	= ReadBits( RawData, BitPos, NANITE_MAX_INSTANCES_BITS );
	uint temp 							= ReadBits( RawData, BitPos, 32 - NANITE_MAX_INSTANCES_BITS );
	VisibleClusterInstance.ClusterInstanceIndex	= ReadBits( RawData, BitPos, NANITE_MAX_CLUSTER_INSTANCES_PER_PAGE_BITS );
	temp 								= ReadBits( RawData, BitPos, 32 - NANITE_MAX_CLUSTER_INSTANCES_PER_PAGE_BITS );
	if( bHasPageData )
	{
		VisibleClusterInstance.vPage.x	= ReadBits( RawData, BitPos, 13 );
		VisibleClusterInstance.vPage.y	= ReadBits( RawData, BitPos, 13 );
		VisibleClusterInstance.vPageEnd.x	= ReadBits( RawData, BitPos, 3 );
		VisibleClusterInstance.vPageEnd.y	= ReadBits( RawData, BitPos, 3 );
		VisibleClusterInstance.vPageEnd += VisibleClusterInstance.vPage;
	}
	else
	{
		VisibleClusterInstance.vPage = 0;
	}

	return VisibleClusterInstance;
}

FVisibleClusterInstance GetVisibleClusterInstance( ByteAddressBuffer VisibleClusters, uint ClusterInstanceIdx, bool bHasPageData = false )
{
	uint4 RawData;
	if( bHasPageData )
		RawData = VisibleClusters.Load4( ClusterInstanceIdx * 16 );
	else
		RawData = uint4( VisibleClusters.Load3( ClusterInstanceIdx * 12 ), 0 );

	return UnpackVisibleClusterInstance(RawData, bHasPageData);
}

FVisibleClusterInstance GetVisibleClusterInstance( uint ClusterInstanceIdx, bool bHasPageData )
{
#if NANITE_USE_RAYTRACING_UNIFORM_BUFFER
	return (FVisibleClusterInstance)0;
#else
	return GetVisibleClusterInstance( VisibleClustersSWHW, ClusterInstanceIdx, bHasPageData );
#endif
}

bool IsVisibleClusterIndexImposter(uint ClusterIndex)
{
	return ClusterIndex >= (1 << 24);
}

FVisibleClusterInstance GetVisibleClusterInstance( uint ClusterInstanceIndex )
{
	FVisibleClusterInstance VisibleClusterInstance;

	if( IsVisibleClusterIndexImposter(ClusterInstanceIndex) )
	{
		// Couldn't have been stored so signals this is an imposter
		VisibleClusterInstance.Flags = 1 << NANITE_NUM_CULLING_FLAG_BITS;
		VisibleClusterInstance.ViewId = 0;	// TODO
		VisibleClusterInstance.InstanceId = BitFieldExtractU32( ClusterInstanceIndex, NANITE_MAX_INSTANCES_BITS - 1, 1 );
		VisibleClusterInstance.PageIndex = 0;
		VisibleClusterInstance.ClusterInstanceIndex = ClusterInstanceIndex & 1;
	}
	else
	{
		VisibleClusterInstance = GetVisibleClusterInstance( ClusterInstanceIndex, false );
	}

	return VisibleClusterInstance;
}

//FInstanceSceneData GetInstanceSceneData( inout FVisibleCluster VisibleCluster, bool bCheckValid = true )
//{
//	FInstanceSceneData InstanceData = GetInstanceSceneData( VisibleCluster.InstanceId, PageConstants.x, bCheckValid );
//
//	// Couldn't have been stored so signals this is an imposter
//	if( VisibleCluster.Flags == (1 << NANITE_NUM_CULLING_FLAG_BITS) )
//	{
//		const uint MaxStreamingPages = 1 << 12;
//		VisibleCluster.PageIndex = MaxStreamingPages + (InstanceData.NaniteRuntimeResourceID & NANITE_MAX_GPU_PAGES_MASK);
//	}
//
//	return InstanceData;
//}

//FInstanceDynamicData CalculateInstanceDynamicData( FNaniteView NaniteView, FInstanceSceneData InstanceData )
//{
//	float4x4 LocalToTranslatedWorld = LWCMultiplyTranslation(InstanceData.LocalToWorld, NaniteView.PreViewTranslation);
//	float4x4 PrevLocalToTranslatedWorld = LWCMultiplyTranslation(InstanceData.PrevLocalToWorld, NaniteView.PrevPreViewTranslation);
//
//	FInstanceDynamicData DynamicData;
//	DynamicData.LocalToTranslatedWorld = LocalToTranslatedWorld;
//	DynamicData.PrevLocalToTranslatedWorld = PrevLocalToTranslatedWorld;
//	DynamicData.bHasMoved = GetGPUSceneFrameNumber() == InstanceData.LastUpdateSceneFrameNumber;
//
//	return DynamicData;
//}
//
//FInstanceSceneData GetInstanceSceneData( uint InstanceId, bool bCheckValid = true )
//{
//	return GetInstanceSceneData( InstanceId, PageConstants.x, bCheckValid );
//}

FCluster UnpackCluster(uint4 ClusterData[NANITE_NUM_PACKED_CLUSTER_FLOAT4S])
{
	FCluster Cluster;
	Cluster.PageBaseAddress		= 0;

	Cluster.NumVerts			= BitFieldExtractU32(ClusterData[0].x, 9, 0);
	Cluster.PositionOffset		= BitFieldExtractU32(ClusterData[0].x, 23, 9);
	Cluster.NumTris				= BitFieldExtractU32(ClusterData[0].y, 8, 0);
	Cluster.IndexOffset			= BitFieldExtractU32(ClusterData[0].y, 24, 8);

	Cluster.ColorMin			= ClusterData[0].z;
	Cluster.ColorBits			= BitFieldExtractU32(ClusterData[0].w, 16, 0);
	//Cluster.GroupIndex			= BitFieldExtractU32(ClusterData[0].w, 16, 16);			// Debug only

	Cluster.PosStart			= ClusterData[1].xyz;
	Cluster.BitsPerIndex		= BitFieldExtractU32(ClusterData[1].w, 4, 0);
	Cluster.PosPrecision		= (int)BitFieldExtractU32(ClusterData[1].w, 5, 4) + NANITE_MIN_POSITION_PRECISION;
	Cluster.PosBits.x			= BitFieldExtractU32(ClusterData[1].w, 5, 9);
	Cluster.PosBits.y			= BitFieldExtractU32(ClusterData[1].w, 5, 14);
	Cluster.PosBits.z			= BitFieldExtractU32(ClusterData[1].w, 5, 19);

	// TiX: moved to Cluster Isntance
	//Cluster.LODBounds			= asfloat(ClusterData[2]);

	////Cluster.BoxBoundsCenter		= asfloat(ClusterData[3].xyz);
	//// tix : BoxBoundsCenter is BoxBoundsCenter16_MipLevel; decode it
	// Cluster.BoxBoundsCenter.x 	= f16tof32(ClusterData[3].x >> 16);
	// Cluster.BoxBoundsCenter.y 	= f16tof32(ClusterData[3].x & 0xffffu);
	// Cluster.BoxBoundsCenter.z 	= f16tof32(ClusterData[3].y >> 16);
	// Cluster.MipLevel			= ClusterData[3].y & 0xffffu;
	
	// Cluster.LODError			= f16tof32(ClusterData[3].w);
	// Cluster.EdgeLength			= f16tof32(ClusterData[3].w >> 16);

	// Cluster.BoxBoundsExtent		= asfloat(ClusterData[4].xyz);
	// Cluster.Flags				= ClusterData[4].w;

	Cluster.AttributeOffset		= BitFieldExtractU32(ClusterData[2].x, 22,  0);
	Cluster.BitsPerAttribute	= BitFieldExtractU32(ClusterData[2].x, 10, 22);
	Cluster.DecodeInfoOffset	= BitFieldExtractU32(ClusterData[2].y, 22,  0);
	Cluster.NumUVs				= BitFieldExtractU32(ClusterData[2].y,  3, 22);
	Cluster.ColorMode			= BitFieldExtractU32(ClusterData[2].y,  2, 22+3);
	Cluster.UV_Prec				= ClusterData[2].z;
	const uint MaterialEncoding = ClusterData[2].w;

	// Material Table Range Encoding (32 bits)
	// uint TriStart        :  8;  // max 128 triangles
	// uint TriLength       :  8;  // max 128 triangles
	// uint MaterialIndex   :  6;  // max  64 materials
	// uint Padding         : 10;

	// Material Packed Range - Fast Path (32 bits)
	// uint Material0Index  : 6;  // max  64 materials (0:Material0Length)
	// uint Material1Index  : 6;  // max  64 materials (Material0Length:Material1Length)
	// uint Material2Index  : 6;  // max  64 materials (remainder)
	// uint Material0Length : 7;  // max 128 triangles (num minus one)
	// uint Material1Length : 7;  // max  64 triangles (materials are sorted, so at most 128/2)

	// Material Packed Range - Slow Path (32 bits)
	// uint BufferIndex     : 19; // 2^19 max value (tons, it's per prim)
	// uint BufferLength    : 6;  // max 64 ranges (num minus one)
	// uint Padding         : 7;  // always 127 for slow path. corresponds to Material1Length=127 in fast path

	// TiX: Ignore Material for now.
	// [branch]
	// if (MaterialEncoding < 0xFE000000u)
	// {
	// 	// Fast inline path
	// 	Cluster.MaterialTableOffset	= 0;
	// 	Cluster.MaterialTableLength	= 0;		
	// 	Cluster.Material0Index		= BitFieldExtractU32(MaterialEncoding, 6, 0);
	// 	Cluster.Material1Index		= BitFieldExtractU32(MaterialEncoding, 6, 6);
	// 	Cluster.Material2Index		= BitFieldExtractU32(MaterialEncoding, 6, 12);
	// 	Cluster.Material0Length		= BitFieldExtractU32(MaterialEncoding, 7, 18) + 1;
	// 	Cluster.Material1Length		= BitFieldExtractU32(MaterialEncoding, 7, 25);

	// 	Cluster.VertReuseBatchCountTableOffset = 0;
	// 	Cluster.VertReuseBatchCountTableSize = 0;
	// 	Cluster.VertReuseBatchInfo	= ClusterData[6];
	// }
	// else
	// {
	// 	// Slow global search path
	// 	Cluster.MaterialTableOffset = BitFieldExtractU32(MaterialEncoding, 19, 0);
	// 	Cluster.MaterialTableLength	= BitFieldExtractU32(MaterialEncoding, 6, 19) + 1;
	// 	Cluster.Material0Index		= 0;
	// 	Cluster.Material1Index		= 0;
	// 	Cluster.Material2Index		= 0;
	// 	Cluster.Material0Length		= 0;
	// 	Cluster.Material1Length		= 0;

	// 	Cluster.VertReuseBatchCountTableOffset = ClusterData[6].x;
	// 	Cluster.VertReuseBatchCountTableSize = ClusterData[6].y;
	// 	Cluster.VertReuseBatchInfo = 0;
	// }

	return Cluster;
}

FClusterInstance UnpackClusterInstance(uint4 ClusterInstanceData[NANITE_NUM_PACKED_CLUSTER_INSTANCE_FLOAT4S])
{
	FClusterInstance ClusterInstance;

	ClusterInstance.LODBounds			= asfloat(ClusterInstanceData[0]);

	//ClusterInstance.BoxBoundsCenter		= asfloat(ClusterInstanceData[3].xyz);
	// tix : BoxBoundsCenter is BoxBoundsCenter16_MipLevel; decode it
	ClusterInstance.BoxBoundsCenter.x 	= f16tof32(ClusterInstanceData[1].x >> 16);
	ClusterInstance.BoxBoundsCenter.y 	= f16tof32(ClusterInstanceData[1].x & 0xffffu);
	ClusterInstance.BoxBoundsCenter.z 	= f16tof32(ClusterInstanceData[1].y >> 16);
	ClusterInstance.MipLevel			= ClusterInstanceData[1].y & 0xffffu;

	ClusterInstance.PageIndex			= ClusterInstanceData[1].z >> NANITE_MAX_CLUSTERS_PER_PAGE_BITS;
	ClusterInstance.ClusterIndex		= ClusterInstanceData[1].z & NANITE_MAX_CLUSTERS_PER_PAGE_MASK;
	
	ClusterInstance.LODError			= f16tof32(ClusterInstanceData[1].w & 0xffffu);
	ClusterInstance.EdgeLength			= f16tof32(ClusterInstanceData[1].w >> 16);

	ClusterInstance.BoxBoundsExtent		= asfloat(ClusterInstanceData[2].xyz);
	ClusterInstance.Flags				= ClusterInstanceData[2].w & 0xffffu;
	ClusterInstance.GroupIndex			= ClusterInstanceData[2].w >> 16;			// Debug only

	// tix TODO : load transform
	float4 t0 = asfloat(ClusterInstanceData[3]);
	float4 t1 = asfloat(ClusterInstanceData[4]);
	float4 t2 = asfloat(ClusterInstanceData[5]);
	ClusterInstance.Transform[0] = float4(t0.xyz, 0);
	ClusterInstance.Transform[1] = float4(t1.xyz, 0);
	ClusterInstance.Transform[2] = float4(t2.xyz, 0);
	ClusterInstance.Transform[3] = float4(t0.w, t1.w, t2.w, 1);

	return ClusterInstance;
}

uint GPUPageIndexToGPUOffset(uint PageIndex)
{
	const uint MaxStreamingPages = DecodeInfo.PageConstants.y;
	return (min(PageIndex, MaxStreamingPages) << NANITE_STREAMING_PAGE_GPU_SIZE_BITS) + ((uint)max((int)PageIndex - (int)MaxStreamingPages, 0) << NANITE_ROOT_PAGE_GPU_SIZE_BITS);
}

FPageHeader UnpackPageHeader(uint4 Data)
{
	FPageHeader Header;
	Header.NumClusters = Data.x;
	Header.NumClusterInstances = Data.y;
	return Header;
}

FPageHeader GetPageHeader(ByteAddressBuffer InputBuffer, uint PageAddress)
{
	return UnpackPageHeader(InputBuffer.Load4(PageAddress));
}

FPageHeader GetPageHeader(RWByteAddressBuffer InputBuffer, uint PageAddress)
{
	return UnpackPageHeader(InputBuffer.Load4(PageAddress));
}

FCluster GetCluster(ByteAddressBuffer InputBuffer, uint SrcBaseOffset, uint ClusterIndex, uint NumPageClusters)
{
	const uint ClusterSOAStride = ( NumPageClusters << 4 );
	const uint ClusterBaseAddress = SrcBaseOffset + ( ClusterIndex << 4 );
	
	uint4 ClusterData[NANITE_NUM_PACKED_CLUSTER_FLOAT4S];
	[unroll]
	for(int i = 0; i < NANITE_NUM_PACKED_CLUSTER_FLOAT4S; i++)
	{
		ClusterData[i] = InputBuffer.Load4( ClusterBaseAddress + i * ClusterSOAStride + NANITE_GPU_PAGE_HEADER_SIZE ); // Adding NANITE_GPU_PAGE_HEADER_SIZE inside the loop prevents compiler confusion about offset modifier and generates better code
	}
	
	return UnpackCluster(ClusterData);
}

FCluster GetCluster(RWByteAddressBuffer InputBuffer, uint SrcBaseOffset, uint ClusterIndex, uint NumPageClusters)
{
	const uint ClusterSOAStride = (NumPageClusters << 4);
	const uint ClusterBaseAddress = SrcBaseOffset + (ClusterIndex << 4);

	uint4 ClusterData[NANITE_NUM_PACKED_CLUSTER_FLOAT4S];
	[unroll]
	for (int i = 0; i < NANITE_NUM_PACKED_CLUSTER_FLOAT4S; i++)
	{
		ClusterData[i] = InputBuffer.Load4( ClusterBaseAddress + i * ClusterSOAStride + NANITE_GPU_PAGE_HEADER_SIZE );  // Adding NANITE_GPU_PAGE_HEADER_SIZE inside the loop prevents compiler confusion about offset modifier and generates better code
	}
	return UnpackCluster(ClusterData);
}

FCluster GetCluster(uint PageIndex, uint ClusterIndex)
{
	uint PageBaseAddress = GPUPageIndexToGPUOffset(PageIndex);
	FPageHeader Header = GetPageHeader(ClusterPageData, PageBaseAddress);
	FCluster Cluster = GetCluster(ClusterPageData, PageBaseAddress, ClusterIndex, Header.NumClusters);
	Cluster.PageBaseAddress = PageBaseAddress;
	return Cluster;
}

FClusterInstance GetClusterInstance(ByteAddressBuffer InputBuffer, uint SrcBaseOffset, uint ClusterInstanceIndex, uint NumPageClusterInstances, uint NumPageClusters)
{
	const uint ClusterInstanceSOAStride = (NumPageClusterInstances << 4);
	const uint ClusterInstanceBaseAddress = SrcBaseOffset + (ClusterInstanceIndex << 4) + NumPageClusters * NANITE_NUM_PACKED_CLUSTER_FLOAT4S * 16;

	uint4 ClusterInstanceData[NANITE_NUM_PACKED_CLUSTER_INSTANCE_FLOAT4S];
	[unroll]
	for (int i = 0; i < NANITE_NUM_PACKED_CLUSTER_INSTANCE_FLOAT4S; i++)
	{
		ClusterInstanceData[i] = InputBuffer.Load4( ClusterInstanceBaseAddress + i * ClusterInstanceSOAStride + NANITE_GPU_PAGE_HEADER_SIZE );  // Adding NANITE_GPU_PAGE_HEADER_SIZE inside the loop prevents compiler confusion about offset modifier and generates better code
	}
	return UnpackClusterInstance(ClusterInstanceData);
}

FClusterInstance GetClusterInstance(RWByteAddressBuffer InputBuffer, uint SrcBaseOffset, uint ClusterInstanceIndex, uint NumPageClusterInstances, uint NumPageClusters)
{
	const uint ClusterInstanceSOAStride = (NumPageClusterInstances << 4);
	const uint ClusterInstanceBaseAddress = SrcBaseOffset + (ClusterInstanceIndex << 4) + NumPageClusters * NANITE_NUM_PACKED_CLUSTER_FLOAT4S * 16;

	uint4 ClusterInstanceData[NANITE_NUM_PACKED_CLUSTER_INSTANCE_FLOAT4S];
	[unroll]
	for (int i = 0; i < NANITE_NUM_PACKED_CLUSTER_INSTANCE_FLOAT4S; i++)
	{
		ClusterInstanceData[i] = InputBuffer.Load4( ClusterInstanceBaseAddress + i * ClusterInstanceSOAStride + NANITE_GPU_PAGE_HEADER_SIZE );  // Adding NANITE_GPU_PAGE_HEADER_SIZE inside the loop prevents compiler confusion about offset modifier and generates better code
	}
	return UnpackClusterInstance(ClusterInstanceData);
}

FClusterInstance GetClusterInstance(uint PageIndex, uint ClusterInstanceIndex)
{
	uint PageBaseAddress = GPUPageIndexToGPUOffset(PageIndex);
	FPageHeader Header = GetPageHeader(ClusterPageData, PageBaseAddress);
	FClusterInstance ClusterInstance = GetClusterInstance(ClusterPageData, PageBaseAddress, ClusterInstanceIndex, Header.NumClusterInstances, Header.NumClusters);
	//ClusterInstance.PageBaseAddress = PageBaseAddress;
	return ClusterInstance;
}

FHierarchyNodeSlice GetHierarchyNodeSlice(uint NodeIndex, uint ChildIndex)
{
	const uint NodeSize = (4 + 4 + 4 + 1) * 4 * NANITE_MAX_BVH_NODE_FANOUT;

	uint BaseAddress = NodeIndex * NodeSize;

	FHierarchyNodeSlice Node;
	Node.LODBounds = asfloat(HierarchyBuffer.Load4(BaseAddress + 16 * ChildIndex));

	uint4 Misc0 = HierarchyBuffer.Load4(BaseAddress + (NANITE_MAX_BVH_NODE_FANOUT * 16) + 16 * ChildIndex);
	uint4 Misc1 = HierarchyBuffer.Load4(BaseAddress + (NANITE_MAX_BVH_NODE_FANOUT * 32) + 16 * ChildIndex);
	uint  Misc2 = HierarchyBuffer.Load( BaseAddress + (NANITE_MAX_BVH_NODE_FANOUT * 48) + 4 * ChildIndex);
	Node.BoxBoundsCenter = asfloat(Misc0.xyz);
	Node.BoxBoundsExtent = asfloat(Misc1.xyz);

	Node.MinLODError = f16tof32(Misc0.w);
	Node.MaxParentLODError = f16tof32(Misc0.w >> 16);
	Node.ChildStartReference = Misc1.w;
	Node.bLoaded = Misc1.w != 0xFFFFFFFFu;

	uint ResourcePageIndex_NumPages_GroupPartSize = Misc2;
	Node.NumChildren = BitFieldExtractU32(ResourcePageIndex_NumPages_GroupPartSize, NANITE_MAX_CLUSTERS_PER_GROUP_BITS, 0);
	Node.NumPages = BitFieldExtractU32(ResourcePageIndex_NumPages_GroupPartSize, NANITE_MAX_GROUP_PARTS_BITS, NANITE_MAX_CLUSTERS_PER_GROUP_BITS);
	Node.StartPageIndex = BitFieldExtractU32(ResourcePageIndex_NumPages_GroupPartSize, NANITE_MAX_RESOURCE_PAGES_BITS, NANITE_MAX_CLUSTERS_PER_GROUP_BITS + NANITE_MAX_GROUP_PARTS_BITS);
	Node.bEnabled = ResourcePageIndex_NumPages_GroupPartSize != 0u;
	Node.bLeaf = ResourcePageIndex_NumPages_GroupPartSize != 0xFFFFFFFFu;

	return Node;
}

// Decode triangle that is represented by one base index and two 5-bit offsets.
uint3 ReadTriangleIndices(FCluster Cluster, uint TriIndex)
{
	const uint BitsPerTriangle = Cluster.BitsPerIndex + 2 * 5;

	FBitStreamReaderState BitStreamReader = BitStreamReader_Create_Aligned(Cluster.PageBaseAddress + Cluster.IndexOffset, TriIndex * BitsPerTriangle, 8 + 2*5);

	uint BaseIndex = BitStreamReader_Read_RO(ClusterPageData, BitStreamReader, Cluster.BitsPerIndex, 8);
	uint Delta0 = BitStreamReader_Read_RO(ClusterPageData, BitStreamReader, 5, 5);
	uint Delta1 = BitStreamReader_Read_RO(ClusterPageData, BitStreamReader, 5, 5);

	return BaseIndex + uint3(0, Delta0, Delta1);
}

uint PackMaterialResolve(
	uint MaterialSlot,
	bool IsDecalReceiver)
{
	uint Packed = 0x1; // Is Nanite
	Packed |= (BitFieldMaskU32(14, 1) & (MaterialSlot << 1u));
	Packed |= CondMask(IsDecalReceiver, 1u << 15u, 0u);
	return Packed;
}

void UnpackMaterialResolve(
	uint Packed,
	out bool IsNanitePixel,
	out bool IsDecalReceiver,
	out uint MaterialSlot)
{
	IsNanitePixel   = BitFieldExtractU32(Packed,  1,  0) != 0;
	MaterialSlot    = BitFieldExtractU32(Packed, 14,  1);
	IsDecalReceiver = BitFieldExtractU32(Packed,  1, 15) != 0;
}

void UnpackVisPixel(
	UlongType Pixel,
	out uint DepthInt,
	out uint VisibleClusterIndex, 
	out uint TriIndex
	)
{
	const uint2 Unpacked = UnpackUlongType(Pixel);
	VisibleClusterIndex = Unpacked.x >> 7;
	TriIndex = Unpacked.x & 0x7F;
	DepthInt = Unpacked.y;

	VisibleClusterIndex--;
}

void UnpackVisPixel(
	UlongType Pixel,
	out uint DepthInt,
	out uint VisibleClusterIndex, 
	out uint TriIndex,
	out bool bIsImposter
	)
{
	const uint2 Unpacked = UnpackUlongType(Pixel);
	VisibleClusterIndex = Unpacked.x >> 7;
	TriIndex = Unpacked.x & 0x7F;
	DepthInt = Unpacked.y;
	bIsImposter = (Unpacked.x >> 31);

	VisibleClusterIndex--;
}

void UnpackDbgPixel(
	UlongType Pixel,
	out uint DepthInt,
	out uint DebugValue
	)
{
	const uint2 Unpacked = UnpackUlongType(Pixel);
	DebugValue = Unpacked.x;
	DepthInt = Unpacked.y;
}

float3 UnpackPosition(uint2 Packed, FCluster Cluster)
{
	int3 Pos;
	Pos.x = BitFieldExtractU32(Packed.x, Cluster.PosBits.x, 0);

	Packed.x = BitAlignU32(Packed.y, Packed.x, Cluster.PosBits.x);
	Packed.y >>= Cluster.PosBits.x;
	Pos.y = BitFieldExtractU32(Packed.x, Cluster.PosBits.y, 0);

	Packed.x = BitAlignU32(Packed.y, Packed.x, Cluster.PosBits.y);
	Pos.z = BitFieldExtractU32(Packed.x, Cluster.PosBits.z, 0);

	const float Scale = asfloat(asint(1.0f) - (Cluster.PosPrecision << 23));
	return (Pos + Cluster.PosStart) * Scale;
}

uint2 GetPackedPosition(uint VertIndex, FCluster Cluster)
{
	const uint BitsPerVertex = Cluster.PosBits.x + Cluster.PosBits.y + Cluster.PosBits.z;
	const uint BitOffset = VertIndex * BitsPerVertex;	// TODO: Use Mul24
	uint3 Data = ClusterPageData.Load3(Cluster.PageBaseAddress + Cluster.PositionOffset + ((BitOffset >> 5) << 2));
	return uint2(BitAlignU32(Data.y, Data.x, BitOffset), BitAlignU32(Data.z, Data.y, BitOffset));
}

float3 DecodePosition(uint VertIndex, FCluster Cluster)
{
#if NANITE_USE_UNCOMPRESSED_VERTEX_DATA
	return asfloat(ClusterPageData.Load3(Cluster.PageBaseAddress + Cluster.PositionOffset + VertIndex * 12));
#else
	const uint2 PackedPos = GetPackedPosition(VertIndex, Cluster);
	return UnpackPosition(PackedPos, Cluster);
#endif
}

FNaniteView UnpackNaniteView(FPackedNaniteView PackedView)
{
	FNaniteView NaniteView;
	
	//NaniteView.SVPositionToTranslatedWorld	= PackedView.SVPositionToTranslatedWorld;
	//NaniteView.ViewToTranslatedWorld		= PackedView.ViewToTranslatedWorld;
	//NaniteView.ViewTilePosition				= PackedView.ViewTilePosition;
	//NaniteView.MatrixTilePosition			= PackedView.MatrixTilePosition;

	//NaniteView.TranslatedWorldToView		= PackedView.TranslatedWorldToView;
	NaniteView.TranslatedWorldToClip		= PackedView.TranslatedWorldToClip;
	//NaniteView.TranslatedWorldToSubpixelClip= PackedView.TranslatedWorldToSubpixelClip;
	NaniteView.ViewToClip					= PackedView.ViewToClip;
	// -- NaniteView.ClipToWorld					= MakeLWCMatrix(PackedView.MatrixTilePosition, PackedView.ClipToRelativeWorld);
	//NaniteView.ClipToWorld					= PackedView.ClipToRelativeWorld;
	
	//NaniteView.PrevTranslatedWorldToView	= PackedView.PrevTranslatedWorldToView;
	//NaniteView.PrevTranslatedWorldToClip	= PackedView.PrevTranslatedWorldToClip;
	//NaniteView.PrevViewToClip				= PackedView.PrevViewToClip;
	// -- NaniteView.PrevClipToWorld				= MakeLWCMatrix(PackedView.MatrixTilePosition, PackedView.PrevClipToRelativeWorld);
	//NaniteView.PrevClipToWorld				= PackedView.PrevClipToRelativeWorld;


	NaniteView.ViewRect						= PackedView.ViewRect;
	NaniteView.ViewSizeAndInvSize			= PackedView.ViewSizeAndInvSize;
	//NaniteView.ClipSpaceScaleOffset			= PackedView.ClipSpaceScaleOffset;
	// -- NaniteView.PreViewTranslation			= MakeLWCVector3(-PackedView.ViewTilePosition, PackedView.PreViewTranslation.xyz);
	// -- NaniteView.PrevPreViewTranslation		= MakeLWCVector3(-PackedView.ViewTilePosition, PackedView.PrevPreViewTranslation.xyz);
	// -- NaniteView.WorldCameraOrigin			= MakeLWCVector3(PackedView.ViewTilePosition, PackedView.WorldCameraOrigin.xyz);
	
	//NaniteView.PreViewTranslation			= PackedView.PreViewTranslation.xyz;
	//NaniteView.PrevPreViewTranslation		= PackedView.PrevPreViewTranslation.xyz;
	NaniteView.WorldCameraOrigin			= PackedView.WorldCameraOrigin;
	NaniteView.ViewForward					= PackedView.ViewForwardAndNearPlane.xyz;
	NaniteView.NearPlane					= PackedView.ViewForwardAndNearPlane.w;
	NaniteView.LODScale						= PackedView.LODScales.x;
	NaniteView.LODScaleHW					= PackedView.LODScales.y;
	//NaniteView.MinBoundsRadiusSq			= PackedView.MinBoundsRadiusSq;
	NaniteView.StreamingPriorityCategory	= PackedView.StreamingPriorityCategory_AndFlags & NANITE_STREAMING_PRIORITY_CATEGORY_MASK;
	NaniteView.Flags						= PackedView.StreamingPriorityCategory_AndFlags >> NANITE_NUM_STREAMING_PRIORITY_CATEGORY_BITS;
	
	//NaniteView.TargetLayerIndex				= PackedView.TargetLayerIdX_AndMipLevelY_AndNumMipLevelsZ.x;
	//NaniteView.TargetMipLevel				= PackedView.TargetLayerIdX_AndMipLevelY_AndNumMipLevelsZ.y;
	//NaniteView.TargetNumMipLevels			= PackedView.TargetLayerIdX_AndMipLevelY_AndNumMipLevelsZ.z;
	//NaniteView.TargetPrevLayerIndex			= PackedView.TargetLayerIdX_AndMipLevelY_AndNumMipLevelsZ.w;
	//NaniteView.RangeBasedCullingDistance	= PackedView.RangeBasedCullingDistance;

	//NaniteView.HZBTestViewRect				= PackedView.HZBTestViewRect;

	return NaniteView;
}

FNaniteView GetNaniteView( uint ViewIndex )
{
// #if NANITE_USE_VIEW_UNIFORM_BUFFER
// 	ViewState LocalView = GetPrimaryView();
// 	FNaniteView NaniteView;

// 	NaniteView.SVPositionToTranslatedWorld  = LocalView.SVPositionToTranslatedWorld;
// 	NaniteView.ViewToTranslatedWorld		= LocalView.ViewToTranslatedWorld;
// 	NaniteView.ViewTilePosition				= LocalView.ViewTilePosition;
// 	NaniteView.MatrixTilePosition			= LocalView.MatrixTilePosition;

// 	NaniteView.TranslatedWorldToView		= LocalView.TranslatedWorldToView;
// 	NaniteView.TranslatedWorldToClip		= LocalView.TranslatedWorldToClip;
// 	NaniteView.TranslatedWorldToSubpixelClip= LocalView.TranslatedWorldToSubpixelClip;
// 	NaniteView.ViewToClip					= LocalView.ViewToClip;
// 	NaniteView.ClipToWorld					= LocalView.ClipToWorld;
	
// 	NaniteView.PrevTranslatedWorldToView	= LocalView.PrevTranslatedWorldToView;
// 	NaniteView.PrevTranslatedWorldToClip	= LocalView.PrevTranslatedWorldToClip;
// 	NaniteView.PrevViewToClip				= LocalView.PrevViewToClip;
// 	NaniteView.PrevClipToWorld				= LocalView.PrevClipToWorld;

// 	NaniteView.ViewSizeAndInvSize			= LocalView.ViewSizeAndInvSize;
// 	NaniteView.ViewRect						= int4(int2(LocalView.ViewRectMin.xy + 0.5f), int2(LocalView.ViewRectMin.xy + LocalView.ViewSizeAndInvSize.xy + 0.5f));
// 	NaniteView.PreViewTranslation			= LocalView.PreViewTranslation;
// 	NaniteView.PrevPreViewTranslation		= LocalView.PrevPreViewTranslation;
// 	NaniteView.WorldCameraOrigin			= LocalView.WorldCameraOrigin;
// 	NaniteView.ViewForward					= LocalView.ViewForward;
// 	NaniteView.NearPlane					= LocalView.NearPlane;
// 	NaniteView.LODScale						= 1.0f;
// 	NaniteView.LODScaleHW					= 1.0f;
// 	NaniteView.MinBoundsRadiusSq			= 0;
// 	NaniteView.StreamingPriorityCategory	= 3;
// 	NaniteView.Flags						= NANITE_VIEW_FLAG_HZBTEST | NANITE_VIEW_FLAG_NEAR_CLIP;
	
// 	NaniteView.TargetLayerIndex = 0;
// 	NaniteView.TargetMipLevel = 0;
// 	NaniteView.TargetNumMipLevels = 0;
// 	NaniteView.TargetPrevLayerIndex	= 0;
// 	NaniteView.RangeBasedCullingDistance = 0.0f;

// 	NaniteView.HZBTestViewRect				= NaniteView.ViewRect;

// #else // !NANITE_USE_VIEW_UNIFORM_BUFFER

//#if NANITE_MULTI_VIEW
//	FPackedNaniteView PackedView = InViews[ViewIndex];
//#else
	FPackedNaniteView PackedView = InViews[0];
//#endif
	FNaniteView NaniteView = UnpackNaniteView(PackedView);

//#endif // NANITE_USE_VIEW_UNIFORM_BUFFER

	return NaniteView;
}

//// Fill ViewState using data from a NaniteView
//void PatchViewState(FNaniteView NaniteView, inout ViewState InOutView)
//{
//	InOutView.SVPositionToTranslatedWorld	= NaniteView.SVPositionToTranslatedWorld;
//	InOutView.ViewToTranslatedWorld			= NaniteView.ViewToTranslatedWorld;
//	InOutView.ViewTilePosition				= NaniteView.ViewTilePosition;
//	InOutView.MatrixTilePosition			= NaniteView.MatrixTilePosition;
//
//	InOutView.TranslatedWorldToView			= NaniteView.TranslatedWorldToView;
//	InOutView.TranslatedWorldToClip			= NaniteView.TranslatedWorldToClip;
//	InOutView.ViewToClip					= NaniteView.ViewToClip;
//	InOutView.ClipToWorld					= NaniteView.ClipToWorld;
//
//	InOutView.PrevTranslatedWorldToView		= NaniteView.PrevTranslatedWorldToView;
//	InOutView.PrevTranslatedWorldToClip		= NaniteView.PrevTranslatedWorldToClip;
//	InOutView.PrevViewToClip				= NaniteView.PrevViewToClip;
//	InOutView.PrevClipToWorld				= NaniteView.PrevClipToWorld;
//
//	InOutView.ViewSizeAndInvSize			= NaniteView.ViewSizeAndInvSize;
//	InOutView.ViewRectMin.xy				= NaniteView.ViewRect.xy - 0.5f; // Convert from float2 with a half texel offset to an int2 texel coord
//	InOutView.PreViewTranslation			= NaniteView.PreViewTranslation;
//	InOutView.PrevPreViewTranslation		= NaniteView.PrevPreViewTranslation;
//	InOutView.WorldCameraOrigin				= NaniteView.WorldCameraOrigin;
//	InOutView.ViewForward					= NaniteView.ViewForward;
//	InOutView.NearPlane						= NaniteView.NearPlane;
//}

void WriteDispatchArgsSWHW(RWBuffer<uint> RasterizerArgsSWHW, uint ArgsOffset, uint NumClustersSW, uint NumClustersHW)
{
	RasterizerArgsSWHW[ArgsOffset + 0] = (NumClustersSW + 63u) / 64u;			// SW: ThreadGroupCountX
	RasterizerArgsSWHW[ArgsOffset + 1] = 1;										// SW: ThreadGroupCountY
	RasterizerArgsSWHW[ArgsOffset + 2] = 1;										// SW: ThreadGroupCountZ
	RasterizerArgsSWHW[ArgsOffset + 3] = 0;										// padding

	RasterizerArgsSWHW[ArgsOffset + 4] = (NumClustersHW + 63u) / 64u;			// HW: ThreadGroupCountX
	RasterizerArgsSWHW[ArgsOffset + 5] = 1;										// HW: ThreadGroupCountY
	RasterizerArgsSWHW[ArgsOffset + 6] = 1;										// HW: ThreadGroupCountZ
	RasterizerArgsSWHW[ArgsOffset + 7] = 0;										// padding
}

void WriteRasterizerArgsSWHW(RWBuffer<uint> RasterizerArgsSWHW, uint ArgsOffset, uint NumClustersSW, uint NumClustersHW)
{
	RasterizerArgsSWHW[ArgsOffset + 0] = NumClustersSW;							// SW: ThreadGroupCountX
	RasterizerArgsSWHW[ArgsOffset + 1] = 1;										// SW: ThreadGroupCountY
	RasterizerArgsSWHW[ArgsOffset + 2] = 1;										// SW: ThreadGroupCountZ
	RasterizerArgsSWHW[ArgsOffset + 3] = 0;										// padding

	uint3 HWArgs;	// Assign to local before writing to RasterizerArgsSWHW to work around an FXC issue where the write to RasterizerArgsSWHW[ArgsOffset + 4] would be omitted
	if (DecodeInfo.RenderFlags & NANITE_RENDER_FLAG_MESH_SHADER)
	{
		HWArgs.x = NumClustersHW;						// HW: ThreadGroupCountX
		HWArgs.y = 1;									// HW: ThreadGroupCountY
		HWArgs.z = 1;									// HW: ThreadGroupCountZ
	}
	else if (DecodeInfo.RenderFlags & NANITE_RENDER_FLAG_PRIMITIVE_SHADER)
	{
		HWArgs.x = NumClustersHW;						// HW: VertexCountPerInstance
		HWArgs.y = 1;									// HW: InstanceCount
		HWArgs.z = 0;									// HW: StartVertexLocation
	}
	else
	{
		HWArgs.x = NANITE_MAX_CLUSTER_TRIANGLES * 3;	// HW: VertexCountPerInstance
		HWArgs.y = NumClustersHW;						// HW: InstanceCount
		HWArgs.z = 0;									// HW: StartVertexLocation
	}

	RasterizerArgsSWHW[ArgsOffset + 4] = HWArgs.x;
	RasterizerArgsSWHW[ArgsOffset + 5] = HWArgs.y;
	RasterizerArgsSWHW[ArgsOffset + 6] = HWArgs.z;
	RasterizerArgsSWHW[ArgsOffset + 7] = 0;				// HW: StartInstanceLocation
}