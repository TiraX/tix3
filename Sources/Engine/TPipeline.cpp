/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "TPipeline.h"

namespace tix
{
	TPipeline::TPipeline()
		: TResource(ERES_PIPELINE)
	{
	}

	TPipeline::~TPipeline()
	{
	}

	void TPipeline::SetShader(TShaderPtr Shader)
	{
		Desc.Shader = Shader;
	}
	
	void TPipeline::InitRenderThreadResource()
	{
		TI_ASSERT(PipelineResource == nullptr);
		if (Desc.Shader->ShaderResource == nullptr)
		{
			Desc.Shader->InitRenderThreadResource();
		}
		PipelineResource = FRHI::Get()->CreatePipeline(Desc.Shader->ShaderResource);
		PipelineResource->SetResourceName(GetResourceName());

		FPipelinePtr _PipelineResource = PipelineResource;
		const TPipelineDesc& PLDesc = Desc;
		ENQUEUE_RENDER_COMMAND(TPipelineUpdateResource)(
			[_PipelineResource, PLDesc]()
			{
				FRHI::Get()->UpdateHardwareResourceGraphicsPipeline(_PipelineResource, PLDesc);
			});
	}

	void TPipeline::DestroyRenderThreadResource()
	{
		TI_ASSERT(PipelineResource != nullptr);

		FPipelinePtr _PipelineResource = PipelineResource;
		ENQUEUE_RENDER_COMMAND(TPipelineDestroyFPipeline)(
			[_PipelineResource]()
			{
				//_PipelineResource = nullptr;
			});
		PipelineResource = nullptr;
	}
    
    ////////////////////////////////////////////////////////////////////////
    
    TTilePipeline::TTilePipeline()
        : TResource(ERES_TILEPIPELINE)
        , RTCount(0)
        , SampleCount(1)
        , ThreadGroupSizeMatchesTileSize(1)
    {
    }
    
    TTilePipeline::~TTilePipeline()
    {
    }
}
