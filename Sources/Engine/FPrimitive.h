/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	BEGIN_UNIFORM_BUFFER_STRUCT(FPrimitiveUniformBuffer)
		DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FMat4, LocalToWorld)
		DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FFloat4, VTUVTransform)
		DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FFloat4, VTDebugInfo)
	END_UNIFORM_BUFFER_STRUCT(FPrimitiveUniformBuffer)

	class TI_API FPrimitive : public IReferenceCounted
	{
	public:
		enum {
			PrimitiveUniformBufferDirty = 1 << 0,
		};

		FPrimitive();
		~FPrimitive();

		void InitFromRenderData(
			FVertexBufferPtr VB,
			FIndexBufferPtr IB,
			FInstanceBufferPtr InsB,
			FPipelinePtr Pipeline,
			FArgumentBufferPtr MIParam
		);
		void InitFromInstancedStaticMesh(
			TStaticMeshPtr InStaticMesh,
			FInstanceBufferPtr InInstanceBuffer,
			uint32 InInstanceCount,
			uint32 InInstanceOffset
		);
		void InitFromSkeletalMesh(
			TStaticMeshPtr InStaticMesh
		);
		void SetSkeletonResource(
			int32 SectionIndex,
			FUniformBufferPtr InSkeletonResource
		);

		struct FSection
		{
			FSection()
				: IndexStart(0)
				, Triangles(0)
			{}
			int32 IndexStart;
			int32 Triangles;
			FPipelinePtr Pipeline;
			FArgumentBufferPtr Argument;
			FUniformBufferPtr SkeletonResourceRef;
		};
		int32 GetNumSections() const
		{
			return (int32)Sections.size();
		}

		const FSection& GetSection(int32 Index) const
		{
			return Sections[Index];
		}

		FVertexBufferPtr GetVertexBuffer()
		{
			return VertexBuffer;
		}
		FIndexBufferPtr GetIndexBuffer()
		{
			return IndexBuffer;
		}
		FInstanceBufferPtr GetInstanceBuffer()
		{
			return InstanceBuffer;
		}
		uint32 GetInstanceCount() const
		{
			return InstanceCount;
		}
		uint32 GetInstanceOffset() const
		{
			return InstanceOffset;
		}
		FPrimitiveUniformBufferPtr GetPrimitiveUniform()
		{
			return PrimitiveUniformBuffer;
		}
		bool IsPrimitiveBufferDirty() const
		{
			return (PrimitiveFlag & PrimitiveUniformBufferDirty) != 0;
		}
		void SetLocalToWorld(const FMat4 InLocalToWorld);
		void SetUVTransform(float UOffset, float VOffset, float UScale, float VScale);
		void SetVTDebugInfo(float A, float B, float C, float D);

		void UpdatePrimitiveBuffer_RenderThread();
	private:
		uint32 PrimitiveFlag;

		FVertexBufferPtr VertexBuffer;
		FIndexBufferPtr IndexBuffer;
		FInstanceBufferPtr InstanceBuffer;
		uint32 InstanceCount;
		uint32 InstanceOffset;

		TVector<FSection> Sections;

		// Every primitive has a unique Primitive uniform buffer, because VT UVTransform
		// Duplicated, always use instance buffer for GPU driven
		FPrimitiveUniformBufferPtr PrimitiveUniformBuffer;
	};
} // end namespace tix

