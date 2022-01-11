/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "FRHI.h"
#include "FFrameResources.h"

namespace tix
{
	FFrameResources::FFrameResources()
	{
		//Resources.reserve(DefaultReserveCount);
	}

	FFrameResources::~FFrameResources()
	{
		RemoveAllReferences();
	}

	void FFrameResources::RemoveAllReferences()
	{
		for (auto& R : Resources)
		{
			R = nullptr;
		}
		Resources.clear();
	}

	void FFrameResources::HoldReference(FRenderResourcePtr InResource)
	{
		Resources.push_back(InResource);
	}
}