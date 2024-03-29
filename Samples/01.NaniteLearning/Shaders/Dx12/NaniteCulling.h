// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "NaniteDataDecode.h"

#define CULLING_PASS_NO_OCCLUSION				0
#define CULLING_PASS_OCCLUSION_MAIN				1
#define CULLING_PASS_OCCLUSION_POST				2
#define CULLING_PASS_EXPLICIT_LIST				3


struct FCandidateNode
{
	uint	Flags;
	uint	ViewId;
	uint	InstanceId;
	uint	NodeIndex;
	uint	EnabledBitmask;
};

uint GetCandidateNodeSize(bool bPostPass)		{ return bPostPass ? 12u : 8u; }
uint GetCandidateClusterInstanceSize()			{ return 12u; }

// NodesAndClusterBatches layout: Main Cluster Batches, Main Candidate Nodes, Post Cluster Batches, Post Candidate Nodes
uint GetClusterBatchesOffset()					{ return 0u; }
uint GetCandidateNodesOffset()					{ return GetMaxClusterBatches() * 4u; }
uint GetNodesAndBatchesOffset(bool bPostPass)	{ return bPostPass ? (GetCandidateNodesOffset() + DecodeInfo.MaxNodes * GetCandidateNodeSize(false)) : 0u; }
uint GetCandidateClusterInstanceOffset()		{ return 0u; }

// CandidateClusters must be globallycoherent here, otherwise DXC will make buffer access non-globallycoherent when targeting SM6.6.
void StoreCandidateClusterInstanceCoherentNoCheck(RWCoherentByteAddressBuffer CandidateClusters, uint ClusterInstanceIndex, FVisibleClusterInstance VisibleClusterInstance)
{
	uint4 RawData = PackVisibleClusterInstance(VisibleClusterInstance, false);
	CandidateClusters.Store3(GetCandidateClusterInstanceOffset() + ClusterInstanceIndex * GetCandidateClusterInstanceSize(), RawData.xyz);
}

// CandidateClusters must be globallycoherent here, otherwise DXC will make buffer access non-globallycoherent when targeting SM6.6.
void StoreCandidateClusterInstanceCoherent(RWCoherentByteAddressBuffer CandidateClusters, uint ClusterInstanceIndex, FVisibleClusterInstance VisibleClusterInstance)
{
	//checkSlow(ClusterIndex < MaxCandidateClusters);
	StoreCandidateClusterInstanceCoherentNoCheck(CandidateClusters, ClusterInstanceIndex, VisibleClusterInstance);
}

void StoreVisibleClusterInstance(RWByteAddressBuffer VisibleClusters, uint ClusterIdx, FVisibleClusterInstance VisibleClusterInstance, bool bHasPageData = false)
{
	uint4 RawData = PackVisibleClusterInstance(VisibleClusterInstance, bHasPageData);
	if (bHasPageData)
	{
		VisibleClusters.Store4(ClusterIdx * 16, RawData);
	}
	else
	{
		VisibleClusters.Store3(ClusterIdx * 12, RawData.xyz);
	}
}

uint4 PackCandidateNode(FCandidateNode Node)
{
	// Leave at least one bit unused in each of the fields, so 0xFFFFFFFFu is never a valid value.
	uint4 RawData;
	RawData.x = (Node.InstanceId << NANITE_NUM_CULLING_FLAG_BITS) | Node.Flags;
	RawData.y = (Node.ViewId << NANITE_MAX_NODES_PER_PRIMITIVE_BITS) | Node.NodeIndex;
	RawData.z = Node.EnabledBitmask;
	RawData.w = 0;

	//checkSlow(RawData.x != 0xFFFFFFFFu && RawData.y != 0xFFFFFFFFu && RawData.z != 0xFFFFFFFFu);

	return RawData;
}

FCandidateNode UnpackCandidateNode(uint4 RawData, bool bHasEnabledMask)
{
	FCandidateNode Node;
	Node.Flags			= BitFieldExtractU32(RawData.x, NANITE_NUM_CULLING_FLAG_BITS, 0);
	Node.InstanceId		= BitFieldExtractU32(RawData.x, NANITE_MAX_INSTANCES_BITS, NANITE_NUM_CULLING_FLAG_BITS);
	Node.NodeIndex		= BitFieldExtractU32(RawData.y, NANITE_MAX_NODES_PER_PRIMITIVE_BITS, 0);
	Node.ViewId			= BitFieldExtractU32(RawData.y, NANITE_MAX_VIEWS_PER_CULL_RASTERIZE_PASS_BITS, NANITE_MAX_NODES_PER_PRIMITIVE_BITS);
	Node.EnabledBitmask = bHasEnabledMask ? RawData.z : 0xFFFFFFFFu;
	return Node;
}

