/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "TMeshBuffer.h"
#include "TStaticMesh.h"

namespace tix
{
	TStaticMesh::TStaticMesh(TVertexBufferPtr InVB, TIndexBufferPtr InIB, uint32 InFlags)
		: TResource(ERES_STATIC_MESH)
		, Flags(InFlags)
		, VertexBuffer(InVB)
		, IndexBuffer(InIB)
	{
		if ((Flags & CpuAccess) != 0)
		{
			VertexBuffer->SetFlag(CpuAccess);
			IndexBuffer->SetFlag(CpuAccess);
		}
	}

	TStaticMesh::~TStaticMesh()
	{
	}

	void TStaticMesh::InitRenderThreadResource()
	{
		VertexBuffer->InitRenderThreadResource();
		IndexBuffer->InitRenderThreadResource();
		if (OccludeMesh != nullptr)
		{
			OccludeMesh->InitRenderThreadResource();
		}
	}

	void TStaticMesh::DestroyRenderThreadResource()
	{
		VertexBuffer->DestroyRenderThreadResource();
		IndexBuffer->DestroyRenderThreadResource();
		if (OccludeMesh != nullptr)
		{
			OccludeMesh->DestroyRenderThreadResource();
		}
	}

	void TStaticMesh::CreateOccludeMeshFromCollision()
	{
		TI_ASSERT(OccludeMesh == nullptr);
		OccludeMesh = CollisionSet->ConvertToStaticMesh();
		if (OccludeMesh != nullptr)
		{
			OccludeMesh->SetResourceName(VertexBuffer->GetResourceName() + "-Occluder");
		}
	}
}