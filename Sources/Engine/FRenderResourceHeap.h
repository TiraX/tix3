/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once
#include "FRenderResourceTable.h"

namespace tix
{
	enum 
	{
		MAX_HEAP_SRV_CBV = 4096,

		MAX_HEAP_SAMPLERS = 16,
		MAX_HEAP_RENDERTARGETS = 128,
		MAX_HEAP_DEPTHSTENCILS = 32,
	};
	class FRenderResourceHeap
	{
	public:
		FRenderResourceHeap();
		~FRenderResourceHeap();

		void Create(EResourceHeapType HeapType, uint32 HeapSize, uint32 HeapOffset);

		//TI_API FRenderResourceTablePtr AllocateTable(uint32 Size);
		TI_API void InitResourceTable(FRenderResourceTablePtr OutTable);

		TI_API void RecallTable(const FRenderResourceTable& Table);

		EResourceHeapType GetHeapType() const
		{
			return HeapType;
		}
		int32 GetHeapSize() const
		{
			return Size;
		}

	private:

	private:
		EResourceHeapType HeapType;
		THMap<uint32, TVector<uint32>> AvaibleHeapTables;
		uint32 Allocated;
		uint32 Size;
		uint32 Offset;

		friend class FRenderResourceTable;
	};
}