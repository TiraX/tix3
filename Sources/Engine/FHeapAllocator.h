/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once
#include "FRenderResourceTable.h"

namespace tix
{
	class FHeapAllocator
	{
	public:
		FHeapAllocator();
		~FHeapAllocator();

		void Init(uint32 InHeapSize);

		// Allocate a table with 'Size', return heap index
		uint32 AllocateTable(uint32 Size);
		void RecallTable(uint32 StartIndex, uint32 Size);

		int32 GetHeapSize() const
		{
			return Size;
		}

	private:

	private:
		THMap<uint32, TVector<uint32>> AvaibleHeapTables;
		uint32 Allocated;
		uint32 Size;
	};
}