/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "FAccelerationStructure.h"

namespace tix
{
	FAccelerationStructure::FAccelerationStructure()
		: FRenderResource(ERenderResourceType::AccelerationStructure)
		, Dirty(true)
	{
	}

	FAccelerationStructure::~FAccelerationStructure()
	{
	}

	/////////////////////////////////////////////////////////////
	FBottomLevelAccelerationStructure::FBottomLevelAccelerationStructure()
	{
	}

	FBottomLevelAccelerationStructure::~FBottomLevelAccelerationStructure()
	{
	}

	/////////////////////////////////////////////////////////////
	FTopLevelAccelerationStructure::FTopLevelAccelerationStructure()
	{
	}

	FTopLevelAccelerationStructure::~FTopLevelAccelerationStructure()
	{
	}
}
