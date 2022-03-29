/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	class FRHI;

	struct FShadowSettings
	{
		int32 W, H;
		float Bias;

		FShadowSettings()
			: W(1024)
			, H(1024)
			, Bias(0.005f)
		{}
	};

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
		void SetShadowSize(int32 W, int32 H)
		{
			ShadowSettings.W = W;
			ShadowSettings.H = H;
		}
		void SetShadowBias(float Bias)
		{
			ShadowSettings.Bias = Bias;
		}

	protected:
		void BindEngineBuffer(FRHICmdList* RHICmdList, E_SHADER_STAGE ShaderStage, const FShaderBinding::FShaderArgument& Argument, FPrimitivePtr Primitive, int32 SectionIndex);
		//void BindMaterialInstanceArgument(FRHI * RHI, FShaderBindingPtr InShaderBinding, FArgumentBufferPtr ArgumentBuffer);

		void DrawPrimitives(FRHICmdList* RHICmdList);
		void PrepareViewUniforms(FRHICmdList* RHICmdList);
		void CreateShadowmap();
		void RenderShadowMap(FRHICmdList* RHICmdList);

	protected:
		// Scene to render
		FDefaultScene* Scene;

		// Uniform buffers
		FViewUniformBufferPtr ViewUniformBuffer;

		// Common Resources
		FUniformBufferPtr CounterResetUniformBuffer;

		// Shadow map
		FShadowSettings ShadowSettings;
		FRenderTargetPtr RT_ShadowPass;
		FTexturePtr Shadowmap;
		FRenderResourceTablePtr ShadowmapTable;

		friend class FDefaultScene;
	};
}
