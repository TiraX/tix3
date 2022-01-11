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
		MeshBuffer = InStaticMesh->GetMeshBuffer()->MeshBufferResource;
		TI_ASSERT(MeshBuffer != nullptr);

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

	void FPrimitive::SetSkeletalMesh(
		FMeshBufferPtr InMeshBuffer,
		uint32 InIndexStart,
		uint32 InTriangles,
		TMaterialInstancePtr InMInstance)
	{
		TI_ASSERT(0);
		//// Add mesh buffer
		//MeshBuffer = InMeshBuffer;

		//// Add pipeline
		//TMaterialPtr Material = InMInstance->LinkedMaterial;
		//TI_ASSERT(Material->PipelineResource != nullptr);
		//Pipeline = Material->PipelineResource;

		//// Instance material argument buffer
		//Argument = InMInstance->ArgumentBuffer;

		//// Draw List
		//if (Material->GetBlendMode() == BLEND_MODE_OPAQUE)
		//{
		//	DrawList = LIST_OPAQUE;
		//}
		//else if (Material->GetBlendMode() == BLEND_MODE_MASK)
		//{
		//	DrawList = LIST_MASK;
		//}
		//else
		//{
		//	DrawList = LIST_TRANSLUCENT;
		//}
	}

	void FPrimitive::SetSkeletonResource(FUniformBufferPtr InSkeletonResource)
	{
		SkeletonResourceRef = InSkeletonResource;
	}

	void FPrimitive::SetLocalToWorld(const matrix4 InLocalToWorld)
	{
		TI_ASSERT(IsRenderThread());
		PrimitiveUniformBuffer->UniformBufferData[0].LocalToWorld = InLocalToWorld;
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
		PrimitiveUniformBuffer->InitUniformBuffer(UB_FLAG_INTERMEDIATE);
		PrimitiveFlag &= ~PrimitiveUniformBufferDirty;
	}
}
