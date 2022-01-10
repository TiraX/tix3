/*
	TiX Engine v2.0 Copyright (C) 2018~2021
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	BEGIN_UNIFORM_BUFFER_STRUCT(FPrimitiveUniformBuffer)
		DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FMatrix, LocalToWorld)
		DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FFloat4, VTUVTransform)
		DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FFloat4, VTDebugInfo)
	END_UNIFORM_BUFFER_STRUCT(FPrimitiveUniformBuffer)

	class FPrimitive : public IReferenceCounted
	{
	public:
		enum {
			PrimitiveUniformBufferDirty = 1 << 0,
		};

		FPrimitive();
		~FPrimitive();

		void SetInstancedStaticMesh(
			TStaticMeshPtr InStaticMesh,
			FInstanceBufferPtr InInstanceBuffer,
			uint32 InInstanceCount,
			uint32 InInstanceOffset
		);
		void SetSkeletalMesh(
			FMeshBufferPtr InMeshBuffer,
			uint32 InIndexStart,
			uint32 InTriangles,
			TMaterialInstancePtr InMInstance
		);
		void SetSkeletonResource(
			FUniformBufferPtr InSkeletonResource
		);

		struct FSection
		{
			FSection()
				: IndexStart(0)
				, Triangles(0)
				, DrawList(LIST_INVALID)
			{}
			int32 IndexStart;
			int32 Triangles;
			FPipelinePtr Pipeline;
			FArgumentBufferPtr Argument;
			E_DRAWLIST_TYPE DrawList;
		};
		int32 GetNumSections() const
		{
			return (int32)Sections.size();
		}

		const FSection& GetSection(int32 Index) const
		{
			return Sections[Index];
		}

		FMeshBufferPtr GetMeshBuffer()
		{
			return MeshBuffer;
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
		FUniformBufferPtr GetSkeletonUniform()
		{
			return SkeletonResourceRef;
		}
		bool IsPrimitiveBufferDirty() const
		{
			return (PrimitiveFlag & PrimitiveUniformBufferDirty) != 0;
		}
		void SetLocalToWorld(const matrix4 InLocalToWorld);
		void SetUVTransform(float UOffset, float VOffset, float UScale, float VScale);
		void SetVTDebugInfo(float A, float B, float C, float D);

		void UpdatePrimitiveBuffer_RenderThread();
	private:
		uint32 PrimitiveFlag;

		FMeshBufferPtr MeshBuffer;
		FInstanceBufferPtr InstanceBuffer;
		uint32 InstanceCount;
		uint32 InstanceOffset;

		FUniformBufferPtr SkeletonResourceRef;

		TVector<FSection> Sections;

		// Every primitive has a unique Primitive uniform buffer, because VT UVTransform
		// Duplicated, always use instance buffer for GPU driven
		FPrimitiveUniformBufferPtr PrimitiveUniformBuffer;
	};
} // end namespace tix

