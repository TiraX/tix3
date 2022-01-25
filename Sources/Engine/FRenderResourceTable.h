/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	class FRenderResourceHeap;
	class TI_API FRenderResourceTable : public FRenderResource
	{
	public:
		FRenderResourceTable(uint32 InSize);
		FRenderResourceTable(FRenderResourceHeap * InHeap, uint32 InStart, uint32 InSize);
		~FRenderResourceTable();

		void PutConstantBufferInTable(FUniformBufferPtr InUniformBuffer, uint32 Index);
		void PutTextureInTable(FTexturePtr InTexture, uint32 Index);
		void PutRWTextureInTable(FTexturePtr InTexture, uint32 MipLevel, uint32 Index);
		void PutUniformBufferInTable(FUniformBufferPtr InBuffer, uint32 Index);
		void PutRWUniformBufferInTable(FUniformBufferPtr InBuffer, uint32 Index);
		void PutVertexBufferInTable(FVertexBufferPtr InBuffer, int32 VBIndex);
		void PutIndexBufferInTable(FIndexBufferPtr InBuffer, int32 IBIndex);
		void PutInstanceBufferInTable(FInstanceBufferPtr InBuffer, uint32 Index);
		void PutRTColorInTable(FTexturePtr InTexture, uint32 Index);
		void PutRTDepthInTable(FTexturePtr InTexture, uint32 Index);
		void PutTopLevelAccelerationStructureInTable(FTopLevelAccelerationStructurePtr InTLAS, uint32 Index);

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

		EResourceHeapType GetHeapType() const;
	protected:
		EResourceHeapType HeapType;
		uint32 Start;
		uint32 Size;

		friend class FRenderResourceHeap;
	};
}