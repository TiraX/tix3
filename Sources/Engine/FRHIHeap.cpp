/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "FRHIHeap.h"

namespace tix
{
	FRHIHeap::FRHIHeap(uint32 InId, EResourceHeapType InType)
		: HeapId(InId)
		, HeapType(InType)
	{
	}

	FRHIHeap::~FRHIHeap()
	{
	}
}