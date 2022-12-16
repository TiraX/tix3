// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

static const int MaxDebugInfo = 128;

struct FHierarchyNodeSliceSimple
{
	float4	LODBounds;
	float	MinE;
	float	MaxE;
	float	ProjectedEdgeScale;
	float	NodeCullThres;
};

struct FPair_
{
	uint bVisible;
	uint bLoaded;
	uint bLeaf;
};

struct FNaniteCullingDebugInfo
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
	FHierarchyNodeSliceSimple Nd[4];
};

//////////////////////////////////////////////////////////////

struct FNaniteTessDebugInfo
{
	uint4 TessFactor;
	uint TessedCount;
	uint TotalGroups;
}; 
struct FNaniteTessDebugTable
{
	uint AS_TriIndex;
	uint AS_TessOffset;
	uint AS_TessCount;
	uint AS_Encoded;
	uint AS_UInt;
	uint MS_UInt;
	uint MS_Encoded;
	uint MS_TriIndex;
	uint MS_TessOffset;
	uint MS_TessCount;
};