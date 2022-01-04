/*
	TiX Engine v2.0 Copyright (C) 2018~2021
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "FEnvLight.h"

namespace tix
{
	FEnvLight::FEnvLight(FTexturePtr InCubemap, const vector3df& InPosition)
		: FRenderResource(RRT_ENV_LIGHT)
		, EnvCubemap(InCubemap)
		, Position(InPosition)
	{
		ResourceTable = FRHI::Get()->CreateRenderResourceTable(1, EHT_SHADER_RESOURCE);
		ResourceTable->PutTextureInTable(EnvCubemap, 0);
	}

	FEnvLight::~FEnvLight()
	{
	}
}