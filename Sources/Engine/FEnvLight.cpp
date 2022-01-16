/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "FEnvLight.h"

namespace tix
{
	FEnvLight::FEnvLight(FTexturePtr InCubemap, const FFloat3& InPosition)
		: FRenderResource(ERenderResourceType::EnvLight)
		, EnvCubemap(InCubemap)
		, Position(InPosition)
	{
		ResourceTable = FRHI::Get()->CreateRenderResourceTable(1, EResourceHeapType::ShaderResource);
		ResourceTable->PutTextureInTable(EnvCubemap, 0);
	}

	FEnvLight::~FEnvLight()
	{
	}
}