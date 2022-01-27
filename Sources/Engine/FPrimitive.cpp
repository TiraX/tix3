/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "FPrimitive.h"

namespace tix
{
	FPrimitive::FPrimitive()
		: PrimitiveFlag(0)
		, InstanceCount(0)
		, InstanceOffset(0)
	{
		PrimitiveUniformBuffer = ti_new FPrimitiveUniformBuffer;
	}

	FPrimitive::~FPrimitive()
	{
		TI_ASSERT(IsRenderThread());
		PrimitiveUniformBuffer = nullptr;
	}

	void FPrimitive::InitFromRenderData(FVertexBufferPtr VB, FIndexBufferPtr IB, FPipelinePtr Pipeline, FArgumentBufferPtr MIParam)
	{
		TI_ASSERT(IsRenderThread());
		// Add mesh buffer
		VertexBuffer = VB;
		IndexBuffer = IB;
		TI_ASSERT(VertexBuffer != nullptr && IndexBuffer != nullptr);

		// Add a default section.
		Sections.resize(1);
		Sections[0].IndexStart = 0;
		Sections[0].Triangles = IB->GetDesc().IndexCount / 3;
		Sections[0].Pipeline = Pipeline;
		Sections[0].Argument = MIParam;
	}

	void FPrimitive::InitFromInstancedStaticMesh(
		TStaticMeshPtr InStaticMesh, 
		FInstanceBufferPtr InInstanceBuffer,
		uint32 InInstanceCount,
		uint32 InInstanceOffset)
	{
		TI_ASSERT(IsRenderThread());
		// Add mesh buffer
		VertexBuffer = InStaticMesh->GetVertexBuffer()->VertexBufferResource;
		IndexBuffer = InStaticMesh->GetIndexBuffer()->IndexBufferResource;
		TI_ASSERT(VertexBuffer != nullptr && IndexBuffer != nullptr);

		// Add instance buffer
		InstanceBuffer = InInstanceBuffer;
		InstanceCount = InInstanceCount;
		InstanceOffset = InInstanceOffset;

		// Add sections here.
		uint32 NumSections = InStaticMesh->GetMeshSectionCount();
		Sections.reserve(NumSections);
		for (uint32 S = 0; S < NumSections; S++)
		{
			const TMeshSection& Section = InStaticMesh->GetMeshSection(S);

			FSection PrimitiveSection;
			PrimitiveSection.IndexStart = Section.IndexStart;
			PrimitiveSection.Triangles = Section.Triangles;

			TMaterialPtr Material = Section.DefaultMaterial->LinkedMaterial;
			TI_ASSERT(Material->PipelineResource != nullptr);
			PrimitiveSection.Pipeline = Material->PipelineResource;
			PrimitiveSection.Argument = Section.DefaultMaterial->ArgumentBuffer;
			
			Sections.push_back(PrimitiveSection);
		}
	}

	void FPrimitive::InitFromSkeletalMesh(TStaticMeshPtr InStaticMesh)
	{
		TI_ASSERT(IsRenderThread());
		// Add mesh buffer
		VertexBuffer = InStaticMesh->GetVertexBuffer()->VertexBufferResource;
		IndexBuffer = InStaticMesh->GetIndexBuffer()->IndexBufferResource;
		TI_ASSERT(VertexBuffer != nullptr && IndexBuffer != nullptr);

		// Add sections here.
		uint32 NumSections = InStaticMesh->GetMeshSectionCount();
		Sections.reserve(NumSections);
		for (uint32 S = 0; S < NumSections; S++)
		{
			const TMeshSection& Section = InStaticMesh->GetMeshSection(S);

			FSection PrimitiveSection;
			PrimitiveSection.IndexStart = Section.IndexStart;
			PrimitiveSection.Triangles = Section.Triangles;

			TMaterialPtr Material = Section.DefaultMaterial->LinkedMaterial;
			TI_ASSERT(Material->PipelineResource != nullptr);
			PrimitiveSection.Pipeline = Material->PipelineResource;
			PrimitiveSection.Argument = Section.DefaultMaterial->ArgumentBuffer;

			Sections.push_back(PrimitiveSection);
		}
	}

	void FPrimitive::SetSkeletonResource(int32 SectionIndex, FUniformBufferPtr InSkeletonResource)
	{
		Sections[SectionIndex].SkeletonResourceRef = InSkeletonResource;
	}

	void FPrimitive::SetLocalToWorld(const FMat4 InLocalToWorld)
	{
		TI_ASSERT(IsRenderThread());
		PrimitiveUniformBuffer->UniformBufferData[0].LocalToWorld = InLocalToWorld.GetTransposed();
		PrimitiveFlag |= PrimitiveUniformBufferDirty;
	}

	void FPrimitive::SetUVTransform(float UOffset, float VOffset, float UScale, float VScale)
	{
		TI_ASSERT(IsRenderThread());
		PrimitiveUniformBuffer->UniformBufferData[0].VTUVTransform = FFloat4(UOffset, VOffset, UScale, VScale);
		PrimitiveFlag |= PrimitiveUniformBufferDirty;
	}

	void FPrimitive::SetVTDebugInfo(float A, float B, float C, float D)
	{
		TI_ASSERT(IsRenderThread());
		PrimitiveUniformBuffer->UniformBufferData[0].VTDebugInfo = FFloat4(A, B, C, D);
		PrimitiveFlag |= PrimitiveUniformBufferDirty;
	}

	void FPrimitive::UpdatePrimitiveBuffer_RenderThread()
	{
		TI_ASSERT(IsRenderThread());
		PrimitiveUniformBuffer->InitUniformBuffer((uint32)EGPUResourceFlag::Intermediate);
		PrimitiveFlag &= ~PrimitiveUniformBufferDirty;
	}
}
