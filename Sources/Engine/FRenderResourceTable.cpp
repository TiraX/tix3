/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "FRenderResourceTable.h"

namespace tix
{
	FRenderResourceTable::FRenderResourceTable()
		: FRenderResource(ERenderResourceType::ResourceTable)
		, HeapType(EResourceHeapType::ShaderResource)
		, HeapId(uint32(-1))
		, Start(uint32(-1))
		, Size(0)
	{
	}

	FRenderResourceTable::FRenderResourceTable(EResourceHeapType HeapType, uint32 InHeapId, uint32 InStart, uint32 InSize)
		: FRenderResource(ERenderResourceType::ResourceTable)
		, HeapType(HeapType)
		, HeapId(InHeapId)
		, Start(InStart)
		, Size(InSize)
	{
		ResInTable.reserve(InSize);
	}

	FRenderResourceTable::~FRenderResourceTable()
	{
		TI_ASSERT(IsRenderThread());
		FRHIHeap* Heap = FRHI::Get()->GetHeapById(HeapId);
		TI_ASSERT(Heap->GetHeapId() == HeapId && Heap->GetHeapType() == HeapType);
		Heap->RecallRenderResourceTable(Start, Size);
	}
}