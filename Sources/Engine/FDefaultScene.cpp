/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "FDefaultScene.h"

namespace tix
{
	FDefaultScene::FDefaultScene()
		: SceneFlags(0)
	{
		// Reserve memory for containers
		Primitives.reserve(128);
	}

	FDefaultScene::~FDefaultScene()
	{
	}

	void FDefaultScene::SetViewProjection(const FViewProjectionInfo& Info)
	{
		TI_ASSERT(IsRenderThread());
		ViewProjection = Info;
		SetSceneFlag(ViewProjectionDirty);
	}

	void FDefaultScene::SetEnvironmentInfo(const FEnvironmentInfo& Info)
	{
		TI_ASSERT(IsRenderThread());
		EnvInfo = Info;
		SetSceneFlag(EnvironmentDirty);
	}

	void FDefaultScene::SetEnvLight(FEnvLightPtr InEnvLight)
	{
		EnvLight = InEnvLight;
	}

	FEnvLightPtr FDefaultScene::GetEnvLight()
	{
		return EnvLight;
	}

	void FDefaultScene::AddPrimitive(FPrimitivePtr InPrim)
	{
		Primitives.push_back(InPrim);
	}
}
