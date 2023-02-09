/*
	TiX Engine v2.0 Copyright (C) 2018~2021
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

// Can not understand what's FixupChunk mean, try disable and reading further
#define FIXUP 0

struct FDecodeInfo
{
	FUInt4 PageConstants;
	uint32 MaxNodes;
	uint32 MaxVisibleClusters;
	uint32 MaxCandidateClusters;
	uint32 RenderFlags;
	uint32 DebugFlags;
	uint32 StartPageIndex;

	FDecodeInfo()
		: PageConstants(0, 0, 0, 0)
		, MaxNodes(0)
		, MaxVisibleClusters(0)
		, MaxCandidateClusters(0)
		, RenderFlags(0)
		, DebugFlags(0)
		, StartPageIndex(0)
	{}
};

struct FPackedHierarchyNode
{
	FFloat4 LODBounds[NANITE_MAX_BVH_NODE_FANOUT];

	struct
	{
		FFloat3 BoxBoundsCenter;
		uint32 MinLODError_MaxParentLODError;
	} Misc0[NANITE_MAX_BVH_NODE_FANOUT];

	struct
	{
		FFloat3 BoxBoundsExtent;
		uint32 ChildStartReference;
	} Misc1[NANITE_MAX_BVH_NODE_FANOUT];

	struct
	{
		uint32		ResourcePageIndex_NumPages_GroupPartSize;
	} Misc2[NANITE_MAX_BVH_NODE_FANOUT];
};


FORCEINLINE uint32 GetBits(uint32 Value, uint32 NumBits, uint32 Offset)
{
	uint32 Mask = (1u << NumBits) - 1u;
	return (Value >> Offset) & Mask;
}

FORCEINLINE void SetBits(uint32& Value, uint32 Bits, uint32 NumBits, uint32 Offset)
{
	uint32 Mask = (1u << NumBits) - 1u;
	TI_ASSERT(Bits <= Mask);
	Mask <<= Offset;
	Value = (Value & ~Mask) | (Bits << Offset);
}

// Packed Cluster as it is used by the GPU
struct FPackedCluster
{
	// Members needed for rasterization
	uint32 NumVerts_PositionOffset;					// NumVerts:9, PositionOffset:23
	uint32 NumTris_IndexOffset;						// NumTris:8, IndexOffset: 24
	uint32 ColorMin;
	uint32 ColorBits;						// R:4, G:4, B:4, A:4. (GroupIndex&0xFFFF) is for debug visualization only.

	FInt3 PosStart;
	uint32 BitsPerIndex_PosPrecision_PosBits;			// BitsPerIndex:4, PosPrecision: 5, PosBits:5.5.5

	// Members needed for culling, moved to cluster Instance
	//FFloat4 LODBounds;									// LWC_TODO: Was FSphere, but that's now twice as big and won't work on GPU.

	//FUInt2 BoxBoundsCenter16_MipLevel;
	//uint32 Reserved;
	//uint32 LODErrorAndEdgeLength;

	//FFloat3 BoxBoundsExtent;
	//uint32 Flags;

	// Members needed by materials
	uint32 AttributeOffset_BitsPerAttribute;			// AttributeOffset: 22, BitsPerAttribute: 10
	uint32 DecodeInfoOffset_NumUVs_ColorMode;			// DecodeInfoOffset: 22, NumUVs: 3, ColorMode: 2
	uint32 UV_Prec;									// U0:4, V0:4, U1:4, V1:4, U2:4, V2:4, U3:4, V3:4
	uint32 PackedMaterialInfo;
		   
	uint32 GetNumVerts() const { return GetBits(NumVerts_PositionOffset, 9, 0); }
	uint32 GetPositionOffset() const { return GetBits(NumVerts_PositionOffset, 23, 9); }
		   
	uint32 GetNumTris() const { return GetBits(NumTris_IndexOffset, 8, 0); }
	uint32 GetIndexOffset() const { return GetBits(NumTris_IndexOffset, 24, 8); }
		   
	uint32 GetBitsPerIndex() const { return GetBits(BitsPerIndex_PosPrecision_PosBits, 4, 0); }
	int32 GetPosPrecision() const { return (int32)GetBits(BitsPerIndex_PosPrecision_PosBits, 5, 4) + NANITE_MIN_POSITION_PRECISION; }
	uint32 GetPosBitsX() const { return GetBits(BitsPerIndex_PosPrecision_PosBits, 5, 9); }
	uint32 GetPosBitsY() const { return GetBits(BitsPerIndex_PosPrecision_PosBits, 5, 14); }
	uint32 GetPosBitsZ() const { return GetBits(BitsPerIndex_PosPrecision_PosBits, 5, 19); }
		   
	uint32 GetAttributeOffset() const { return GetBits(AttributeOffset_BitsPerAttribute, 22, 0); }
	uint32 GetBitsPerAttribute() const { return GetBits(AttributeOffset_BitsPerAttribute, 10, 22); }

	void SetNumVerts(uint32 NumVerts) { SetBits(NumVerts_PositionOffset, NumVerts, 9, 0); }
	void SetPositionOffset(uint32 Offset) { SetBits(NumVerts_PositionOffset, Offset, 23, 9); }
		 
	void SetNumTris(uint32 NumTris) { SetBits(NumTris_IndexOffset, NumTris, 8, 0); }
	void SetIndexOffset(uint32 Offset) { SetBits(NumTris_IndexOffset, Offset, 24, 8); }
		 
	void SetBitsPerIndex(uint32 BitsPerIndex) { SetBits(BitsPerIndex_PosPrecision_PosBits, BitsPerIndex, 4, 0); }
	void SetPosPrecision(int32 Precision) { SetBits(BitsPerIndex_PosPrecision_PosBits, uint32(Precision - NANITE_MIN_POSITION_PRECISION), 5, 4); }
	void SetPosBitsX(uint32 NumBits) { SetBits(BitsPerIndex_PosPrecision_PosBits, NumBits, 5, 9); }
	void SetPosBitsY(uint32 NumBits) { SetBits(BitsPerIndex_PosPrecision_PosBits, NumBits, 5, 14); }
	void SetPosBitsZ(uint32 NumBits) { SetBits(BitsPerIndex_PosPrecision_PosBits, NumBits, 5, 19); }
		 
	void SetAttributeOffset(uint32 Offset) { SetBits(AttributeOffset_BitsPerAttribute, Offset, 22, 0); }
	void SetBitsPerAttribute(uint32 Bits) { SetBits(AttributeOffset_BitsPerAttribute, Bits, 10, 22); }
		 
	void SetDecodeInfoOffset(uint32 Offset) { SetBits(DecodeInfoOffset_NumUVs_ColorMode, Offset, 22, 0); }
	void SetNumUVs(uint32 Num) { SetBits(DecodeInfoOffset_NumUVs_ColorMode, Num, 3, 22); }
	void SetColorMode(uint32 Mode) { SetBits(DecodeInfoOffset_NumUVs_ColorMode, Mode, 2, 22 + 3); }
		 
	void SetColorBitsR(uint32 NumBits) { SetBits(ColorBits, NumBits, 4, 0); }
	void SetColorBitsG(uint32 NumBits) { SetBits(ColorBits, NumBits, 4, 4); }
	void SetColorBitsB(uint32 NumBits) { SetBits(ColorBits, NumBits, 4, 8); }
	void SetColorBitsA(uint32 NumBits) { SetBits(ColorBits, NumBits, 4, 12); }
		 
	//void SetGroupIndex(uint32 GroupIndex) { SetBits(ColorBits_GroupIndex, GroupIndex & 0xFFFFu, 16, 16); }
};

struct FPackedClusterInstance
{
	// Members needed for culling
	FFloat4 LODBounds;									// LWC_TODO: Was FSphere, but that's now twice as big and won't work on GPU.

	FUInt2 BoxBoundsCenter16_MipLevel;
	uint32 PageAndCluster;
	uint32 LODErrorAndEdgeLength;

	FFloat3 BoxBoundsExtent;
	uint32 Flags_GroupIndex;

	FMat34 Transform;
};

struct FPageStreamingState
{
	uint32			BulkOffset;
	uint32			BulkSize;
	uint32			PageSize;
	uint32			DependenciesStart;
	uint32			DependenciesNum;
	uint32			Flags;
};

class FHierarchyFixup
{
public:
	FHierarchyFixup() {}

	FHierarchyFixup(uint32 InPageIndex, uint32 NodeIndex, uint32 ChildIndex, uint32 InClusterGroupPartStartIndex, uint32 PageDependencyStart, uint32 PageDependencyNum)
	{
		TI_ASSERT(InPageIndex < NANITE_MAX_RESOURCE_PAGES);
		PageIndex = InPageIndex;

		TI_ASSERT(NodeIndex < (1 << (32 - NANITE_MAX_HIERACHY_CHILDREN_BITS)));
		TI_ASSERT(ChildIndex < NANITE_MAX_HIERACHY_CHILDREN);
		TI_ASSERT(InClusterGroupPartStartIndex < (1 << (32 - NANITE_MAX_CLUSTERS_PER_GROUP_BITS)));
		HierarchyNodeAndChildIndex = (NodeIndex << NANITE_MAX_HIERACHY_CHILDREN_BITS) | ChildIndex;
		ClusterGroupPartStartIndex = InClusterGroupPartStartIndex;

		TI_ASSERT(PageDependencyStart < NANITE_MAX_RESOURCE_PAGES);
		TI_ASSERT(PageDependencyNum <= NANITE_MAX_GROUP_PARTS_MASK);
		PageDependencyStartAndNum = (PageDependencyStart << NANITE_MAX_GROUP_PARTS_BITS) | PageDependencyNum;
	}

	uint32 GetPageIndex() const { return PageIndex; }
	uint32 GetNodeIndex() const { return HierarchyNodeAndChildIndex >> NANITE_MAX_HIERACHY_CHILDREN_BITS; }
	uint32 GetChildIndex() const { return HierarchyNodeAndChildIndex & (NANITE_MAX_HIERACHY_CHILDREN - 1); }
	uint32 GetClusterGroupPartStartIndex() const { return ClusterGroupPartStartIndex; }
	uint32 GetPageDependencyStart() const { return PageDependencyStartAndNum >> NANITE_MAX_GROUP_PARTS_BITS; }
	uint32 GetPageDependencyNum() const { return PageDependencyStartAndNum & NANITE_MAX_GROUP_PARTS_MASK; }

	uint32 PageIndex;
	uint32 HierarchyNodeAndChildIndex;
	uint32 ClusterGroupPartStartIndex;
	uint32 PageDependencyStartAndNum;
};

class FClusterFixup
{
public:
	FClusterFixup() {}

	FClusterFixup(uint32 PageIndex, uint32 ClusterInstanceIndex, uint32 PageDependencyStart, uint32 PageDependencyNum)
	{
		TI_ASSERT(PageIndex < (1 << (32 - NANITE_MAX_CLUSTERS_PER_GROUP_BITS)));
		TI_ASSERT(ClusterInstanceIndex < NANITE_MAX_CLUSTER_INSTANCES_PER_PAGE);
		PageAndClusterInstanceIndex = (PageIndex << NANITE_MAX_CLUSTER_INSTANCES_PER_PAGE_BITS) | ClusterInstanceIndex;

		TI_ASSERT(PageDependencyStart < NANITE_MAX_RESOURCE_PAGES);
		TI_ASSERT(PageDependencyNum <= NANITE_MAX_GROUP_PARTS_MASK);
		PageDependencyStartAndNum = (PageDependencyStart << NANITE_MAX_GROUP_PARTS_BITS) | PageDependencyNum;
	}

	uint32 GetPageIndex() const { return PageAndClusterInstanceIndex >> NANITE_MAX_CLUSTER_INSTANCES_PER_PAGE_BITS; }
	uint32 GetClusterInstanceIndex() const { return PageAndClusterInstanceIndex & (NANITE_MAX_CLUSTER_INSTANCES_PER_PAGE - 1u); }
	uint32 GetPageDependencyStart() const { return PageDependencyStartAndNum >> NANITE_MAX_GROUP_PARTS_BITS; }
	uint32 GetPageDependencyNum() const { return PageDependencyStartAndNum & NANITE_MAX_GROUP_PARTS_MASK; }

	uint32 PageAndClusterInstanceIndex;
	uint32 PageDependencyStartAndNum;
};


class FFixupChunk	//TODO: rename to something else
{
public:
	struct FHeader
	{
		uint16 NumClusters = 0;
		uint16 NumClusterInstances = 0;
		uint16 NumHierachyFixups = 0;
		uint16 NumClusterFixups = 0;
	} Header;

	uint8 Data[sizeof(FHierarchyFixup) * NANITE_MAX_CLUSTERS_PER_PAGE + sizeof(FClusterFixup) * NANITE_MAX_CLUSTER_INSTANCES_PER_PAGE];	// One hierarchy fixup per cluster and at most one cluster fixup per cluster.

	FClusterFixup& GetClusterFixup(uint32 Index) const { TI_ASSERT(Index < Header.NumClusterFixups);  return ((FClusterFixup*)(Data + Header.NumHierachyFixups * sizeof(FHierarchyFixup)))[Index]; }
	FHierarchyFixup& GetHierarchyFixup(uint32 Index) const { TI_ASSERT(Index < Header.NumHierachyFixups); return ((FHierarchyFixup*)Data)[Index]; }
	uint32				GetSize() const { return sizeof(Header) + Header.NumHierachyFixups * sizeof(FHierarchyFixup) + Header.NumClusterFixups * sizeof(FClusterFixup); }
};

class TNaniteMesh
{
public:
	static const int32 MaxHierarchyNodes = 256;
	~TNaniteMesh() = default;
	static TNaniteMesh* LoadMesh();

	bool IsRootPage(uint32 PageIndex) const { return PageIndex < NumRootPages; }
protected:
	TNaniteMesh();
	static bool ConvertNanieMesh(TNaniteMesh& Mesh);

public:
	TVector<uint8> RootData; // Root pages are loaded on resource load, so we always have something to draw.
	TVector<uint8> StreamablePages;	// Remaining pages are streamed on demand.
	TVector<FPackedHierarchyNode> HierarchyNodes;
	TVector<FPageStreamingState> PageStreamingStates;
	TVector<uint32> PageDependencies;
	uint32 NumRootPages = 0;
	uint32 NumRootPageClusters = 0;
	uint32 NumRootPageClusterInstances = 0;
	int32 PositionPrecision = 0;
};


inline uint32 GetMaxNodes()
{
	const int32 GNaniteMaxNodes = 2 * 1048576;
	return GNaniteMaxNodes & -NANITE_MAX_BVH_NODES_PER_GROUP;
}

inline uint32 GetMaxCandidateClusters()
{
	int32 GNaniteMaxCandidateClusters = 16 * 1048576;
	const uint32 MaxCandidateClusters = GNaniteMaxCandidateClusters & -NANITE_PERSISTENT_CLUSTER_CULLING_GROUP_SIZE;
	return MaxCandidateClusters;
}

inline uint32 GetMaxClusterBatches()
{
	const uint32 MaxCandidateClusters = GetMaxCandidateClusters();
	TI_ASSERT(MaxCandidateClusters % NANITE_PERSISTENT_CLUSTER_CULLING_GROUP_SIZE == 0);
	return MaxCandidateClusters / NANITE_PERSISTENT_CLUSTER_CULLING_GROUP_SIZE;
}

inline uint32 GetMaxVisibleClusters()
{
	const int32 GNaniteMaxVisibleClusters = 4 * 1048576;
	return GNaniteMaxVisibleClusters;
}

inline uint32 GetMaxStreamingPages()
{
	const int32 MaxStreamingPages = 4096;
	return MaxStreamingPages;
}