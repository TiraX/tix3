/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

#include "FHeapAllocator.h"

namespace tix
{
	enum FHeapSize
	{
		MAX_HEAP_SRV_CBV = 4096,

		MAX_HEAP_SAMPLERS = 16,
		MAX_HEAP_RENDERTARGETS = 128,
		MAX_HEAP_DEPTHSTENCILS = 32,
	};

	class FRHIHeap
	{
	public:
		FRHIHeap(uint32 InId, EResourceHeapType InType);
		virtual ~FRHIHeap();

		virtual FRenderResourceTablePtr CreateRenderResourceTable(uint32 InSize) = 0;
		virtual void RecallRenderResourceTable(uint32 InStart, uint32 InSize) = 0;

		uint32 GetHeapId() const
		{
			return HeapId;
		}
		EResourceHeapType GetHeapType() const
		{
			return HeapType;
		}
	protected:
		uint32 HeapId;
		EResourceHeapType HeapType;
	};
}