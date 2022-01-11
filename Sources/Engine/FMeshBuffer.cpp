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

	FMeshBuffer::FMeshBuffer(
		E_PRIMITIVE_TYPE InPrimType,
		uint32 InVSFormat, 
		uint32 InVertexCount, 
		E_INDEX_TYPE InIndexType,
		uint32 InIndexCount,
		const FBox& InBBox
	)
		: FRenderResource(RRT_VERTEX_BUFFER)
	{
		Desc.PrimitiveType = InPrimType;
		Desc.VertexCount = InVertexCount;
		Desc.IndexType = InIndexType;
		Desc.IndexCount = InIndexCount;
		Desc.VsFormat = InVSFormat;
		Desc.BBox = InBBox;
		Desc.Stride = TMeshBuffer::GetStrideFromFormat(InVSFormat);
	}

	FMeshBuffer::~FMeshBuffer()
	{
	}

	void FMeshBuffer::SetFromTMeshBuffer(TMeshBufferPtr InMeshBuffer)
	{
		Desc = InMeshBuffer->GetDesc();
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