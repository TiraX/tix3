/*
	TiX Engine v3.0 Copyright (C) 2018~2021
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "PersistentCullCS.h"

FPersistentCullCS::FPersistentCullCS()
	: FComputeTask("S_PersistentCullCS")
{
}

FPersistentCullCS::~FPersistentCullCS()
{
}

void FPersistentCullCS::ApplyParameters(
	FRHICmdList* RHICmdList,
	const FDecodeInfo& InDecodeInfo,
	FUniformBufferPtr InClusterPageData,
	FUniformBufferPtr InHierachyBuffer,
	FUniformBufferPtr InView,
	FUniformBufferPtr InQueueState,
	FUniformBufferPtr InClusterBatches,
	FUniformBufferPtr InCandididateClusters,
	FUniformBufferPtr InStreamingRequest,
	FUniformBufferPtr InVisibleClustersSWHW,
	FUniformBufferPtr InVisibleClustersArgsSWHW
)
{
	DecodeInfo = InDecodeInfo;

	bool UpdateResourceTable = false;
	if (ResourceTable == nullptr)
	{
		ResourceTable = RHICmdList->GetHeap(0)->CreateRenderResourceTable(NumParams);
		UpdateResourceTable = true;
	}

#define _AssignValue(x) if (x != In##x) { x = In##x; UpdateResourceTable = true; }
	_AssignValue(ClusterPageData);
	_AssignValue(HierachyBuffer);
	_AssignValue(View);
	_AssignValue(QueueState);
	_AssignValue(ClusterBatches);
	_AssignValue(CandididateClusters);
	_AssignValue(StreamingRequest);
	_AssignValue(VisibleClustersSWHW);
	_AssignValue(VisibleClustersArgsSWHW);
#undef _AssignValue

	if (UpdateResourceTable)
	{
		FRHI* RHI = FRHI::Get();
		// Crack, set ClusterPageData state to unordered access
		RHICmdList->SetGPUBufferState(ClusterPageData->GetGPUBuffer(), EGPUResourceState::NonPixelShaderResource);
		RHI->PutUniformBufferInTable(ResourceTable, ClusterPageData, SRV_ClusterPageData);
		RHI->PutUniformBufferInTable(ResourceTable, HierachyBuffer, SRV_HierachyBuffer);
		RHI->PutUniformBufferInTable(ResourceTable, View, SRV_View);

		RHI->PutRWUniformBufferInTable(ResourceTable, QueueState, UAV_QueueState);
		RHI->PutRWUniformBufferInTable(ResourceTable, ClusterBatches, UAV_MainAndPostNodesAndClusterBatches);
		RHI->PutRWUniformBufferInTable(ResourceTable, CandididateClusters, UAV_MainAndPostCandididateClusters);
		//RHI->PutRWUniformBufferInTable(ResourceTable, StreamingRequest, UAV_OutStreamingRequests);
		RHI->PutRWUniformBufferInTable(ResourceTable, VisibleClustersSWHW, UAV_OutVisibleClustersSWHW);
		RHI->PutRWUniformBufferInTable(ResourceTable, VisibleClustersArgsSWHW, UAV_VisibleClustersArgsSWHW);
	}
}

void FPersistentCullCS::Run(FRHICmdList* RHICmdList)
{
	RHICmdList->BeginEvent("PersistentCull");

	RHICmdList->SetGPUBufferState(ClusterPageData->GetGPUBuffer(), EGPUResourceState::NonPixelShaderResource);
	RHICmdList->SetGPUBufferState(HierachyBuffer->GetGPUBuffer(), EGPUResourceState::NonPixelShaderResource);
	RHICmdList->SetGPUBufferState(View->GetGPUBuffer(), EGPUResourceState::NonPixelShaderResource);

	RHICmdList->SetGPUBufferState(QueueState->GetGPUBuffer(), EGPUResourceState::UnorderedAccess);
	RHICmdList->SetGPUBufferState(ClusterBatches->GetGPUBuffer(), EGPUResourceState::UnorderedAccess);
	RHICmdList->SetGPUBufferState(CandididateClusters->GetGPUBuffer(), EGPUResourceState::UnorderedAccess);
	//RHICmdList->SetGPUBufferState(StreamingRequest->GetGPUBuffer(), EGPUResourceState::UnorderedAccess);
	RHICmdList->SetGPUBufferState(VisibleClustersSWHW->GetGPUBuffer(), EGPUResourceState::UnorderedAccess);
	RHICmdList->SetGPUBufferState(VisibleClustersArgsSWHW->GetGPUBuffer(), EGPUResourceState::UnorderedAccess);

	RHICmdList->SetComputePipeline(ComputePipeline);
	RHICmdList->SetComputeConstant(FPersistentCullCS::RC_DecodeInfo, &DecodeInfo, sizeof(FDecodeInfo) / sizeof(uint32));
	RHICmdList->SetComputeResourceTable(FPersistentCullCS::RT_Table, ResourceTable);
	RHICmdList->DispatchCompute(
		FInt3(NANITE_PERSISTENT_CLUSTER_CULLING_GROUP_SIZE, 1, 1),
		FInt3(1440, 1, 1)
	);
	RHICmdList->EndEvent();
}
