	/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	class TEngineResources
	{
	public:
		static TTexturePtr EmptyTextureWhite;
		static TTexturePtr EmptyTextureBlack;
		static TTexturePtr EmptyTextureNormal;

		static void CreateGlobalResources();
		static void DestroyGlobalResources();
	};
}
