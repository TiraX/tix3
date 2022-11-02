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
	float	MinLODError;
	float	MaxParentLODError;
	uint32	ChildStartReference;	// Can be node (index) or cluster (page:cluster)
	uint32	NumChildren;
	bool	bEnabled;
	bool	bLoaded;
	bool	bLeaf;
};

struct FPair_
{
	uint32 bVisible;
	uint32 bLoaded;
	uint32 bLeaf;
};

struct FNaniteDebug
{
	static const int32 MaxDebugInfo = 16;
	uint32 GroupNodeBatchStartIndex;
	uint32 GroupNumCandidateNodes;
	uint32 GroupCandidateNodesOffset;
	uint32 BatchSize;
	uint32 NodeData[NANITE_MAX_BVH_NODES_PER_GROUP];
	//FHierarchyNodeSliceSimple NodeSlice[16];
	//uint32 bShouldVisitChild[64];
	FPair_ P[64];

	FNaniteDebug()
		: GroupNodeBatchStartIndex(0)
		, GroupNumCandidateNodes(0)
		, GroupCandidateNodesOffset(0)
		, BatchSize(0)
	{
		//memset(bShouldVisitChild, 0, sizeof(bShouldVisitChild));
	}
};


FUniformBufferPtr CreateDebugInfoUniform(FRHICmdList* RHICmdList, int32 Capcity = 1);
