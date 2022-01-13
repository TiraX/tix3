/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "TMeshBuffer.h"
#include "TMeshBufferSemantic.h"

namespace tix
{
	TVertexBuffer::TVertexBuffer()
		: TResource(ERES_VERTEX_BUFFER)
	{
	}

	TVertexBuffer::~TVertexBuffer()
	{
	}

	uint32 TVertexBuffer::GetStrideFromFormat(uint32 Format)
	{
		// Calculate stride
		uint32 Stride = 0;
		for (uint32 seg = 1, i = 0; seg <= EVSSEG_TOTAL; seg <<= 1, ++i)
		{
			if ((Format & seg) != 0)
			{
				Stride += TVertexBuffer::SemanticSize[i];
			}
		}
		return Stride;
	}

	TVector<E_MESH_STREAM_INDEX> TVertexBuffer::GetSteamsFromFormat(uint32 Format)
	{
		TVector<E_MESH_STREAM_INDEX> Streams;
		for (uint32 seg = 1, i = 0; seg <= EVSSEG_TOTAL; seg <<= 1, ++i)
		{
			if ((Format & seg) != 0)
			{
				Streams.push_back((E_MESH_STREAM_INDEX)i);
			}
		}
		return Streams;
	}

	void TVertexBuffer::InitRenderThreadResource()
	{
		TI_ASSERT(VertexBufferResource == nullptr);
		VertexBufferResource = ti_new FVertexBuffer(Desc);
		VertexBufferResource->SetResourceName(GetResourceName());

		FVertexBufferPtr VertexBuffer = VertexBufferResource;
		TStreamPtr InData = Data;
		ENQUEUE_RENDER_COMMAND(TVertexBufferUpdateFVertexBuffer)(
			[VertexBuffer, InData]()
			{
				VertexBuffer->CreateGPUBuffer(InData);
			});

		// release CPU memory after create render resource
		Data = nullptr;	
	}

	void TVertexBuffer::DestroyRenderThreadResource()
	{
		TI_ASSERT(VertexBufferResource != nullptr);

		FVertexBufferPtr VertexBuffer = VertexBufferResource;
		ENQUEUE_RENDER_COMMAND(TVertexBufferDestroyFVertexBuffer)(
			[VertexBuffer]()
			{
				//VertexBuffer = nullptr;
			});
		VertexBuffer = nullptr;
		VertexBufferResource = nullptr;
	}

	void TVertexBuffer::SetVertexData(
		uint32 InFormat,
		const void* InVertexData, uint32 InVertexCount,
		const FBox& InBox)
	{
		Desc.VsFormat = InFormat;
		Desc.VertexCount = InVertexCount;

		Desc.Stride = GetStrideFromFormat(InFormat);

		// Copy data
		TI_ASSERT(Data == nullptr);
		const int32 VertexDataSize = InVertexCount * Desc.Stride;
		const uint32 VertexBufferSize = TMath::Align4(VertexDataSize);
		TI_ASSERT(VertexBufferSize == VertexDataSize); // vertex always 4 bytes aligned
		Data = ti_new TStream(VertexBufferSize);
		Data->Put(InVertexData, VertexDataSize);
	}
	///////////////////////////////////////////////////////////

	TIndexBuffer::TIndexBuffer()
		: TResource(ERES_INDEX_BUFFER)
	{
	}

	TIndexBuffer::~TIndexBuffer()
	{
	}

	void TIndexBuffer::InitRenderThreadResource()
	{
		TI_ASSERT(IndexBufferResource == nullptr);
		IndexBufferResource = ti_new FIndexBuffer(Desc);
		IndexBufferResource->SetResourceName(GetResourceName());

		FIndexBufferPtr IndexBuffer = IndexBufferResource;
		TStreamPtr InData = Data;
		ENQUEUE_RENDER_COMMAND(TIndexBufferUpdateFIndexBuffer)(
			[IndexBuffer, InData]()
			{
				IndexBuffer->CreateGPUBuffer(InData);
			});

		// release CPU memory after create render resource
		Data = nullptr;
	}

	void TIndexBuffer::DestroyRenderThreadResource()
	{
		TI_ASSERT(IndexBufferResource != nullptr);

		FIndexBufferPtr IndexBuffer = IndexBufferResource;
		ENQUEUE_RENDER_COMMAND(TIndexBufferDestroyFIndexBuffer)(
			[IndexBuffer]()
			{
				//IndexBuffer = nullptr;
			});
		IndexBuffer = nullptr;
		IndexBufferResource = nullptr;
	}

