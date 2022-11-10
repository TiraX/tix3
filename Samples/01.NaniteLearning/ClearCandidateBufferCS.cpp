/*
	TiX Engine v3.0 Copyright (C) 2018~2021
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "ClearCandidateBufferCS.h"

FClearCandidateBufferCS::FClearCandidateBufferCS()
	: FComputeTask("S_ClearCandidateBufferCS")
{
}

FClearCandidateBufferCS::~FClearCandidateBufferCS()
{
}

void FClearCandidateBufferCS::ApplyParameters(
	FRHICmdList* RHICmdList,
	FUniformBufferPtr InCandididateClusters
)
{
	CandididateClusters = InCandididateClusters;
}

void FClearCandidateBufferCS::Run(FRHICmdList* RHICmdList)
{
	const FInt3 DispatchSize(128, 1, 1);
	FInt3 GroupCount(1, 1, 1);
	GroupCount.X = CandididateClusters->GetTotalBufferSize() / 128 / (16 * 4);

	RHICmdList->BeginEvent("ClearCandidateBuffer");
	RHICmdList->SetComputePipeline(ComputePipeline);

	RHICmdList->SetComputeUAVBuffer(UAV_CandididateClusters, CandididateClusters);

	RHICmdList->DispatchCompute(DispatchSize, GroupCount);
	RHICmdList->EndEvent();
}
