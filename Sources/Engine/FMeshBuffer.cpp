/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"

namespace tix
{
	FVertexBuffer::FVertexBuffer()
		: FRenderResource(RRT_VERTEX_BUFFER)
	{
	}

	FVertexBuffer::FVertexBuffer(const TVertexBufferDesc& InDesc)
		: FRenderResource(RRT_VERTEX_BUFFER)
		, Desc(InDesc)
	{
		Desc.Stride = TVertexBuffer::GetStrideFromFormat(InDesc.VsFormat);
	}

	FVertexBuffer::~FVertexBuffer()
	{
	}

	void FVertexBuffer::CreateGPUResource(TStreamPtr Data)
	{
		TI_ASSERT(IsRenderThread());
		TI_ASSERT(GPUResourceVertexBuffer == nullptr);
		FRHI* RHI = FRHI::Get();

		FGPUResourceDesc VBDesc;
		TI_ASSERT(0);	// todo: create gpu resource flag here. like uniform buffer flag
		VBDesc.Flag = 0;
		VBDesc.BufferSize = Desc.VertexCount * Desc.Stride;

		GPUResourceVertexBuffer = RHI->CreateGPUResourceBuffer();
		GPUResourceVertexBuffer->Init(VBDesc, Data);
	}

	///////////////////////////////////////////////////////////
	FIndexBuffer::FIndexBuffer()
		: FRenderResource(RRT_INDEX_BUFFER)
	{
	}

	FIndexBuffer::FIndexBuffer(const TIndexBufferDesc& InDesc)
		: FRenderResource(RRT_INDEX_BUFFER)
		, Desc(InDesc)
	{
	}

	FIndexBuffer::~FIndexBuffer()
	{
	}

	void FIndexBuffer::CreateGPUResource(TStreamPtr Data)
	{
		TI_ASSERT(IsRenderThread());
		TI_ASSERT(GPUResourceIndexBuffer == nullptr);
		FRHI* RHI = FRHI::Get();

		TI_ASSERT(0);	// todo: create gpu resource flag here. like uniform buffer flag
		FGPUResourceDesc IBDesc;
		IBDesc.Flag = 0;
		IBDesc.BufferSize = Desc.IndexCount * (Desc.IndexType == EIT_16BIT ? sizeof(uint16) : sizeof(uint32));

		GPUResourceIndexBuffer = RHI->CreateGPUResourceBuffer();
		GPUResourceIndexBuffer->Init(IBDesc, Data);
	}

	///////////////////////////////////////////////////////////
	FInstanceBuffer::FInstanceBuffer()
		: FRenderResource(RRT_INSTANCE_BUFFER)
		, InstanceCount(0)
		, Stride(0)
	{
	}

	FInstanceBuffer::FInstanceBuffer(uint32 TotalInstancesCount, uint32 InstanceStride)
		: FRenderResource(RRT_INSTANCE_BUFFER)
		, InstanceCount(TotalInstancesCount)
		, Stride(InstanceStride)
	{
	}

	FInstanceBuffer::~FInstanceBuffer()
	{
	}

	void FInstanceBuffer::SetFromTInstanceBuffer(TInstanceBufferPtr InInstanceData)
	{
		InstanceCount = InInstanceData->GetDesc().InstanceCount;
		Stride = InInstanceData->GetDesc().Stride;
	}
}