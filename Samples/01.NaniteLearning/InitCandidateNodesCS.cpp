/*
	TiX Engine v3.0 Copyright (C) 2018~2021
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "InitCandidateNodesCS.h"
#include "NaniteMesh.h"

FInitCandidateNodesCS::FInitCandidateNodesCS()
	: FComputeTask("S_InitCandidateNodesCS")
{
}

FInitCandidateNodesCS::~FInitCandidateNodesCS()
{
}

void FInitCandidateNodesCS::ApplyParameters(
	FRHICmdList* RHICmdList,
	const FDecodeInfo& InDecodeInfo,
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
	_AssignValue(NodesAndClusterBatches);
#undef _AssignValue

	if (UpdateResourceTable)
	{
		FRHI* RHI = FRHI::Get();
		RHI->PutRWUniformBufferInTable(ResourceTable, NodesAndClusterBatches, UAV_MainAndPostNodesAndClusterBatches);
	}
}

void FInitCandidateNodesCS::Run(FRHICmdList* RHICmdList)
{
	RHICmdList->BeginEvent("Nanite::InitNodes");

	RHICmdList->SetGPUBufferState(NodesAndClusterBatches->GetGPUBuffer(), EGPUResourceState::UnorderedAccess);

	RHICmdList->SetComputePipeline(ComputePipeline);
	RHICmdList->SetComputeConstant(FInitCandidateNodesCS::RC_DecodeInfo, &DecodeInfo, sizeof(FDecodeInfo) / sizeof(uint32));
	RHICmdList->SetComputeResourceTable(FInitCandidateNodesCS::RT_Table, ResourceTable);
	FInt3 Groups(1, 1, 1);
	Groups.X = TMath::DivideAndRoundUp((int32)GetMaxNodes(), 64);
	RHICmdList->DispatchCompute(
		FInt3(64, 1, 1),
		Groups
	);
	RHICmdList->EndEvent();
}
