/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "FRenderResourceTable.h"

namespace tix
{
	FRenderResourceTable::FRenderResourceTable(uint32 InSize)
		: FRenderResource(ERenderResourceType::ResourceTable)
		, HeapType(EResourceHeapType::None)
		, Start(uint32(-1))
		, Size(InSize)
	{
	}

	FRenderResourceTable::FRenderResourceTable(FRenderResourceHeap * InHeap, uint32 InStart, uint32 InSize)
		: FRenderResource(ERenderResourceType::ResourceTable)
		, HeapType(InHeap->GetHeapType())
		, Start(InStart)
		, Size(InSize)
	{
	}

	FRenderResourceTable::~FRenderResourceTable()
	{
		TI_ASSERT(IsRenderThread());
		FRHI::Get()->GetRenderResourceHeap(HeapType).RecallTable(*this);
	}

	void FRenderResourceTable::PutConstantBufferInTable(FUniformBufferPtr InUniformBuffer, uint32 Index)
	{
		TI_ASSERT(HeapType == EResourceHeapType::ShaderResource);
		TI_ASSERT(Index < Size);
		FRHI::Get()->PutConstantBufferInHeap(InUniformBuffer, HeapType, Start + Index);
	}

	void FRenderResourceTable::PutTextureInTable(FTexturePtr InTexture, uint32 Index)
	{
		TI_ASSERT(HeapType == EResourceHeapType::ShaderResource);
		TI_ASSERT(Index < Size);
		FRHI::Get()->PutTextureInHeap(InTexture, HeapType, Start + Index);
	}

	void FRenderResourceTable::PutRWTextureInTable(FTexturePtr InTexture, uint32 MipLevel, uint32 Index)
	{
		TI_ASSERT(HeapType == EResourceHeapType::ShaderResource);
		TI_ASSERT(Index < Size);
		FRHI::Get()->PutRWTextureInHeap(InTexture, MipLevel, HeapType, Start + Index);
	}

	void FRenderResourceTable::PutUniformBufferInTable(FUniformBufferPtr InBuffer, uint32 Index)
	{
		TI_ASSERT(HeapType == EResourceHeapType::ShaderResource);
		TI_ASSERT(Index < Size);
		FRHI::Get()->PutUniformBufferInHeap(InBuffer, HeapType, Start + Index);
	}

	void FRenderResourceTable::PutRWUniformBufferInTable(FUniformBufferPtr InBuffer, uint32 Index)
	{
		TI_ASSERT(HeapType == EResourceHeapType::ShaderResource);
		TI_ASSERT(Index < Size);
		FRHI::Get()->PutRWUniformBufferInHeap(InBuffer, HeapType, Start + Index);
	}

	void FRenderResourceTable::PutVertexBufferInTable(FVertexBufferPtr InBuffer, int32 VBIndex)
	{
		TI_ASSERT(HeapType == EResourceHeapType::ShaderResource);
		TI_ASSERT(VBIndex < (int32)Size);
		if (VBIndex >= 0)
			VBIndex = Start + VBIndex;
		FRHI::Get()->PutVertexBufferInHeap(InBuffer, HeapType, VBIndex);
	}

	void FRenderResourceTable::PutIndexBufferInTable(FIndexBufferPtr InBuffer, int32 IBIndex)
	{
		TI_ASSERT(HeapType == EResourceHeapType::ShaderResource);
		TI_ASSERT(IBIndex < (int32)Size);
		if (IBIndex >= 0)
			IBIndex = Start + IBIndex;
		FRHI::Get()->PutIndexBufferInHeap(InBuffer, HeapType, IBIndex);
	}

	void FRenderResourceTable::PutInstanceBufferInTable(FInstanceBufferPtr InBuffer, uint32 Index)
	{
		TI_ASSERT(HeapType == EResourceHeapType::ShaderResource);
		TI_ASSERT(Index < Size);
		FRHI::Get()->PutInstanceBufferInHeap(InBuffer, HeapType, Start + Index);
	}

	void FRenderResourceTable::PutRTColorInTable(FTexturePtr InTexture, uint32 Index)
	{
		TI_ASSERT(HeapType == EResourceHeapType::RenderTarget);
		TI_ASSERT(InTexture->GetDesc().Mips > 0 && Index * InTexture->GetDesc().Mips < Size);
		FRHI::Get()->PutRTColorInHeap(InTexture, Start + Index * InTexture->GetDesc().Mips);
	}

	void FRenderResourceTable::PutRTDepthInTable(FTexturePtr InTexture, uint32 Index)
	{
		TI_ASSERT(HeapType == EResourceHeapType::DepthStencil);
		TI_ASSERT(InTexture->GetDesc().Mips > 0 && Index * InTexture->GetDesc().Mips < Size);
		FRHI::Get()->PutRTDepthInHeap(InTexture, Start + Index * InTexture->GetDesc().Mips);
	}

	void FRenderResourceTable::PutTopLevelAccelerationStructureInTable(
		FTopLevelAccelerationStructurePtr InTLAS, 
		uint32 Index)
	{
		TI_ASSERT(HeapType == EResourceHeapType::ShaderResource);
		TI_ASSERT(Index < Size);
		FRHI::Get()->PutTopAccelerationStructureInHeap(InTLAS, HeapType, Start + Index);
	}

	EResourceHeapType FRenderResourceTable::GetHeapType() const
	{
		return HeapType;
	}
}