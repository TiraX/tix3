/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

struct FCandidateNode
{
	uint32	Flags;
	uint32	ViewId;
	uint32	InstanceId;
	uint32	NodeIndex;
	uint32	EnabledBitmask;
};

struct FHierarchyNodeSliceSimple
{
	FFloat4	LODBounds;
	float	MinLODError;
	float	MaxParentLODError;
	float	ProjectedEdgeScale;
	float	NodeCullThres;
};

struct FPair_
{
	uint32 bVisible;
	uint32 bLoaded;
	uint32 bLeaf;
};

struct FNaniteCullingDebug
{
	static const int32 MaxDebugInfo = 128;
	uint32 GroupNodeBatchStartIndex;
	uint32 GroupNumCandidateNodes;
	uint32 NodeCount;
	uint32 BatchSize;
	//uint32 NodeData[NANITE_MAX_BVH_NODES_PER_GROUP];
	//FHierarchyNodeSliceSimple NodeSlice[16];
	//uint32 bShouldVisitChild[64];
	//FPair_ P[64];

	uint32 ClusterBatchStartIndex;
	uint32 GroupClusterBatchReadySize;
	uint32 NumLeaves;
	uint32 Padding1;
	//uint32 bOutput[64];
	FHierarchyNodeSliceSimple Nd[4];

	FNaniteCullingDebug()
		: GroupNodeBatchStartIndex(0)
		, GroupNumCandidateNodes(0)
		, NodeCount(0)
		, BatchSize(0)
		, ClusterBatchStartIndex(0)
		, GroupClusterBatchReadySize(0)
		, NumLeaves(0)
		, Padding1(0)
	{
		//memset(bShouldVisitChild, 0, sizeof(bShouldVisitChild));
	}
};


FUniformBufferPtr CreateCullingDebugInfoUniform(FRHICmdList* RHICmdList, int32 Capcity = 1);


struct FNaniteTessDebug
{
	static const int32 MaxDebugInfo = 128;
	FUInt4 TessFactor;
	uint32 TessedTrisCount;
	uint32 TotalGroups;
	uint32 Offset;

	FNaniteTessDebug()
		: TessedTrisCount(0)
		, TotalGroups(0)
	{
		//memset(bShouldVisitChild, 0, sizeof(bShouldVisitChild));
	}
};

struct FNaniteTessDebugTable
{
	static const int32 MaxDebugInfo = 2400;
	FUInt4 TF;
	uint32 TriIndexInAS;
	uint32 TessIndex;

	FFloat3 UnpackedN;
	FFloat2 UnpackedUV;
};
FUniformBufferPtr CreateTessDebugInfoUniform(FRHICmdList* RHICmdList, int32 Capcity = 128);
FUniformBufferPtr CreateTessDebugTableUniform(FRHICmdList* RHICmdList, int32 Capcity = 128);
