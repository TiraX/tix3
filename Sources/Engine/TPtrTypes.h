/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
#define DECLARE_INSTRUSIVE_PTR(ClassName) \
	class ClassName; \
	typedef TI_INTRUSIVE_PTR(ClassName) ClassName##Ptr

	DECLARE_INSTRUSIVE_PTR(TAssetFile);
	DECLARE_INSTRUSIVE_PTR(TImage);
	DECLARE_INSTRUSIVE_PTR(TStream);
	DECLARE_INSTRUSIVE_PTR(TMaterial);
	DECLARE_INSTRUSIVE_PTR(TMaterialInstance);
	DECLARE_INSTRUSIVE_PTR(TMeshBuffer);
	DECLARE_INSTRUSIVE_PTR(TSkeleton);
	DECLARE_INSTRUSIVE_PTR(TAnimSequence);
	DECLARE_INSTRUSIVE_PTR(TCollisionSet);
	DECLARE_INSTRUSIVE_PTR(TInstanceBuffer);
	DECLARE_INSTRUSIVE_PTR(TResource);
	DECLARE_INSTRUSIVE_PTR(TAsset);
    DECLARE_INSTRUSIVE_PTR(TPipeline);
    DECLARE_INSTRUSIVE_PTR(TTilePipeline);
	DECLARE_INSTRUSIVE_PTR(TTexture);
	DECLARE_INSTRUSIVE_PTR(TShader);
	DECLARE_INSTRUSIVE_PTR(TSceneTileResource);
	DECLARE_INSTRUSIVE_PTR(TStaticMesh);
	DECLARE_INSTRUSIVE_PTR(TRtxPipeline);	
	DECLARE_INSTRUSIVE_PTR(FRenderResource);
	DECLARE_INSTRUSIVE_PTR(FRenderResourceTable);
	DECLARE_INSTRUSIVE_PTR(FTexture);
	DECLARE_INSTRUSIVE_PTR(FPipeline);
	DECLARE_INSTRUSIVE_PTR(FMeshBuffer);
	DECLARE_INSTRUSIVE_PTR(FEnvLight);
	DECLARE_INSTRUSIVE_PTR(FSceneTileResource);
	DECLARE_INSTRUSIVE_PTR(FInstanceBuffer);
	DECLARE_INSTRUSIVE_PTR(FGPUCommandBuffer);
	DECLARE_INSTRUSIVE_PTR(FGPUCommandSignature);
	DECLARE_INSTRUSIVE_PTR(FPrimitive);
	DECLARE_INSTRUSIVE_PTR(FLight);
	DECLARE_INSTRUSIVE_PTR(FUniformBuffer);
	DECLARE_INSTRUSIVE_PTR(FArgumentBuffer);
	DECLARE_INSTRUSIVE_PTR(FRenderTarget);
	DECLARE_INSTRUSIVE_PTR(FShaderBinding);
	DECLARE_INSTRUSIVE_PTR(FShader);
	DECLARE_INSTRUSIVE_PTR(FComputeTask);
	DECLARE_INSTRUSIVE_PTR(FRtxPipeline);
	DECLARE_INSTRUSIVE_PTR(FBottomLevelAccelerationStructure);
	DECLARE_INSTRUSIVE_PTR(FTopLevelAccelerationStructure);
	DECLARE_INSTRUSIVE_PTR(FGPUResource);
	DECLARE_INSTRUSIVE_PTR(FGPUResourceBuffer);
	DECLARE_INSTRUSIVE_PTR(FGPUResourceTexture);

#undef DECLARE_INSTRUSIVE_PTR
} // end namespace tix

