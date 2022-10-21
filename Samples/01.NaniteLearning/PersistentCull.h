/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once
//BEGIN_UNIFORM_BUFFER_STRUCT(FGTAOUniform)
//	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FFloat4, ScreenSize)	// xy = Size; zw = InvSize;
//	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FFloat4, FocalLen)		// xy = FocalLen; zw = InvFocalLen;
//	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FFloat4, Radius)		// x = radius; y = radius^2; z = 1.0/radius
//END_UNIFORM_BUFFER_STRUCT(FGTAOUniform)

class FPersistentCull : public FComputeTask
{
public:
	FPersistentCull();
	virtual ~FPersistentCull();

	virtual void Run(FRHICmdList* RHICmdList) override;

private:
	//enum
	//{
	//	SRV_SCENE_NORMAL,
	//	SRV_SCENE_DEPTH,
	//	SRV_RANDOM_TEXTURE,

	//	UAV_AO_RESULT,

	//	PARAM_TOTAL_COUNT,
	//};

private:
	//FGTAOUniformPtr InfoUniform;

	//FRenderResourceTablePtr ResourceTable;

	//FTexturePtr SceneNormal;
	//FTexturePtr SceneDepth;
	//FTexturePtr RandomTex;

	//float FoV;

	//FTexturePtr AOTexture;
};
