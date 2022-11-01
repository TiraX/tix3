/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "NaniteDebug.h"

FUniformBufferPtr CreateDebugInfoUniform(FRHICmdList* RHICmdList, int32 Capcity)
{
	TStreamPtr ZeroData = ti_new TStream(sizeof(FNaniteDebug) * Capcity);
	ZeroData->FillWithZero(sizeof(FNaniteDebug) * Capcity);

	FUniformBufferPtr UB = 
		FUniformBuffer::CreateBuffer(
			RHICmdList,
			"Nanite.DebugInfo",
			sizeof(FNaniteDebug),
			Capcity,
			(uint32)EGPUResourceFlag::Uav,
			ZeroData,
			EGPUResourceState::UnorderedAccess);
	return UB;
}