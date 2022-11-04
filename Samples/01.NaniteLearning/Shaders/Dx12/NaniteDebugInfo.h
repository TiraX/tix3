// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

static const int MaxDebugInfo = 32;

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
	uint NodeCount;
	uint BatchSize;
	//uint NodeData[NANITE_MAX_BVH_NODES_PER_GROUP];
	//FHierarchyNodeSliceSimple NodeSlice[16];
	//uint bShouldVisitChild[64];
	//FPair_ P[64];

	uint ClusterBatchStartIndex;
	uint GroupClusterBatchReadySize;
	uint NumLeaves;
	uint Padding1;
	//uint bOutput[64];
	uint LoadedLeaf[64];
};
