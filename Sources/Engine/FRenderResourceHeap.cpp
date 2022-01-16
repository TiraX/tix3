/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "FRenderResourceHeap.h"

namespace tix
{
	FRenderResourceHeap::FRenderResourceHeap()
		: HeapType(EResourceHeapType::None)
		, Allocated(0)
		, Size(0)
		, Offset(0)
	{
	}

	FRenderResourceHeap::~FRenderResourceHeap()
	{
	}

	void FRenderResourceHeap::Create(EResourceHeapType InHeapType, uint32 HeapSize, uint32 HeapOffset)
	{
		HeapType = InHeapType;
		Size = HeapSize;
		Offset = HeapOffset;
	}

	//FRenderResourceTablePtr FRenderResourceHeap::AllocateTable(uint32 TableSize)
	//{
	//	FRenderResourceTablePtr ResourceTable = FRHI::Get()->CreateRenderResourceTable(TableSize);
	//	InitResourceTable(ResourceTable);
	//	return ResourceTable;
	//}

	void FRenderResourceHeap::InitResourceTable(FRenderResourceTablePtr OutTable)
	{
		TI_ASSERT(IsRenderThread());
		const uint32 TableSize = OutTable->GetTableSize();
		TI_ASSERT(TableSize != 0);
		TVector<uint32>& Avaibles = AvaibleHeapTables[TableSize];

		if (Avaibles.size() > 0)
		{
			uint32 StartIndex = Avaibles.back();
			Avaibles.pop_back();
			OutTable->HeapType = HeapType;
			OutTable->Start = StartIndex;
			OutTable->Size = TableSize;
		}
		uint32 Result = Allocated + Offset;
		Allocated += TableSize;
		TI_ASSERT(Allocated <= Size);
		OutTable->HeapType = HeapType;
		OutTable->Start = Result;
		OutTable->Size = TableSize;
	}

	void FRenderResourceHeap::RecallTable(const FRenderResourceTable& Table)
	{
		TI_ASSERT(IsRenderThread());
		TVector<uint32>& Avaibles = AvaibleHeapTables[Table.GetTableSize()];
		Avaibles.push_back(Table.GetStartIndex());
	}
}