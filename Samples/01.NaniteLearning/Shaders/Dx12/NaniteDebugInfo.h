// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

static const int MaxDebugInfo = 16;
struct FNaniteDebugInfo
{
	uint4 NodeData[NANITE_MAX_BVH_NODES_PER_GROUP];
	uint NodeIndex[NANITE_MAX_BVH_NODES_PER_GROUP];
	uint GroupNodeBatchStartIndex;
};
