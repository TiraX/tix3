/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "FRtxPipeline.h"

namespace tix
{
	FRtxPipeline::FRtxPipeline(FShaderPtr InShader)
		: FRenderResource(RRT_RTX_PIPELINE)
		, ShaderLib(InShader)
	{
	}

	FRtxPipeline::~FRtxPipeline()
	{
	}
}