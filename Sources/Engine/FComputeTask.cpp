/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "FComputeTask.h"

namespace tix
{
	FComputeTask::FComputeTask(const TString& ComputeShaderName, uint32 InFlags)
		: ShaderName(ComputeShaderName)
        , Flags(InFlags)
	{
		ComputeShader = FRHI::Get()->CreateComputeShader(ComputeShaderName);
		ComputePipeline = FRHI::Get()->CreatePipeline(ComputeShader);
	}

	FComputeTask::~FComputeTask()
	{
	}

	void FComputeTask::Finalize()
	{
		if (IsRenderThread())
		{
			FinalizeInRenderThread();
		}
		else
		{
			FComputeTaskPtr ComputeTask = this;
			ENQUEUE_RENDER_COMMAND(FComputeTaskFinalize)(
				[ComputeTask]()
				{
					ComputeTask->FinalizeInRenderThread();
				});
		}
	}

	void FComputeTask::FinalizeInRenderThread()
	{
		TI_ASSERT(IsRenderThread());

		TShaderPtr Shader = ti_new TShader(ShaderName, EST_COMPUTE);
		Shader->LoadShaderCode();
		FRHI::Get()->UpdateHardwareResourceShader(ComputeShader, Shader->GetShaderCodes());

        if (HasFlag(COMPUTE_TILE))
        {
            // For metal : Create tile pipeline state
            TI_ASSERT(TilePLDesc->GetRTCount() > 0);
            FRHI::Get()->UpdateHardwareResourceTilePL(ComputePipeline, TilePLDesc);
        }
        else
        {
            TPipelinePtr PipelineDesc = nullptr;
            FRHI::Get()->UpdateHardwareResourcePL(ComputePipeline, PipelineDesc);
        }
	}
}
