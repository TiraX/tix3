/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	class FRHI;

	// Renderer interface
	class TI_API FFullScreenRender
	{
	public: 
		FFullScreenRender();
		~FFullScreenRender();

		FShaderPtr GetFullScreenShader()
		{
			return FullScreenShader;
		}

		void InitCommonResources(FRHI* RHI);
		void DrawFullScreenTexture(FRHI* RHI, FTexturePtr Texture);
		// Blit the full screen texture to screen directly.
		void DrawFullScreenTexture(FRHI* RHI, FRenderResourceTablePtr TextureTable);
		// Blit the full screen texture to screen directly.
		void DrawFullScreenTexture(FRHI* RHI, FArgumentBufferPtr ArgumentBuffer);
		// Blit a full screen quad. Need to setup pipeline first manually.
		void DrawFullScreenQuad(FRHI* RHI);

	protected:
		bool bInited;
		// Common Resources
		struct FullScreenVertex
		{
			FFloat3 Position;
			FHalf2 UV;
		};
		FMeshBufferPtr FullScreenQuad;
		FPipelinePtr FullScreenPipeline;
		FShaderPtr FullScreenShader;
	};
}
