/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "FArgumentBuffer.h"

namespace tix
{
	FArgumentBuffer::FArgumentBuffer(int32 ReservedSlots)
		: FRenderResource(ERenderResourceType::ArgumentBuffer)
	{
		Arguments.resize(ReservedSlots);
	}

	FArgumentBuffer::~FArgumentBuffer()
	{
	}

	void FArgumentBuffer::SetBuffer(int32 Index, FUniformBufferPtr InUniform)
	{
		TI_ASSERT(Index < MaxResourcesInArgumentBuffer);
		if (Arguments.size() <= Index)
		{
			Arguments.resize(Index + 1);
		}
		Arguments[Index] = InUniform;
	}

	void FArgumentBuffer::SetTexture(int32 Index, FTexturePtr InTexture)
	{
		TI_ASSERT(Index < MaxResourcesInArgumentBuffer);
		if (Arguments.size() <= Index)
		{
			Arguments.resize(Index + 1);
		}
		Arguments[Index] = InTexture;
	}
}