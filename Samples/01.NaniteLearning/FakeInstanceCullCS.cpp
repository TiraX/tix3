/*
	TiX Engine v3.0 Copyright (C) 2018~2021
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "FakeInstanceCullCS.h"
#include "NaniteMesh.h"

FFakeInstanceCullCS::FFakeInstanceCullCS()
	: FComputeTask("S_FakeInstanceCullCS")
{
}

FFakeInstanceCullCS::~FFakeInstanceCullCS()
{
}

void FFakeInstanceCullCS::ApplyParameters(
	FRHICmdList* RHICmdList,
	const FDecodeInfo& InDecodeInfo,
	FUniformBufferPtr InQueueState,
	FUniformBufferPtr InNodesAndClusterBatches
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
	_AssignValue(QueueState);
	_AssignValue(NodesAndClusterBatches);
#undef _AssignValue

	if (UpdateResourceTable)
	{
		FRHI* RHI = FRHI::Get();
		RHI->PutRWUniformBufferInTable(ResourceTable, QueueState, UAV_QueueState);
		RHI->PutRWUniformBufferInTable(ResourceTable, NodesAndClusterBatches, UAV_MainAndPostNodesAndClusterBatches);
	}
}

void FFakeInstanceCullCS::Run(FRHICmdList* RHICmdList)
{
	RHICmdList->BeginEvent("Nanite::FakeInstanceCull");

	RHICmdList->SetGPUBufferState(QueueState->GetGPUBuffer(), EGPUResourceState::UnorderedAccess);
	RHICmdList->SetGPUBufferState(NodesAndClusterBatches->GetGPUBuffer(), EGPUResourceState::UnorderedAccess);

	RHICmdList->SetComputePipeline(ComputePipeline);
	RHICmdList->SetComputeConstant(FFakeInstanceCullCS::RC_DecodeInfo, &DecodeInfo, sizeof(FDecodeInfo) / sizeof(uint32));
	RHICmdList->SetComputeResourceTable(FFakeInstanceCullCS::RT_Table, ResourceTable);

	// We have only 1 instance, only dispatch 1 group
	RHICmdList->DispatchCompute(
		FInt3(64, 1, 1),
		FInt3(1, 1, 1)
	);
	RHICmdList->EndEvent();
}
