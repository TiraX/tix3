/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	// FMeshBuffer, hold vertex buffer and index buffer render resource
	class FMeshBuffer : public FRenderResource
	{
	public:
		FMeshBuffer();
		FMeshBuffer(
			E_PRIMITIVE_TYPE InPrimType,
			uint32 InVSFormat,
			uint32 InVertexCount,
			E_INDEX_TYPE InIndexType,
			uint32 InIndexCount,
			const FBox& InBBox
		);
		virtual ~FMeshBuffer();

	public:
		void TI_API SetFromTMeshBuffer(TMeshBufferPtr InMeshBuffer);

		uint32 GetVerticesCount() const
		{
			return VsDataCount;
		}

		uint32 GetIndicesCount() const
		{
			return PsDataCount;
		}

		E_PRIMITIVE_TYPE GetPrimitiveType() const
		{
			return PrimitiveType;
		}

		E_INDEX_TYPE GetIndexType() const
		{
			return IndexType;
		}

		void SetPrimitiveType(E_PRIMITIVE_TYPE type)
		{
			PrimitiveType = type;
		}

		uint32 GetVSFormat() const
		{
			return VsFormat;
		}

		uint32 GetStride() const
		{
			return Stride;
		}

		uint32 GetUsage() const
		{
			return Usage;
		}

		const FBox& GetBBox() const
		{
			return BBox;
		}
	protected:

	protected:
		E_PRIMITIVE_TYPE PrimitiveType;

		uint32 VsDataCount;

		E_INDEX_TYPE IndexType;
		uint32 PsDataCount;

		uint32 VsFormat;
		uint32 Stride;

		FBox BBox;
	};

	///////////////////////////////////////////////////////////

	// FInstanceBuffer, hold instance buffer
	class FInstanceBuffer : public FRenderResource
	{
	public:
		FInstanceBuffer();
		FInstanceBuffer(uint32 TotalInstancesCount, uint32 InstanceStride);
		virtual ~FInstanceBuffer();

		void TI_API SetFromTInstanceBuffer(TInstanceBufferPtr InstanceData);

		uint32 GetInstancesCount() const
		{
			return InstanceCount;
		}

		uint32 GetStride() const
		{
			return Stride;
		}

	private:
		uint32 InstanceCount;
		uint32 Stride;
	};
}
