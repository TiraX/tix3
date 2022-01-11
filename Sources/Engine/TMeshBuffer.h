/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	// TMeshSection, hold mesh section info
	struct TMeshSection
	{
		TMeshSection()
			: IndexStart(0)
			, Triangles(0)
		{}

		TMaterialInstancePtr DefaultMaterial;
		uint32 IndexStart;
		uint32 Triangles;
		TVector<uint32> BoneMap;
	};

	struct TMeshBufferDesc
	{
		E_PRIMITIVE_TYPE PrimitiveType;
		FBox BBox;
		uint32 VertexCount;

		E_INDEX_TYPE IndexType;
		uint32 IndexCount;

		uint32 VsFormat;
		uint32 Stride;

		TMeshBufferDesc()
			: PrimitiveType(EPT_TRIANGLELIST)
			, VertexCount(0)
			, IndexType(EIT_16BIT)
			, IndexCount(0)
			, VsFormat(0)
			, Stride(0)
		{}
	};

	// TMeshBuffer, hold mesh vertex and index data memory in game thread
	class TI_API TMeshBuffer : public TResource
	{
	public:
		TMeshBuffer();
		~TMeshBuffer();

		static const int32 SemanticSize[ESSI_TOTAL];
		static const int8* SemanticName[ESSI_TOTAL];
		static const int32 SemanticIndex[ESSI_TOTAL];
	public:
		FMeshBufferPtr MeshBufferResource;

		static uint32 GetStrideFromFormat(uint32 Format);
		static TVector<E_MESH_STREAM_INDEX> GetSteamsFromFormat(uint32 Format);

		virtual void InitRenderThreadResource() override;
		virtual void DestroyRenderThreadResource() override;

		void SetVertexStreamData(
			uint32 InFormat,
			const void* InVertexData, uint32 InVertexCount,
			E_INDEX_TYPE InIndexType,
			const void* InIndexData, uint32 InIndexCount);

		const TMeshBufferDesc& GetDesc() const
		{
			return Desc;
		}

		void SetPrimitiveType(E_PRIMITIVE_TYPE type)
		{
			Desc.PrimitiveType = type;
		}

		void SetBBox(const FBox& bbox)
		{
			Desc.BBox = bbox;
		}
	protected:

	protected:
		TMeshBufferDesc Desc;
		TStreamPtr Data;
	};

	///////////////////////////////////////////////////////////

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
		FInstanceBufferPtr InstanceResource;

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
			return InstanceData;
		}
	protected:

	protected:
		TInstanceBufferDesc Desc;
		uint8* InstanceData;
	};
}
