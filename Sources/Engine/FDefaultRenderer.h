/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	class FRHI;

	// Default Renderer
	class TI_API FDefaultRenderer : public FRendererInterface
	{
	public:
		FDefaultRenderer(FSceneInterface* InScene);
		virtual ~FDefaultRenderer();

		virtual void InitInRenderThread() override;
		virtual void InitRenderFrame() override;
		virtual void EndRenderFrame() override;
		virtual void Render(FRHICmdList* RHICmdList) override;

		virtual void ApplyShaderParameter(FRHICmdList* RHICmdList, FPrimitivePtr Primitive, int32 SectionIndex);

		virtual FUniformBufferPtr GetCounterResetUniformBuffer() override
		{
			return CounterResetUniformBuffer;
		}

	protected:
		void BindEngineBuffer(FRHICmdList* RHICmdList, E_SHADER_STAGE ShaderStage, const FShaderBinding::FShaderArgument& Argument, FPrimitivePtr Primitive, int32 SectionIndex);
		//void BindMaterialInstanceArgument(FRHI * RHI, FShaderBindingPtr InShaderBinding, FArgumentBufferPtr ArgumentBuffer);

		void DrawPrimitives(FRHICmdList* RHICmdList);
		void PrepareViewUniforms(FRHICmdList* RHICmdList);

	protected:
		// Scene to render
		FDefaultScene* Scene;

		// Uniform buffers
		FViewUniformBufferPtr ViewUniformBuffer;

		// Common Resources
		FUniformBufferPtr CounterResetUniformBuffer;

		friend class FDefaultScene;
	};
}
