/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "TMeshBuffer.h"
#include "TMeshBufferSemantic.h"

namespace tix
{
	TMeshBuffer::TMeshBuffer()
		: TResource(ERES_MESH_BUFFER)
	{
	}

	TMeshBuffer::~TMeshBuffer()
	{
	}

	uint32 TMeshBuffer::GetStrideFromFormat(uint32 Format)
	{
		// Calculate stride
		uint32 Stride = 0;
		for (uint32 seg = 1, i = 0; seg <= EVSSEG_TOTAL; seg <<= 1, ++i)
		{
			if ((Format & seg) != 0)
			{
				Stride += TMeshBuffer::SemanticSize[i];
			}
		}
		return Stride;
	}

	TVector<E_MESH_STREAM_INDEX> TMeshBuffer::GetSteamsFromFormat(uint32 Format)
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

	void TMeshBuffer::InitRenderThreadResource()
	{
		TI_ASSERT(MeshBufferResource == nullptr);
		MeshBufferResource = ti_new FMeshBuffer(Desc);

		FMeshBufferPtr MeshBuffer = MeshBufferResource;
		TStreamPtr InData = Data;
		ENQUEUE_RENDER_COMMAND(TMeshBufferUpdateFMeshBuffer)(
			[MeshBuffer, InData]()
			{
				MeshBuffer->CreateGPUResource(InData);
			});

		// release CPU memory after create render resource
		Data = nullptr;	
	}

	void TMeshBuffer::DestroyRenderThreadResource()
	{
		TI_ASSERT(MeshBufferResource != nullptr);

		FMeshBufferPtr MeshBuffer = MeshBufferResource;
		ENQUEUE_RENDER_COMMAND(TMeshBufferDestroyFMeshBuffer)(
			[MeshBuffer]()
			{
				//MeshBuffer = nullptr;
			});
		MeshBuffer = nullptr;
		MeshBufferResource = nullptr;
	}

	void TMeshBuffer::SetVertexStreamData(
		uint32 InFormat,
		const void* InVertexData, uint32 InVertexCount,
		E_INDEX_TYPE InIndexType,
		const void* InIndexData, uint32 InIndexCount)
	{
		Desc.VsFormat = InFormat;
		Desc.VertexCount = InVertexCount;

		Desc.IndexType = InIndexType;
		Desc.IndexCount = InIndexCount;

		Desc.Stride = GetStrideFromFormat(InFormat);

		// Copy data
		TI_ASSERT(Data == nullptr);
		const int32 VertexDataSize = InVertexCount * Desc.Stride;
		const int32 IndexDataSize = InIndexCount * (InIndexType == EIT_16BIT ? sizeof(uint16) : sizeof(uint32));
		const uint32 VertexBufferSize = TMath::Align4(VertexDataSize);
		TI_ASSERT(VertexBufferSize == VertexDataSize); // vertex always 4 bytes aligned
		const uint32 IndexBufferSize = TMath::Align4(IndexDataSize);
		Data = ti_new TStream(VertexBufferSize + IndexBufferSize);
		Data->Put(InVertexData, VertexDataSize);
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
		, InstanceData(nullptr)
	{
	}

	TInstanceBuffer::~TInstanceBuffer()
	{
		SAFE_DELETE(InstanceData);
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
		TI_ASSERT(InstanceData == nullptr);
		InstanceData = ti_new uint8[BufferSize];
		memcpy(InstanceData, InInstanceData, BufferSize);
	}

	void TInstanceBuffer::InitRenderThreadResource()
	{
		TI_ASSERT(InstanceResource == nullptr);
		InstanceResource = FRHI::Get()->CreateInstanceBuffer();
		// Set Instance Resource Usage to USAGE_COPY_SOURCE, 
		// as GPU Driven pipeline need to copy these instance buffer into a merged instance buffer
		TI_TODO("Add gpu driven CVAR to check here.");
		//InstanceResource->SetUsage(FRenderResource::USAGE_COPY_SOURCE);
		TI_ASSERT(Desc.InstanceCount != 0);

		FInstanceBufferPtr InstanceBuffer = InstanceResource;
		TInstanceBufferPtr InInstanceData = this;
		ENQUEUE_RENDER_COMMAND(TInstanceBufferUpdateFInstanceBuffer)(
			[InstanceBuffer, InInstanceData]()
			{
				InstanceBuffer->SetFromTInstanceBuffer(InInstanceData);
				FRHI::Get()->UpdateHardwareResourceIB(InstanceBuffer, InInstanceData);
			});
	}

	void TInstanceBuffer::DestroyRenderThreadResource()
	{
		TI_ASSERT(InstanceResource != nullptr);

		FInstanceBufferPtr InstanceBuffer = InstanceResource;
		ENQUEUE_RENDER_COMMAND(TInstanceBufferDestroyFInstanceBuffer)(
			[InstanceBuffer]()
			{
				//InstanceBuffer = nullptr;
			});
		InstanceResource = nullptr;
	}
}