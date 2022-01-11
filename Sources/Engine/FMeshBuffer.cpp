/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"

namespace tix
{
	FMeshBuffer::FMeshBuffer()
		: FRenderResource(RRT_VERTEX_BUFFER)
	{
	}

	FMeshBuffer::FMeshBuffer(const TMeshBufferDesc& InDesc)
		: FRenderResource(RRT_VERTEX_BUFFER)
		, Desc(InDesc)
	{
		Desc.Stride = TMeshBuffer::GetStrideFromFormat(InDesc.VsFormat);
	}

	FMeshBuffer::~FMeshBuffer()
	{
	}

	void FMeshBuffer::CreateGPUResource(TStreamPtr Data)
	{
		TI_ASSERT(IsRenderThread());
		TI_ASSERT(0);
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