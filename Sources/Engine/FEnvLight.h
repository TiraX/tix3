/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	// FEnvLight: Hold indirect light info
	// Filtered cubemap for indirect specular, and qiuxie for indirect diffuse
	class FEnvLight : public FRenderResource
	{
	public:
		FEnvLight(FTexturePtr InCubemap, const FFloat3& InPosition);
		virtual ~FEnvLight();

		FTexturePtr GetEnvCubemap()
		{
			return EnvCubemap;
		}

		FRenderResourceTablePtr GetResourceTable()
		{
			return ResourceTable;
		}

	private:
		FTexturePtr EnvCubemap;
		FFloat3 Position;

		FRenderResourceTablePtr ResourceTable;
	};
}
