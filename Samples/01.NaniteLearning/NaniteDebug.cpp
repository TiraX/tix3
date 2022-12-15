/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "NaniteDebug.h"

FUniformBufferPtr CreateCullingDebugInfoUniform(FRHICmdList* RHICmdList, int32 Capcity)
{
	TStreamPtr ZeroData = ti_new TStream(sizeof(FNaniteCullingDebug) * Capcity);
	ZeroData->FillWithZero(sizeof(FNaniteCullingDebug) * Capcity);

	FUniformBufferPtr UB = 
		FUniformBuffer::CreateBuffer(
			RHICmdList,
			"Nanite.CullingDebugInfo",
			sizeof(FNaniteCullingDebug),
			Capcity,
			(uint32)EGPUResourceFlag::Uav,
			ZeroData,
			EGPUResourceState::UnorderedAccess);
	return UB;
}

FUniformBufferPtr CreateTessDebugInfoUniform(FRHICmdList* RHICmdList, int32 Capcity)
{
	TStreamPtr ZeroData = ti_new TStream(sizeof(FNaniteTessDebug) * Capcity);
	ZeroData->FillWithZero(sizeof(FNaniteTessDebug) * Capcity);

	FUniformBufferPtr UB =
		FUniformBuffer::CreateBuffer(
			RHICmdList,
			"Nanite.TessDebugInfo",
			sizeof(FNaniteTessDebug),
			Capcity,
			(uint32)EGPUResourceFlag::Uav,
			ZeroData,
			EGPUResourceState::UnorderedAccess);
	return UB;
}
