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

	void FPrimitive::SetInstancedStaticMesh(
		TStaticMeshPtr InStaticMesh, 
		FInstanceBufferPtr InInstanceBuffer,
		uint32 InInstanceCount,
		uint32 InInstanceOffset)
	{
		TI_ASSERT(IsGameThread());
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
			
			if (Material->GetBlendMode() == BLEND_MODE_OPAQUE)
			{
				PrimitiveSection.DrawList = LIST_OPAQUE;
			}
			else if (Material->GetBlendMode() == BLEND_MODE_MASK)
			{
				PrimitiveSection.DrawList = LIST_MASK;
			}
			else
			{
				PrimitiveSection.DrawList = LIST_TRANSLUCENT;
			}
			Sections.push_back(PrimitiveSection);
		}
	}

	void FPrimitive::SetSkeletalMesh(TStaticMeshPtr InStaticMesh)
	{
		TI_ASSERT(IsGameThread());
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

			if (Material->GetBlendMode() == BLEND_MODE_OPAQUE)
			{
				PrimitiveSection.DrawList = LIST_OPAQUE;
			}
			else if (Material->GetBlendMode() == BLEND_MODE_MASK)
			{
				PrimitiveSection.DrawList = LIST_MASK;
			}
			else
			{
				PrimitiveSection.DrawList = LIST_TRANSLUCENT;
			}
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
