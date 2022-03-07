/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	class FRHICmdList;

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

		void InitCommonResources(FRHICmdList* RHICmdList);
		void DrawFullScreenTexture(FRHICmdList* RHICmdList, FTexturePtr Texture);
		// Blit the full screen texture to screen directly.
		void DrawFullScreenTexture(FRHICmdList* RHICmdList, FRenderResourceTablePtr TextureTable);
		// Blit the full screen texture to screen directly.
		void DrawFullScreenTexture(FRHICmdList* RHICmdList, FArgumentBufferPtr ArgumentBuffer);
		// Blit a full screen quad. Need to setup pipeline first manually.
		void DrawFullScreenQuad(FRHICmdList* RHICmdList);

	protected:
		bool bInited;
		// Common Resources
		struct FullScreenVertex
		{
			FFloat3 Position;
			FHalf2 UV;
		};
		FVertexBufferPtr FullScreenVB;
		FIndexBufferPtr FullScreenIB;
		FPipelinePtr FullScreenPipeline;
		FShaderPtr FullScreenShader;
	};
}
