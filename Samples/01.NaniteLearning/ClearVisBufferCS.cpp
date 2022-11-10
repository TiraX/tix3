/*
	TiX Engine v3.0 Copyright (C) 2018~2021
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "ClearVisBufferCS.h"

FClearVisBufferCS::FClearVisBufferCS()
	: FComputeTask("S_ClearVisBufferCS")
{
}

FClearVisBufferCS::~FClearVisBufferCS()
{
}

void FClearVisBufferCS::ApplyParameters(
	FRHICmdList* RHICmdList,
	const FInt2& InSize,
	FTexturePtr InVisBuffer
)
{
	Size = InSize;
	VisBuffer = InVisBuffer;
	ResourceTable = RHICmdList->GetHeap(0)->CreateRenderResourceTable(1);

	FRHI* RHI = FRHI::Get();
	RHI->PutRWTextureInTable(ResourceTable, InVisBuffer, 0, 0);
}

void FClearVisBufferCS::Run(FRHICmdList* RHICmdList)
{
	const FInt3 DispatchSize(16, 16, 1);
	FInt3 GroupCount(1, 1, 1);
	GroupCount.X = TMath::DivideAndRoundUp(Size.X, 16);
	GroupCount.Y = TMath::DivideAndRoundUp(Size.Y, 16);

	RHICmdList->BeginEvent("ClearVisBuffer");
	RHICmdList->SetComputePipeline(ComputePipeline);

	RHICmdList->SetComputeConstant(0, &Size, sizeof(FInt2) / sizeof(int32));
	RHICmdList->SetComputeResourceTable(1, ResourceTable);

	RHICmdList->DispatchCompute(DispatchSize, GroupCount);
	RHICmdList->EndEvent();
}
