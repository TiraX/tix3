/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "FRHIDx12.h"
#include "FArgumentBufferDx12.h"

#if COMPILE_WITH_RHI_DX12

namespace tix
{
	FArgumentBufferDx12::FArgumentBufferDx12(int32 ReservedSlots)
		: FArgumentBuffer(ReservedSlots)
	{
	}

	FArgumentBufferDx12::~FArgumentBufferDx12()
	{
		TI_ASSERT(IsRenderThread());
		ResourceTable = nullptr;
	}
}

#endif	// COMPILE_WITH_RHI_DX12