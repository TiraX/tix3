/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	struct TVertexBufferDesc
	{
		E_PRIMITIVE_TYPE PrimitiveType;
		FBox BBox;
		uint32 VertexCount;

		uint32 VsFormat;
		uint32 Stride;

		TVertexBufferDesc()
			: PrimitiveType(EPT_TRIANGLELIST)
			, VertexCount(0)
			, VsFormat(0)
			, Stride(0)
		{}
	};
	struct TIndexBufferDesc
	{
		E_INDEX_TYPE IndexType;
		uint32 IndexCount;

		TIndexBufferDesc()
			: IndexType(EIT_16BIT)
			, IndexCount(0)
		{}
	};
	struct TInstanceBufferDesc
	{
		uint32 InsFormat;
		int32 InstanceCount;
		uint32 Stride;

		TInstanceBufferDesc()
			: InsFormat(0)
			, InstanceCount(0)
			, Stride(0)
		{}
	};

	// TVertexBuffer, hold vertex data
	class TI_API TVertexBuffer : public TResource
	{
	public:
		TVertexBuffer();
		virtual ~TVertexBuffer();

		static const int32 SemanticSize[ESSI_TOTAL];
		static const int8* SemanticName[ESSI_TOTAL];
		static const int32 SemanticIndex[ESSI_TOTAL];
	public:
		FVertexBufferPtr VertexBufferResource;

		static uint32 GetStrideFromFormat(uint32 Format);
		static TVector<E_MESH_STREAM_INDEX> GetSteamsFromFormat(uint32 Format);

		virtual void InitRenderThreadResource() override;
		virtual void DestroyRenderThreadResource() override;

		void SetVertexData(
			uint32 InFormat,
			const void* InVertexData, uint32 InVertexCount,
			const FBox& InBox);

		const TVertexBufferDesc& GetDesc() const
		{
			return Desc;
		}
	protected:

	protected:
		TVertexBufferDesc Desc;
		TStreamPtr Data;
	};

	///////////////////////////////////////////////////////////
	// TIndexBuffer, hold index data
	class TI_API TIndexBuffer : public TResource
	{
	public:
		TIndexBuffer();
		virtual ~TIndexBuffer();

		FIndexBufferPtr IndexBufferResource;

		virtual void InitRenderThreadResource() override;
		virtual void DestroyRenderThreadResource() override;

		void SetIndexData(
			E_INDEX_TYPE InIndexType,
			const void* InIndexData, uint32 InIndexCount);

		const TIndexBufferDesc& GetDesc() const
		{
			return Desc;
		}
	protected:

	protected:
		TIndexBufferDesc Desc;
		TStreamPtr Data;
	};

	///////////////////////////////////////////////////////////

	// TInstanceBuffer, hold instance data
	class TI_API TInstanceBuffer : public TResource
	{
	public:
		TInstanceBuffer();
		~TInstanceBuffer();

		static const int32 SemanticSize[EISI_TOTAL];
		static const int8* SemanticName[EISI_TOTAL];
		static const int32 SemanticIndex[EISI_TOTAL];

		static const uint32 InstanceFormat;
		static const uint32 InstanceStride;

	public:
		FInstanceBufferPtr InstanceBufferResource;

		static int32 GetStrideFromFormat(uint32 Format);
		static TVector<E_INSTANCE_STREAM_INDEX> GetSteamsFromFormat(uint32 Format);
		void SetInstanceStreamData(
			uint32 InFormat, 
			const void* InInstanceData, int32 InInstanceCount
		);

		virtual void InitRenderThreadResource() override;
		virtual void DestroyRenderThreadResource() override;

		const TInstanceBufferDesc& GetDesc() const
		{
			return Desc;
		}

		const void* GetInstanceData() const
		{
			return Data->GetBuffer();
		}
	protected:

	protected:
		TInstanceBufferDesc Desc;
		TStreamPtr Data;
	};
}
