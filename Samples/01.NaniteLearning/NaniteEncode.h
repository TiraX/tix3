/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once
#include "Cluster.h"

class TNaniteMesh;


struct FClusterGroupPart					// Whole group or a part of a group that has been split.
{
	TVector<uint32> Clusters;				// Can be reordered during page allocation, so we need to store a list here.
	TVector<uint32> ClusterInstances;
	FBox GroupPartBounds;
	uint32 PageIndex;
	uint32 GroupIndex;				// Index of group this is a part of.
	uint32 HierarchyNodeIndex;
	uint32 HierarchyChildIndex;
	uint32 PageClusterOffset;
	uint32 PageClusterInstanceOffset;
};

struct FPageSections
{
	uint32 Cluster = 0;
	uint32 MaterialTable = 0;
	uint32 DecodeInfo = 0;
	uint32 Index = 0;
	uint32 Position = 0;
	uint32 Attribute = 0;
	uint32 ClusterInstance = 0;

	uint32 GetMaterialTableSize() const { return TMath::Align(MaterialTable, 16); }
	uint32 GetClusterOffset() const { return NANITE_GPU_PAGE_HEADER_SIZE; }
	uint32 GetMaterialTableOffset() const { return GetClusterOffset() + Cluster; }
	uint32 GetDecodeInfoOffset() const { return GetMaterialTableOffset() + GetMaterialTableSize(); }
	uint32 GetIndexOffset() const { return GetDecodeInfoOffset() + DecodeInfo; }
	uint32 GetPositionOffset() const { return GetIndexOffset() + Index; }
	uint32 GetAttributeOffset() const { return GetPositionOffset() + Position; }
	uint32 GetClusterInstanceOffset() const { return GetAttributeOffset() + Attribute; }
	uint32 GetTotal() const { return GetClusterInstanceOffset() + ClusterInstance; }

	FPageSections GetOffsets() const
	{
		return FPageSections{ GetClusterOffset(), GetMaterialTableOffset(), GetDecodeInfoOffset(), GetIndexOffset(), GetPositionOffset(), GetAttributeOffset(), GetClusterInstanceOffset() };
	}

	void operator+=(const FPageSections& Other)
	{
		Cluster += Other.Cluster;
		MaterialTable += Other.MaterialTable;
		DecodeInfo += Other.DecodeInfo;
		Index += Other.Index;
		Position += Other.Position;
		Attribute += Other.Attribute;
		ClusterInstance += Other.ClusterInstance;
	}
};

struct FPageGPUHeader
{
	uint32 NumClusters = 0;
	uint32 NumClusterInstances = 0;
	uint32 Pad[2] = { 0 };
};

struct FPageDiskHeader
{
	// only 8 uint32, since GetPageDiskHeader in hlsl read uint4 x 2.
	uint32 GpuSize;
	uint32 NumClustersAndInstances;
	uint32 NumRawFloat4s;
	uint32 NumTexCoords;
	uint32 NumVertexRefs;
	uint32 DecodeInfoOffset;
	uint32 StripBitmaskOffset;
	uint32 VertexRefBitmaskOffset;
};

struct FClusterDiskHeader
{
	uint32 IndexDataOffset;
	uint32 PageClusterMapOffset;
	uint32 VertexRefDataOffset;
	uint32 PositionDataOffset;
	uint32 AttributeDataOffset;
	uint32 NumVertexRefs;
	uint32 NumPrevRefVerticesBeforeDwords;
	uint32 NumPrevNewVerticesBeforeDwords;
};

struct FPage
{
	uint32	PartsStartIndex = 0;
	uint32	PartsNum = 0;
	uint32	NumClusters = 0;
	uint32	NumClusterInstances = 0;
	bool	bRelativeEncoding = false;

	FPageSections	GpuSizes;
};

// TODO: optimize me
struct FUVRange
{
	FInt2 Min;
	FInt2 GapStart;
	FInt2 GapLength;
	int32 Precision = 0;
	int32 Pad = 0;
};

struct FEncodingInfo
{
	uint32 BitsPerIndex;
	uint32 BitsPerAttribute;
	uint32 UVPrec;

	uint32 ColorMode;
	FInt4 ColorMin;
	FInt4 ColorBits;

	FPageSections GpuSizes;

	FUVRange UVRanges[NANITE_MAX_UVS];

	FEncodingInfo& operator = (const FEncodingInfo& Other)
	{
		BitsPerIndex = Other.BitsPerIndex;
		BitsPerAttribute = Other.BitsPerAttribute;
		UVPrec = Other.UVPrec;
		ColorMode = Other.ColorMode;
		ColorMin = Other.ColorMin;
		ColorBits = Other.ColorBits;
		GpuSizes = Other.GpuSizes;
		for (int32 i = 0; i < NANITE_MAX_UVS; i++)
		{
			UVRanges[i] = Other.UVRanges[i];
		}
		return *this;
	}
};

// Wasteful to store size for every vert but easier this way.
struct FVariableVertex
{
	const float* Data;
	uint32 SizeInBytes;

	bool operator==(const FVariableVertex& Other) const
	{
		return 0 == memcmp(Data, Other.Data, SizeInBytes);
	}

	bool operator<(const FVariableVertex& Other) const
	{
		return memcmp(Data, Other.Data, SizeInBytes) < 0;
	}
};

void Encode(
	TNaniteMesh& Mesh,
	TVector<FClusterGroup>& Groups, 
	TVector<FCluster>& ClusterSources,
	TVector< FClusterInstance>& ClusterInstances
);