// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "NaniteDefinitions.h"

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

uint MaxCandidateClusters;

uint GetMaxClusterBatches() { return MaxCandidateClusters / NANITE_PERSISTENT_CLUSTER_CULLING_GROUP_SIZE; }
