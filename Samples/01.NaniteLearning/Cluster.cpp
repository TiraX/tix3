/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "Cluster.h"

void CorrectAttributes(float* Attributes)
{
	FFloat3& Normal = *reinterpret_cast<FFloat3*>(Attributes);
	Normal.Normalize();
}

void CorrectAttributesColor(float* Attributes)
{
	CorrectAttributes(Attributes);

	SColorf& Color = *reinterpret_cast<SColorf*>(Attributes + 3);
	Color.R = TMath::Clamp(Color.R, 0.f, 1.f);
	Color.G = TMath::Clamp(Color.G, 0.f, 1.f);
	Color.B = TMath::Clamp(Color.B, 0.f, 1.f);
	Color.A = TMath::Clamp(Color.A, 0.f, 1.f);
}


void FCluster::GenerateGUID(int32 _Id)
{
	GUID = (uint64(_Id + 269264));
	_ID = _Id;
}

void FCluster::BuildCluster(
	const TVector<RawVertex>& InVerts,
	const TVector<int32>& InClusterIndexes,
	const TVector<int32>& InMaterialIndexes,
	int32 InNumTexCoords)
{
	NumTris = (uint32)InClusterIndexes.size() / 3;

	bHasColors = false;
	NumTexCoords = InNumTexCoords;

	Verts.reserve(NumTris * GetVertSize());
	Indexes.reserve(3 * NumTris);
	MaterialIndexes.reserve(NumTris);
	ExternalEdges.reserve(3 * NumTris);
	NumExternalEdges = 0;

	TI_ASSERT(InMaterialIndexes.size() * 3 == InClusterIndexes.size());

	THMap< uint32, uint32 > OldToNewIndex;
	OldToNewIndex.reserve(NumTris);

	for (int32 i = 0; i < (int32)InClusterIndexes.size(); i++)
	{
		uint32 VertIndex = i;
		uint32 OldIndex = InClusterIndexes[VertIndex];
		THMap<uint32, uint32>::const_iterator ItNewIndex = OldToNewIndex.find(OldIndex);
		uint32 NewIndex = ItNewIndex != OldToNewIndex.end() ? ItNewIndex->second : ~0u;

		if (NewIndex == ~0u)
		{
			//Verts.AddUninitialized(GetVertSize());
			Verts.resize(Verts.size() + GetVertSize());
			NewIndex = NumVerts++;
			OldToNewIndex[OldIndex] = NewIndex;

			const RawVertex& InVert = InVerts[OldIndex];

			GetPosition(NewIndex) = InVert.Pos;
			//GetNormal(NewIndex) = InVert.TangentZ.ContainsNaN() ? FVector3f::UpVector : InVert.TangentZ;
			GetNormal(NewIndex) = InVert.Nor;

			if (bHasColors)
			{
				RuntimeFail();
				//GetColor(NewIndex) = InVert.Color.ReinterpretAsLinear();
			}

			FFloat2* UVs = GetUVs(NewIndex);
			for (uint32 UVIndex = 0; UVIndex < NumTexCoords; UVIndex++)
			{
				//UVs[UVIndex] = InVert.UVs[UVIndex].ContainsNaN() ? FVector2f::ZeroVector : InVert.UVs[UVIndex];
				UVs[UVIndex] = InVert.UV;
			}

			float* Attributes = GetAttributes(NewIndex);

			// Make sure this vertex is valid from the start
			if (bHasColors)
				CorrectAttributesColor(Attributes);
			else
				CorrectAttributes(Attributes);
		}

		Indexes.push_back(NewIndex);

		NumExternalEdges += 0;
	}

	for (int32 i = 0; i < (int32)InClusterIndexes.size(); i += 3)
	{
		MaterialIndexes.push_back(InMaterialIndexes[i / 3]);
	}

	Bound();
}

void FCluster::Bound()
{
	Bounds = FBox();
	SurfaceArea = 0.0f;

	TVector< FFloat3 > Positions;
	Positions.resize(NumVerts);

	for (uint32 i = 0; i < NumVerts; i++)
	{
		Positions[i] = GetPosition(i);
		Bounds.AddInternalPoint(Positions[i]);
	}
	SphereBounds = FSpheref(Positions.data(), NumVerts);
	LODBounds = SphereBounds;

	float MaxEdgeLength2 = 0.0f;
	for (int i = 0; i < (int32)Indexes.size(); i += 3)
	{
		FFloat3 v[3];
		v[0] = GetPosition(Indexes[i + 0]);
		v[1] = GetPosition(Indexes[i + 1]);
		v[2] = GetPosition(Indexes[i + 2]);

		FFloat3 Edge01 = v[1] - v[0];
		FFloat3 Edge12 = v[2] - v[1];
		FFloat3 Edge20 = v[0] - v[2];

		MaxEdgeLength2 = TMath::Max(MaxEdgeLength2, Edge01.GetLengthSQ());
		MaxEdgeLength2 = TMath::Max(MaxEdgeLength2, Edge12.GetLengthSQ());
		MaxEdgeLength2 = TMath::Max(MaxEdgeLength2, Edge20.GetLengthSQ());

		float TriArea = 0.5f * Edge01.Cross(Edge20).GetLength();
		SurfaceArea += TriArea;
	}
	EdgeLength = TMath::Sqrt(MaxEdgeLength2);
}