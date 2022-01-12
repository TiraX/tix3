/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "FUniformBuffer.h"

namespace tix
{
	FUniformBuffer::FUniformBuffer(uint32 InStructureSizeInBytes, uint32 InElements, uint32 InUBFlag)
		: FRenderResource(ERenderResourceType::UniformBuffer)
		, StructureSizeInBytes(InStructureSizeInBytes)
		, Elements(InElements)
		, Flag(InUBFlag)
	{
	}

	FUniformBuffer::~FUniformBuffer()
	{
	}
}