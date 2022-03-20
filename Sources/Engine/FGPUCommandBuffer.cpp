/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "FGPUCommandBuffer.h"

namespace tix
{
	FGPUCommandBuffer::FGPUCommandBuffer(FGPUCommandSignaturePtr Signature, uint32 InCommandsCount, uint32 InBufferFlag)
		: FRenderResource(ERenderResourceType::GpuCommandBuffer)
		, GPUCommandSignature(Signature)
		, CommandsCount(InCommandsCount)
		, CBFlag(InBufferFlag)
	{
	}

	FGPUCommandBuffer::~FGPUCommandBuffer()
	{
	}

	void FGPUCommandBuffer::CreateGPUBuffer(FRHICmdList* CmdList, TStreamPtr InData, EGPUResourceState TargetState)
	{
		TI_ASSERT(IsRenderThread());
		TI_ASSERT(CBBuffer == nullptr);
		FRHI* RHI = FRHI::Get();

		FGPUBufferDesc Desc;
		Desc.Flag = CBFlag;
		Desc.BufferSize = CBData->GetLength();

		// Create GPU resource and copy data
		CBBuffer = RHI->CreateGPUBuffer();
		CBBuffer->Init(CmdList, Desc, CBData);
		RHI->SetGPUBufferName(CBBuffer, GetResourceName());

		if (TargetState != EGPUResourceState::Ignore)
			CmdList->SetGPUBufferState(CBBuffer, TargetState);
	}
}
