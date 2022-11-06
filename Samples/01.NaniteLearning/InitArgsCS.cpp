/*
	TiX Engine v3.0 Copyright (C) 2018~2021
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "InitArgsCS.h"

FInitArgsCS::FInitArgsCS()
	: FComputeTask("S_InitArgsCS")
{
}

FInitArgsCS::~FInitArgsCS()
{
}

void FInitArgsCS::ApplyParameters(
	FRHICmdList* RHICmdList,
	const FDecodeInfo& InDecodeInfo,
	FUniformBufferPtr InQueueState,
	FUniformBufferPtr InVisibleClustersArgsSWHW
)
{
	DecodeInfo = InDecodeInfo;
	QueueState = InQueueState;
	VisibleClustersArgsSWHW = InVisibleClustersArgsSWHW;

	if (ResourceTable == nullptr)
	{
		ResourceTable = RHICmdList->GetHeap(0)->CreateRenderResourceTable(NumParams);

		FRHI* RHI = FRHI::Get();
		RHI->PutRWUniformBufferInTable(ResourceTable, QueueState, UAV_QueueState);
		RHI->PutRWUniformBufferInTable(ResourceTable, VisibleClustersArgsSWHW, UAV_VisibleClustersArgsSWHW);
	}
}

void FInitArgsCS::Run(FRHICmdList* RHICmdList)
{
	const FInt3 DispatchSize(1, 1, 1);
	FInt3 GroupCount(1, 1, 1);

	RHICmdList->BeginEvent("InitArgs");
	RHICmdList->SetComputePipeline(ComputePipeline);

	RHICmdList->SetComputeConstant(RC_DecodeInfo, &DecodeInfo, sizeof(FDecodeInfo) / sizeof(uint32));
	RHICmdList->SetComputeResourceTable(RT_Table, ResourceTable);

	RHICmdList->DispatchCompute(DispatchSize, GroupCount);
	RHICmdList->EndEvent();
}
