/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "FPipeline.h"

namespace tix
{
	FPipeline::FPipeline(FShaderPtr InShader)
		: FRenderResource(ERenderResourceType::Pipeline)
		, Shader(InShader)
	{
	}

	FPipeline::~FPipeline()
	{
	}

	///////////////////////////////////////////////////////////////////////////

	//FRenderPipeline::FRenderPipeline(FShaderPtr InShader)
	//	: FPipeline(InShader)
	//{
	//}

	//FRenderPipeline::~FRenderPipeline()
	//{
	//}

	///////////////////////////////////////////////////////////////////////////

	//FComputePipeline::FComputePipeline(FShaderPtr InShader)
	//	: FPipeline(InShader)
	//{
	//}

	//FComputePipeline::~FComputePipeline()
	//{
	//}
}