// NodesAndClusterBatches must be globallycoherent here, otherwise DXC will make buffer access non-globallycoherent when targeting SM6.6.
uint4 LoadCandidateNodeDataCoherent(RWCoherentByteAddressBuffer NodesAndClusterBatches, uint NodeIndex, bool bPostPass)
{
	//checkSlow(NodeIndex < MaxNodes);
	const uint Offset = GetNodesAndBatchesOffset(bPostPass) + GetCandidateNodesOffset();
	return bPostPass ?	uint4(NodesAndClusterBatches.Load3(Offset + NodeIndex * 12), 0) :
						uint4(NodesAndClusterBatches.Load2(Offset + NodeIndex * 8), 0, 0);
}

void StoreCandidateNodeData(RWByteAddressBuffer NodesAndClusterBatches, uint NodeIndex, uint4 RawData, bool bPostPass)
{
	//checkSlow(NodeIndex < MaxNodes);
	const uint Offset = GetNodesAndBatchesOffset(bPostPass) + GetCandidateNodesOffset();
	if (bPostPass)
		NodesAndClusterBatches.Store3(Offset + NodeIndex * 12, RawData.xyz);
	else
		NodesAndClusterBatches.Store2(Offset + NodeIndex * 8, RawData.xy);
}

// NodesAndClusterBatches must be globallycoherent here, otherwise DXC will make buffer access non-globallycoherent when targeting SM6.6.
void StoreCandidateNodeDataCoherent(RWCoherentByteAddressBuffer NodesAndClusterBatches, uint NodeIndex, uint4 RawData, bool bPostPass)
{
	//checkSlow(NodeIndex < MaxNodes);
	const uint Offset = GetNodesAndBatchesOffset(bPostPass) + GetCandidateNodesOffset();
	if (bPostPass)
		NodesAndClusterBatches.Store3(Offset + NodeIndex * 12, RawData.xyz);
	else
		NodesAndClusterBatches.Store2(Offset + NodeIndex * 8, RawData.xy);
}

void StoreCandidateNode(RWByteAddressBuffer NodesAndClusterBatches, uint NodeIndex, FCandidateNode Node, bool bPostPass)
{
	//checkSlow(NodeIndex < MaxNodes);
	StoreCandidateNodeData(NodesAndClusterBatches, NodeIndex, PackCandidateNode(Node), bPostPass);
}

// NodesAndClusterBatches must be globallycoherent here, otherwise DXC will make buffer access non-globallycoherent when targeting SM6.6.
void StoreCandidateNodeCoherent(RWCoherentByteAddressBuffer NodesAndClusterBatches, uint NodeIndex, FCandidateNode Node, bool bPostPass)
{
	//checkSlow(NodeIndex < MaxNodes);
	StoreCandidateNodeDataCoherent(NodesAndClusterBatches, NodeIndex, PackCandidateNode(Node), bPostPass);
}

void ClearCandidateNode(RWByteAddressBuffer NodesAndClusterBatches, uint NodeIndex, bool bPostPass)
{
	//checkSlow(NodeIndex < MaxNodes);
	StoreCandidateNodeData(NodesAndClusterBatches, NodeIndex, 0xFFFFFFFFu, bPostPass);
}

// NodesAndClusterBatches must be globallycoherent here, otherwise DXC will make buffer access non-globallycoherent when targeting SM6.6.
void ClearCandidateNodeCoherent(RWCoherentByteAddressBuffer NodesAndClusterBatches, uint NodeIndex, bool bPostPass)
{
	//checkSlow(NodeIndex < MaxNodes);
	StoreCandidateNodeDataCoherent(NodesAndClusterBatches, NodeIndex, 0xFFFFFFFFu, bPostPass);
}

uint LoadClusterBatchCoherent(RWCoherentByteAddressBuffer NodesAndClusterBatches, uint BatchIndex, bool bPostPass)
{
	//checkSlow(BatchIndex < GetMaxClusterBatches());
	return NodesAndClusterBatches.Load(GetNodesAndBatchesOffset(bPostPass) + GetClusterBatchesOffset() + BatchIndex * 4);
}

void AddToClusterBatchCoherent(RWCoherentByteAddressBuffer NodesAndClusterBatches, uint BatchIndex, uint Add, bool bPostPass)
{
	//checkSlow(BatchIndex < GetMaxClusterBatches());
	NodesAndClusterBatches.InterlockedAdd(GetNodesAndBatchesOffset(bPostPass) + GetClusterBatchesOffset() + BatchIndex * 4, Add);
}

void ClearClusterBatch(RWByteAddressBuffer NodesAndClusterBatches, uint BatchIndex, bool bPostPass)
{
	//checkSlow(BatchIndex < GetMaxClusterBatches());
	NodesAndClusterBatches.Store(GetNodesAndBatchesOffset(bPostPass) + GetClusterBatchesOffset() + BatchIndex * 4, 0);
}

void ClearClusterBatchCoherent(RWCoherentByteAddressBuffer NodesAndClusterBatches, uint BatchIndex, bool bPostPass)
{
	//checkSlow(BatchIndex < GetMaxClusterBatches());
	NodesAndClusterBatches.Store(GetNodesAndBatchesOffset(bPostPass) + GetClusterBatchesOffset() + BatchIndex * 4, 0);
}