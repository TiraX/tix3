/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	class FRHI;
	class FSceneInterface;

	// Renderer interface
	class TI_API FRendererInterface
	{
	public: 
		FRendererInterface(FSceneInterface* InScene) {};
		virtual ~FRendererInterface() {};

		virtual void InitInRenderThread() = 0;
		virtual void InitRenderFrame() = 0;
		virtual void EndRenderFrame() = 0;
		virtual void Render(FRHI* RHI) = 0;

		virtual FUniformBufferPtr GetCounterResetUniformBuffer() = 0;
	protected:
	};
}
