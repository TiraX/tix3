/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"

namespace tix
{
	FTexture::FTexture()
		: FRenderResource(ERenderResourceType::Texture)
	{
	}

	FTexture::FTexture(const TTextureDesc& Desc)
		: FRenderResource(ERenderResourceType::Texture)
	{
		TextureDesc = Desc;
	}

	FTexture::~FTexture()
	{
		TI_ASSERT(IsRenderThread());
	}

	void FTexture::InitTextureInfo(TTexturePtr InTexture)
	{
		TextureDesc = InTexture->GetDesc();
	}
}