	void TIndexBuffer::SetIndexData(
		E_INDEX_TYPE InIndexType,
		const void* InIndexData, uint32 InIndexCount)
	{
		Desc.IndexType = InIndexType;
		Desc.IndexCount = InIndexCount;

		// Copy data
		TI_ASSERT(Data == nullptr);
		const int32 IndexDataSize = InIndexCount * (InIndexType == EIT_16BIT ? sizeof(uint16) : sizeof(uint32));
		const uint32 IndexBufferSize = TMath::Align4(IndexDataSize);
		Data = ti_new TStream(IndexBufferSize);
		Data->Put(InIndexData, IndexDataSize);
	}

	///////////////////////////////////////////////////////////

	const uint32 TInstanceBuffer::InstanceFormat = EINSSEG_TRANSITION | EINSSEG_ROT_SCALE_MAT0 | EINSSEG_ROT_SCALE_MAT1 | EINSSEG_ROT_SCALE_MAT2;
	const uint32 TInstanceBuffer::InstanceStride =
		TInstanceBuffer::SemanticSize[EISI_TRANSITION] +
		TInstanceBuffer::SemanticSize[EISI_ROT_SCALE_MAT0] +
		TInstanceBuffer::SemanticSize[EISI_ROT_SCALE_MAT1] +
		TInstanceBuffer::SemanticSize[EISI_ROT_SCALE_MAT2];

	TInstanceBuffer::TInstanceBuffer()
		: TResource(ERES_INSTANCE)
	{
	}

	TInstanceBuffer::~TInstanceBuffer()
	{
	}

	int32 TInstanceBuffer::GetStrideFromFormat(uint32 Format)
	{
		// Calculate stride
		int32 Stride = 0;
		for (uint32 seg = 1, i = 0; seg <= EINSSEG_TOTAL; seg <<= 1, ++i)
		{
			if ((Format & seg) != 0)
			{
				Stride += TInstanceBuffer::SemanticSize[i];
			}
		}
		return Stride;
	}

	TVector<E_INSTANCE_STREAM_INDEX> TInstanceBuffer::GetSteamsFromFormat(uint32 Format)
	{
		TVector<E_INSTANCE_STREAM_INDEX> Streams;
		for (uint32 seg = 1, i = 0; seg <= EINSSEG_TOTAL; seg <<= 1, ++i)
		{
			if ((Format & seg) != 0)
			{
				Streams.push_back((E_INSTANCE_STREAM_INDEX)i);
			}
		}
		return Streams;
	}

	void TInstanceBuffer::SetInstanceStreamData(
		uint32 InFormat, 
		const void* InInstanceData, int32 InInstanceCount
	)
	{
		Desc.InsFormat = InFormat;
		Desc.InstanceCount = InInstanceCount;
		Desc.Stride = GetStrideFromFormat(InFormat);

		const int32 BufferSize = Desc.InstanceCount * Desc.Stride;
		TI_ASSERT(Data == nullptr);
		Data = ti_new TStream(BufferSize);
		Data->Put(InInstanceData, BufferSize);
	}

	void TInstanceBuffer::InitRenderThreadResource()
	{
		TI_ASSERT(InstanceBufferResource == nullptr);
		InstanceBufferResource = ti_new FInstanceBuffer(Desc);
		InstanceBufferResource->SetResourceName(GetResourceName());

		FInstanceBufferPtr InstanceBuffer = InstanceBufferResource;
		TStreamPtr InData = Data;
		ENQUEUE_RENDER_COMMAND(TInstanceBufferUpdateFInstanceBuffer)(
			[InstanceBuffer, InData]()
			{
				InstanceBuffer->CreateGPUBuffer(InData);
			});

		TI_TODO("release CPU memory after create GPU resource");
		// Do not release yet, still need instance data to create BLAS
		// Data = nullptr;
	}

	void TInstanceBuffer::DestroyRenderThreadResource()
	{
		TI_ASSERT(InstanceBufferResource != nullptr);

		FInstanceBufferPtr InstanceBuffer = InstanceBufferResource;
		ENQUEUE_RENDER_COMMAND(TInstanceBufferDestroyFInstanceBuffer)(
			[InstanceBuffer]()
			{
				//InstanceBuffer = nullptr;
			});
		InstanceBuffer = nullptr;
		InstanceBufferResource = nullptr;
	}
}