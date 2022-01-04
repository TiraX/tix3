/*
	TiX Engine v2.0 Copyright (C) 2018
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

#if COMPILE_WITH_RHI_METAL

namespace tix
{
	class FPipelineMetal : public FPipeline
	{
	public:
		FPipelineMetal(FShaderPtr InShader);
		virtual ~FPipelineMetal();
	protected:

    private:
        id <MTLRenderPipelineState> RenderPipelineState;
        id <MTLDepthStencilState> DepthState;
        id <MTLComputePipelineState> ComputePipelineState;
		friend class FRHIMetal;
	};
}

#endif	// COMPILE_WITH_RHI_METAL
