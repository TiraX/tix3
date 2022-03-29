/*
TiX Engine v3.0 Copyright (C) 2022~2025
By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	
	enum E_COMPUTETASK_FLAG
	{
		COMPUTE_NONE = 0,
		
		// Tile shader used in metal iOS. Do not have effect on other RHIs(Dx12, Vulkan)
		COMPUTE_TILE = 1
	};
	
	class TI_API FComputeTask : public IReferenceCounted
	{
	public:
		FComputeTask(const TString& ComputeShaderName, uint32 InFlags = COMPUTE_NONE);
		virtual ~FComputeTask();

		void Finalize();
		virtual void Run(FRHICmdList * RHICmdList) = 0;
		
		bool HasFlag(uint32 InFlag) const
		{
			return (Flags & InFlag) != 0;
		}
		
		// For metal
		void SetTilePipelineDesc(const TTilePipelinePtr& InDesc)
		{
			TilePLDesc = InDesc;
		}

	protected:
		virtual void FinalizeInRenderThread();

	protected:
		TString ShaderName;
		uint32 Flags;
		
		FPipelinePtr ComputePipeline;
		
		TTilePipelinePtr TilePLDesc;
	};
}
