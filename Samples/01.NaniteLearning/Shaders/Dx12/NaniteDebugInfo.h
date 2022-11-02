// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

static const int MaxDebugInfo = 16;

struct FHierarchyNodeSliceSimple
{
	float	MinLODError;
	float	MaxParentLODError;
	uint	ChildStartReference;	// Can be node (index) or cluster (page:cluster)
	uint	NumChildren;
	bool	bEnabled;
	bool	bLoaded;
	bool	bLeaf;
};

struct FPair_
{
	uint bVisible;
	uint bLoaded;
	uint bLeaf;
};

struct FNaniteDebugInfo
{
	uint GroupNodeBatchStartIndex;
	uint GroupNumCandidateNodes;
	uint GroupCandidateNodesOffset;
	uint BatchSize;
	uint NodeData[NANITE_MAX_BVH_NODES_PER_GROUP];
	//FHierarchyNodeSliceSimple NodeSlice[16];
	//uint bShouldVisitChild[64];
	FPair_ P[64];
};
