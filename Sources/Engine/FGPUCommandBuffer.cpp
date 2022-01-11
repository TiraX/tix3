/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "FGPUCommandBuffer.h"

namespace tix
{
	FGPUCommandBuffer::FGPUCommandBuffer(FGPUCommandSignaturePtr Signature, uint32 InCommandsCount, uint32 InBufferFlag)
		: FRenderResource(RRT_GPU_COMMAND_BUFFER)
		, GPUCommandSignature(Signature)
	{
	}

	FGPUCommandBuffer::~FGPUCommandBuffer()
	{
	}
}
