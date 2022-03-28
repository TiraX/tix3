/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	class TI_API FRenderResourceTable : public FRenderResource
	{
	public:
		FRenderResourceTable();
		FRenderResourceTable(EResourceHeapType InHeapType, uint32 InHeapId, uint32 InStart, uint32 InSize);
		virtual ~FRenderResourceTable();

		uint32 GetStartIndex() const
		{
			return Start;
		}

		uint32 GetIndexAt(uint32 Index) const
		{
			TI_ASSERT(Index < Size);
			return Start + Index;
		}

		uint32 GetTableSize() const
		{
			return Size;
		}

		uint32 GetHeapId() const
		{
			return HeapId;
		}

		EResourceHeapType GetHeapType() const
		{
			return HeapType;
		}

		void HoldResource(FRenderResourcePtr Res)
		{
			ResInTable.push_back(Res);
		}
	protected:
		EResourceHeapType HeapType;
		uint32 HeapId;
		uint32 Start;
		uint32 Size;
		TVector<FRenderResourcePtr> ResInTable;

		friend class FRenderResourceHeap;
	};
}