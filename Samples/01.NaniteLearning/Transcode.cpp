/*
	TiX Engine v3.0 Copyright (C) 2018~2021
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "Transcode.h"

FTranscode::FTranscode()
	: FComputeTask("S_TranscodeCS")
{
}

FTranscode::~FTranscode()
{
}

void FTranscode::SetupParams(FRHICmdList* RHICmdList)
{
	// Create resource table for SRVs and UAVs
	TI_ASSERT(ResourceTable == nullptr);
	ResourceTable = RHICmdList->GetHeap(0)->CreateRenderResourceTable(PARAM_NUM);
}

void FTranscode::Run(FRHICmdList* RHICmdList)
{
	//const FInt3& DispatchSize = TccConfig::GetDispatchSize1();
	//FInt3 GroupCount = CalcDispatchGroups(DispatchSize);

	//RHICmdList->BeginEvent(ShaderName.c_str());
	//RHICmdList->SetComputePipeline(ComputePipeline);
	//int32 StartPageIndex = 0;
	//RHICmdList->SetComputeConstant(RC_StartPageIndex, &StartPageIndex, 1);
	//RHICmdList->SetComputeResourceTable(RT_Table, ResourceTable);

	//RHICmdList->DispatchCompute(DispatchSize, GroupCount);
	//RHICmdList->EndEvent();
}
