/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "FHeapAllocator.h"

namespace tix
{
	FHeapAllocator::FHeapAllocator()
		: Allocated(0)
		, Size(0)
	{
		AvaibleHeapTables.reserve(32);
	}

	FHeapAllocator::~FHeapAllocator()
	{
	}

	void FHeapAllocator::Init(uint32 InHeapSize)
	{
		TI_ASSERT(Size == 0 && Allocated == 0 && AvaibleHeapTables.empty());
		Size = InHeapSize;
	}

	uint32 FHeapAllocator::AllocateTable(uint32 TableSize)
	{
		TI_ASSERT(TableSize != 0);
		TVector<uint32>& Avaibles = AvaibleHeapTables[TableSize];

		if (Avaibles.size() > 0)
		{
			uint32 StartIndex = Avaibles.back();
			Avaibles.pop_back();
			return StartIndex;
		}
		uint32 Result = Allocated;
		Allocated += TableSize;
		TI_ASSERT(Allocated <= Size);
		return Result;
	}

	void FHeapAllocator::RecallTable(uint32 StartIndex, uint32 Size)
	{
		TI_ASSERT(IsRenderThread());
		TVector<uint32>& Avaibles = AvaibleHeapTables[Size];
		Avaibles.push_back(StartIndex);
	}
}