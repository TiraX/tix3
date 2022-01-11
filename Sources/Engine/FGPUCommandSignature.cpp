/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "FGPUCommandSignature.h"

namespace tix
{
	FGPUCommandSignature::FGPUCommandSignature(FPipelinePtr InPipeline, const TVector<E_GPU_COMMAND_TYPE>& InCommandStructure)
		: FRenderResource(RRT_GPU_COMMAND_SIGNATURE)
		, Pipeline(InPipeline)
		, CommandStructure(InCommandStructure)
	{
	}

	FGPUCommandSignature::~FGPUCommandSignature()
	{
	}
}
