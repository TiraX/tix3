/*
	TiX Engine v3.0 Copyright (C) 2018~2021
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "ScatterUploadCS.h"

FScatterUploadCS::FScatterUploadCS()
	: FComputeTask("S_ScatterUploadCS")
{
}

FScatterUploadCS::~FScatterUploadCS()
{
}

void FScatterUploadCS::Reset(uint32 Count, FUniformBufferPtr InDstBuffer)
{
	ScatterInfo.Count = 0;
	Addresses.clear();
	Values.clear();
	Addresses.reserve(Count);
	Values.reserve(Count);
	DstBuffer = InDstBuffer;
}

void FScatterUploadCS::Add(uint32 Address, uint32 Value)
{
	TI_ASSERT(Addresses.size() == Values.size());
	Addresses.push_back(Address);
	Values.push_back(Value);
	++ScatterInfo.Count;
}

void FScatterUploadCS::Run(FRHICmdList* RHICmdList)
{
	TI_ASSERT(Addresses.size() == Values.size());
	if (Addresses.size() == 0)
		return;
	if (UB_Addr == nullptr || UB_Addr->GetElements() < (uint32)Addresses.size())
	{
		TStreamPtr AddrData = ti_new TStream(Addresses.data(), (uint32)Addresses.size() * sizeof(uint32));
		UB_Addr = 
			FUniformBuffer::CreateBuffer(
				RHICmdList, 
				"ScatterAddr", 
				sizeof(uint32), 
				(uint32)Addresses.size(),
				(uint32)EGPUResourceFlag::Intermediate, 
				AddrData
			);
		TStreamPtr ValueData = ti_new TStream(Values.data(), (uint32)Values.size() * sizeof(uint32));
		UB_Value =
			FUniformBuffer::CreateBuffer(
				RHICmdList,
				"ScatterValue",
				sizeof(uint32),
				(uint32)Values.size(),
				(uint32)EGPUResourceFlag::Intermediate,
				ValueData
			);
	}
	const FInt3 DispatchSize(64, 1, 1);
	FInt3 GroupCount(1, 1, 1);
	GroupCount.X = TMath::RoundUpToPowerOfTwo(ScatterInfo.Count / DispatchSize.X);

	RHICmdList->BeginEvent("ScatterUpload");
	RHICmdList->SetComputePipeline(ComputePipeline);

	RHICmdList->SetComputeConstant(RC_ScatterInfo, &ScatterInfo, sizeof(FScatterInfo) / sizeof(uint32));
	RHICmdList->SetComputeShaderResource(SRV_ScatterAddresses, UB_Addr);
	RHICmdList->SetComputeShaderResource(SRV_ScatterValues, UB_Value);
	RHICmdList->SetComputeUnorderedAccessResource(UAV_DstPageBuffer, DstBuffer);

	RHICmdList->DispatchCompute(DispatchSize, GroupCount);
	RHICmdList->EndEvent();
}
