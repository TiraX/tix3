/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	class FRHI;
	class FScene;

	// Default Renderer
	class TI_API FDefaultRenderer : public FRenderer
	{
	public:
		FDefaultRenderer();
		virtual ~FDefaultRenderer();

		virtual void InitInRenderThread() override;
		virtual void InitRenderFrame(FScene* Scene) override;
		virtual void EndRenderFrame(FScene* Scene) override;
		virtual void Render(FRHI* RHI, FScene* Scene) override;

		virtual void ApplyShaderParameter(FRHI * RHI, FScene * Scene, FPrimitivePtr Primitive, int32 SectionIndex);

		virtual FUniformBufferPtr GetCounterResetUniformBuffer() override
		{
			return CounterResetUniformBuffer;
		}

	protected:
		void BindEngineBuffer(FRHI * RHI, E_SHADER_STAGE ShaderStage, const FShaderBinding::FShaderArgument& Argument, FScene * Scene, FPrimitivePtr Primitive, int32 SectionIndex);
		//void BindMaterialInstanceArgument(FRHI * RHI, FShaderBindingPtr InShaderBinding, FArgumentBufferPtr ArgumentBuffer);

		void DrawSceneTiles(FRHI* RHI, FScene* Scene);

	protected:
		// Common Resources
		FUniformBufferPtr CounterResetUniformBuffer;
	};
}
