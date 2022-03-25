/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "FRHIDx12.h"
#include "FPipelineDx12.h"

#if COMPILE_WITH_RHI_DX12

namespace tix
{
	FPipelineDx12::FPipelineDx12(FShaderPtr InShader)
		: FPipeline(InShader)
	{
	}

	FPipelineDx12::~FPipelineDx12()
	{
		PipelineState = nullptr;
	}
}

#endif	// COMPILE_WITH_RHI_DX12