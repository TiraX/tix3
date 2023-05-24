/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once
#include "TMeshBuffer.h"

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

	// TStaticMesh, hold static mesh components like mesh buffer, mesh sections, collisions, occluders
	class TI_API TStaticMesh : public TResource
	{
	public:
		TStaticMesh(TVertexBufferPtr InVB, TIndexBufferPtr InIB, uint32 InFlags);
		virtual ~TStaticMesh();

		virtual void InitRenderThreadResource();
		virtual void DestroyRenderThreadResource();

		TVertexBufferPtr GetVertexBuffer()
		{
			return VertexBuffer;
		}

		TIndexBufferPtr GetIndexBuffer()
		{
			return IndexBuffer;
		}

		TStaticMeshPtr GetOccludeMesh()
		{
			return OccludeMesh;
		}

		void AddMeshSection(const TMeshSection& InSection)
		{
			TI_ASSERT(InSection.Triangles <= IndexBuffer->GetDesc().IndexCount / 3);
			MeshSections.push_back(InSection);
		}

		void SetCollision(TCollisionSetPtr InCollision)
		{
			CollisionSet = InCollision;
		}

		void SetOccludeMesh(TStaticMeshPtr InOcclude)
		{
			OccludeMesh = InOcclude;
		}
		
		uint32 GetMeshSectionCount() const
		{
			return (uint32)MeshSections.size();
		}

		const TMeshSection& GetMeshSection(int32 SectionIndex) const
		{
			return MeshSections[SectionIndex];
		}

		void CreateOccludeMeshFromCollision();

	private:
		uint32 Flags;
		TVertexBufferPtr VertexBuffer;
		TIndexBufferPtr IndexBuffer;
		TVector<TMeshSection> MeshSections;
		TCollisionSetPtr CollisionSet;
		TStaticMeshPtr OccludeMesh;
	};
}
