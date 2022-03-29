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

		FShaderPtr ComputeShader = FRHI::Get()->CreateComputeShader(ShaderName);
		TVector<TStreamPtr> ShaderCodes;
		ShaderCodes.resize(ESS_COUNT);
		ShaderCodes[ESS_COMPUTE_SHADER] = TShader::LoadShaderBlob(ShaderName);
		FRHI::Get()->UpdateHardwareResourceShader(ComputeShader, ShaderCodes);

		ComputePipeline = FRHI::Get()->CreatePipeline(ComputeShader);
		ComputePipeline->SetResourceName(ShaderName + "-CPSO");

		if (HasFlag(COMPUTE_TILE))
		{
			// For metal : Create tile pipeline state
			TI_ASSERT(TilePLDesc->GetRTCount() > 0);
			FRHI::Get()->UpdateHardwareResourceTilePL(ComputePipeline, TilePLDesc);
		}
		else
		{
			FRHI::Get()->UpdateHardwareResourceComputePipeline(ComputePipeline);
		}
	}
}
