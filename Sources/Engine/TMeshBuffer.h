/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	struct TVertexBufferDesc
	{
		EPrimitiveType PrimitiveType;
		FBox BBox;
		uint32 VertexCount;

		uint32 VsFormat;
		uint32 Stride;

		TVertexBufferDesc()
			: PrimitiveType(EPrimitiveType::TriangleList)
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

	inline uint8 FloatToUNorm(float n)
	{
		if (n < -1.f)
			n = -1.f;
		if (n > 1.f)
			n = 1.f;
		n = n * 0.5f + 0.5f;
		float n0 = n * 255.f + 0.5f;
		return (uint8)n0;
	}

	inline uint8 FloatToColor(float n)
	{
		if (n < 0.f)
			n = 0.f;
		if (n > 1.f)
			n = 1.f;
		float n0 = n * 255.f + 0.5f;
		return (uint8)n0;
	}

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

		inline static FByte4 EncodeNormalToByte4(const FFloat3& Normal)
		{
			FByte4 NData;
			NData.X = FloatToUNorm(Normal.X);
			NData.Y = FloatToUNorm(Normal.Y);
			NData.Z = FloatToUNorm(Normal.Z);
			return NData;
		}

		inline static FByte4 EncodeColorToByte4(const FFloat4& Color)
		{
			FByte4 CData;
			CData.X = FloatToUNorm(Color.X);
			CData.Y = FloatToUNorm(Color.Y);
			CData.Z = FloatToUNorm(Color.Z);
			CData.W = FloatToUNorm(Color.W);
			return CData;
		}

		inline static FHalf2 EncodeUVToHalf2(const FFloat2& UV)
		{
			FHalf2 UVData;
			UVData.X = UV.X;
			UVData.Y = UV.Y;
			return UVData;
		}

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

		const TStreamPtr GetVertexBufferData() const
		{
			return Data;
		}
	protected:

	protected:
		TVertexBufferDesc Desc;
		TStreamPtr Data;
	};

	/////////////////////////////////////////////////////////////
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

		const TStreamPtr GetIndexBufferData() const
		{
			return Data;
		}
	protected:

	protected:
		TIndexBufferDesc Desc;
		TStreamPtr Data;
	};

	/////////////////////////////////////////////////////////////

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

		const TStreamPtr GetInstanceData() const
		{
			return Data;
		}
	protected:

	protected:
		TInstanceBufferDesc Desc;
		TStreamPtr Data;
	};
}
