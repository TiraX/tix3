/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "TCollisionSet.h"

namespace tix
{
	TCollisionSet::TCollisionSet()
		: TResource(ERES_COLLISION)
	{
	}

	TCollisionSet::~TCollisionSet()
	{
	}

	void TCollisionSet::InitRenderThreadResource()
	{
	}

	void TCollisionSet::DestroyRenderThreadResource()
	{
	}

	TStaticMeshPtr TCollisionSet::ConvertToStaticMesh() const
	{
		TVector<FFloat3> Positions;
		TVector<uint32> Indices;
		Positions.reserve(64 * 1024);
		Indices.reserve(128 * 1024);

		for (const auto& Sphere : Spheres)
		{
			TShape::CreateICOSphere(3, Sphere.Center, Sphere.Radius, Positions, Indices);
		}

		for (const auto& Box : Boxes)
		{
			TShape::CreateBox(Box.Center, Box.Edge, Box.Rotation, Positions, Indices);
		}

		for (const auto& Capsule : Capsules)
		{
			TShape::CreateCapsule(6, 10, Capsule.Center, Capsule.Radius, Capsule.Length, Capsule.Rotation, Positions, Indices);
		}

		for (const auto& Convex : Convexes)
		{
			uint32 IndexOffset = (uint32)Positions.size();
			Positions.insert(Positions.end(), Convex.VertexData.begin(), Convex.VertexData.end());
			for (auto I : Convex.IndexData)
			{
				uint32 Index = I + IndexOffset;
				Indices.push_back(Index);
			}
		}
		TI_ASSERT(Positions.size() < 65535);
		if (Positions.size() > 0)
		{
			TVertexBufferPtr VB = ti_new TVertexBuffer;
			TIndexBufferPtr IB = ti_new TIndexBuffer;

			FBox Box;
			Box.Reset(Positions[0]);
			for (const auto& P : Positions)
			{
				Box.AddInternalPoint(P);
			}

			VB->SetVertexData(EVSSEG_POSITION, Positions.data(), (uint32)Positions.size(), Box);
			IB->SetIndexData(EIT_32BIT, Indices.data(), (uint32)Indices.size());

			TStaticMeshPtr SM = ti_new TStaticMesh(VB, IB, 0);
			return SM;
		}
		else
		{
			return nullptr;
		}
	}
}