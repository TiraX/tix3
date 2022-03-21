/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "TShape.h"

namespace tix
{
	void _CreateICOSphere(uint32 Frequency, TVector<FFloat3>& OutPositions, TVector<uint32>& OutIndices)
	{
		TI_ASSERT(Frequency >= 1 && Frequency < 80);

		// Reserve space for containers
		static const uint32 PointsByFrequency[] = { 12, 42, 92, 162, 252, 362, 492, 642, 812, 1002};
		static const uint32 FacesByFrequency[] = { 20, 80, 180, 320, 500, 720, 980, 1280, 1620, 2000};
		uint32 FreqIndex = TMath::Min(Frequency - 1, 9u);
		OutPositions.reserve(PointsByFrequency[FreqIndex]);
		OutIndices.reserve(FacesByFrequency[FreqIndex] * 3);

		const float T = (1.f + 2.236f) / 2.f;
		static const FFloat3 BasePos[] =
		{
			FFloat3(-1.f, T, 0.f).Normalize(),
			FFloat3(1.f, T, 0.f).Normalize(),
			FFloat3(-1.f, -T, 0.f).Normalize(),
			FFloat3(1.f, -T, 0.f).Normalize(),

			FFloat3(0.f, -1.f, T).Normalize(),
			FFloat3(0.f, 1.f, T).Normalize(),
			FFloat3(0.f, -1.f, -T).Normalize(),
			FFloat3(0.f, 1.f, -T).Normalize(),

			FFloat3(T, 0.f, -1.f).Normalize(),
			FFloat3(T, 0.f, 1.f).Normalize(),
			FFloat3(-T, 0.f, -1.f).Normalize(),
			FFloat3(-T, 0.f, 1.f).Normalize()
		};

		static const FInt3 BaseFaces[] =
		{
			FInt3(0, 11, 5),
			FInt3(0, 5, 1),
			FInt3(0, 1, 7),
			FInt3(0, 7, 10),
			FInt3(0, 10, 11),

			FInt3(1, 5, 9),
			FInt3(5, 11, 4),
			FInt3(11, 10, 2),
			FInt3(10, 7, 6),
			FInt3(7, 1, 8),

			FInt3(3, 9, 4),
			FInt3(3, 4, 2),
			FInt3(3, 2, 6),
			FInt3(3, 6, 8),
			FInt3(3, 8, 9),

			FInt3(4, 9, 5),
			FInt3(2, 4, 11),
			FInt3(6, 2, 10),
			FInt3(8, 6, 7),
			FInt3(9, 8, 1)
		};

		const uint32 Faces = sizeof(BaseFaces) / sizeof(FInt3);
		const float FreqInv = 1.f / Frequency;
		const float Tolerance = 1.f / Frequency * 0.4f;
		// Key.X = StartPoint, Key.Y = EndPoint, Key.Z = Step
		THMap<FInt3, uint32> PointsMap;
		TVector<uint32> IndicesMap;
		IndicesMap.reserve(64);

		auto MakeKey = [](const FFloat3& Pos, float Tolerance)
		{
			FInt3 Key;
			Key.X = TMath::Round(Pos.X / Tolerance);
			Key.Y = TMath::Round(Pos.Y / Tolerance);
			Key.Z = TMath::Round(Pos.Z / Tolerance);
			return Key;
		};
		auto AddSharedPoint = [](const FInt3& Key, const FFloat3& Pos, THMap<FInt3, uint32>& PointsMap, TVector<FFloat3>& OutPositions)
		{
			// Return shared point index
			if (PointsMap.find(Key) == PointsMap.end())
			{
				OutPositions.push_back(Pos);
				PointsMap[Key] = (uint32)(OutPositions.size() - 1);
			}
			TI_ASSERT(PointsMap[Key] < 65535);
			return uint32(PointsMap[Key]);
		};

		const uint32 TesselationPoints = (1 + (Frequency + 1)) * (Frequency + 1) / 2;
		for (uint32 f = 0; f < Faces; ++f)
		{
			const FInt3 Face = BaseFaces[f];
			const uint32 PointsOffset = TesselationPoints * f;
			//   p0
			//  /  \
			// p1---p2
			const FFloat3& P0 = BasePos[Face.X];
			const FFloat3& P1 = BasePos[Face.Y];
			const FFloat3& P2 = BasePos[Face.Z];

			FInt3 Key = MakeKey(P0, Tolerance);

			FFloat3 StepLeft = (P1 - P0) * FreqInv;
			FFloat3 StepRight = (P2 - P0) * FreqInv;

			// Generate points
			uint32 PointIndex = AddSharedPoint(Key, P0, PointsMap, OutPositions);
			IndicesMap.push_back(PointIndex);

			for (uint32 StepY = 0; StepY < Frequency; ++StepY)
			{
				FFloat3 PointLeft = (P0 + StepLeft * float(StepY + 1)).Normalize();
				FFloat3 PointRight = (P0 + StepRight * float(StepY + 1)).Normalize();

				float FreqHorizonInv = 1.f / (StepY + 1);
				FFloat3 StepHorizon = (PointRight - PointLeft) * FreqHorizonInv;

				FInt3 KeyLeft = MakeKey(PointLeft, Tolerance);
				FInt3 KeyRight = MakeKey(PointRight, Tolerance);
				PointIndex = AddSharedPoint(KeyLeft, PointLeft, PointsMap, OutPositions);
				IndicesMap.push_back(PointIndex);
				for (uint32 StepX = 0; StepX < StepY; ++StepX)
				{
					FFloat3 P = (PointLeft + StepHorizon * float(StepX + 1)).Normalize();
					FInt3 KeyP = MakeKey(P, Tolerance);
					PointIndex = AddSharedPoint(KeyP, P, PointsMap, OutPositions);
					IndicesMap.push_back(PointIndex);
				}
				PointIndex = AddSharedPoint(KeyRight, PointRight, PointsMap, OutPositions);
				IndicesMap.push_back(PointIndex);
			}

			// Generate indices
			int32 Index = PointsOffset;
			for (uint32 StepY = 0; StepY < Frequency; ++StepY)
			{
				uint32 PointCount = StepY + 1;

				for (uint32 StepX = 0; StepX < PointCount; ++StepX)
				{
					FInt3 NewFace;
					NewFace.X = Index + StepX;	// Current Point
					NewFace.Y = Index + StepX + PointCount + 1;	// Next Row Right
					NewFace.Z = Index + StepX + PointCount;	// Next Row Left
					OutIndices.push_back(IndicesMap[NewFace.X]);
					OutIndices.push_back(IndicesMap[NewFace.Y]);
					OutIndices.push_back(IndicesMap[NewFace.Z]);

					if (StepX != PointCount - 1)
					{
						NewFace.X = Index + StepX;
						NewFace.Y = Index + StepX + 1;
						NewFace.Z = Index + StepX + PointCount + 1;
						OutIndices.push_back(IndicesMap[NewFace.X]);
						OutIndices.push_back(IndicesMap[NewFace.Y]);
						OutIndices.push_back(IndicesMap[NewFace.Z]);
					}
				}
				Index += PointCount;
			}
		}
	}

	void _CreateLongLatitudeSphere(uint32 Longitude, uint32 Latitude, TVector<FFloat3>& OutPositions, TVector<uint32>& OutIndices)
	{
		//(x, y, z) = (sin(Pi * m/M) cos(2Pi * n/N), sin(Pi * m/M) sin(2Pi * n/N), cos(Pi * m/M))
		OutPositions.clear();
		OutIndices.clear();
		OutPositions.reserve(Longitude * (Latitude - 1) + 2);
		OutIndices.reserve(Longitude * (Latitude - 2) * 6 + Longitude * 2 * 3);

		// Calculate positions
		OutPositions.push_back(FFloat3(0, 0, 1.f));
		for (uint32 Lat = 1 ; Lat < Latitude; ++ Lat)
		{
			for (uint32 Long = 0 ; Long < Longitude ; ++ Long)
			{
				FFloat3 Pos;
				Pos.X = sin(PI * Lat / Latitude) * cos(PI * 2.f * Long / Longitude);
				Pos.Y = sin(PI * Lat / Latitude) * sin(PI * 2.f * Long / Longitude);
				Pos.Z = cos(PI * Lat / Latitude);
				OutPositions.push_back(Pos);
			}
		}
		OutPositions.push_back(FFloat3(0, 0, -1.f));

		// Create indices
		// First Latitude
		for (uint32 Tri = 0 ; Tri < Longitude ; ++ Tri)
		{
			OutIndices.push_back(0);
			OutIndices.push_back((Tri + 1) % Longitude + 1);
			OutIndices.push_back(Tri + 1);
		}

		// Middle faces
		for (uint32 i = 0; i < Latitude - 2; ++i)
		{
			int CurrLineStart = Longitude * i + 1;
			int NextLineStart = CurrLineStart + Longitude;

			for (uint32 l = 0; l < Longitude; ++l) {
				int curr0, curr1, next0, next1;
				curr0 = l + CurrLineStart;
				curr1 = (l + 1) % Longitude + CurrLineStart;
				next0 = l + NextLineStart;
				next1 = (l + 1) % Longitude + NextLineStart;
				OutIndices.push_back(curr0);
				OutIndices.push_back(curr1);
				OutIndices.push_back(next0);

				OutIndices.push_back(next0);
				OutIndices.push_back(curr1);
				OutIndices.push_back(next1);
			}
		}

		// Last Latitude
		int32 LastPoint = (int32)OutPositions.size() - 1;
		int32 LastLatStart = (Latitude - 2) * Longitude;
		for (uint32 Tri = 0; Tri < Longitude; ++Tri)
		{
			OutIndices.push_back(LastLatStart + Tri + 1);
			OutIndices.push_back(LastLatStart + (Tri + 1) % Longitude + 1);
			OutIndices.push_back(LastPoint);
		}
	}

	void _CreateUnitBox(TVector<FFloat3>& OutPositions, TVector<uint32>& OutIndices)
	{
		static const FFloat3 Points[] =
		{ 
			FFloat3(-0.5f, -0.5f, -0.5f),
			FFloat3( 0.5f, -0.5f, -0.5f),
			FFloat3( 0.5f, -0.5f, 0.5f),
			FFloat3(-0.5f, -0.5f, 0.5f),
			FFloat3(-0.5f, 0.5f, -0.5f),
			FFloat3( 0.5f, 0.5f, -0.5f),
			FFloat3( 0.5f, 0.5f, 0.5f),
			FFloat3(-0.5f, 0.5f, 0.5f)
		};
		static const uint32 Faces[] =
		{
			1, 5, 4,
			2, 6, 5,

			3, 7, 6,
			0, 4, 7,

			2, 1, 0,
			5, 6, 7,
			
			7, 4, 5,
			0, 3, 2,
			
			7, 3, 0,
			6, 2, 3,
			
			5, 1, 2,
			4, 0, 1
		};
		OutPositions.resize(8);
		memcpy(OutPositions.data(), Points, sizeof(Points));

		OutIndices.resize(36);
		memcpy(OutIndices.data(), Faces, sizeof(Faces));
	}

	void _CreateCapsule(uint32 Latitude, uint32 Longitude, TVector<FFloat3>& OutPositions, TVector<uint32>& OutIndices)
	{
		//(x, y, z) = (sin(Pi * m/M) cos(2Pi * n/N), sin(Pi * m/M) sin(2Pi * n/N), cos(Pi * m/M))
		OutPositions.clear();
		OutIndices.clear();
		OutPositions.reserve(Longitude * Latitude + 2);
		OutIndices.reserve(Longitude * (Latitude - 1) * 6 + Longitude * 2 * 3);

		// Calculate positions
		OutPositions.push_back(FFloat3(0, 0, 1.f));
		for (uint32 Lat = 1; Lat < Latitude; ++Lat)
		{
			for (uint32 Long = 0; Long < Longitude; ++Long)
			{
				FFloat3 Pos;
				Pos.X = sin(PI * Lat / Latitude) * cos(PI * 2.f * Long / Longitude);
				Pos.Y = sin(PI * Lat / Latitude) * sin(PI * 2.f * Long / Longitude);
				Pos.Z = cos(PI * Lat / Latitude);
				OutPositions.push_back(Pos);
			}
			if (Lat == Latitude / 2)
			{
				// duplicate middle latitude
				for (uint32 Long = 0; Long < Longitude; ++Long)
				{
					FFloat3 Pos;
					Pos.X = sin(PI * Lat / Latitude) * cos(PI * 2.f * Long / Longitude);
					Pos.Y = sin(PI * Lat / Latitude) * sin(PI * 2.f * Long / Longitude);
					Pos.Z = cos(PI * Lat / Latitude);
					OutPositions.push_back(Pos);
				}
			}
		}
		OutPositions.push_back(FFloat3(0, 0, -1.f));

		// Create indices
		// First Latitude
		for (uint32 Tri = 0; Tri < Longitude; ++Tri)
		{
			OutIndices.push_back(0);
			OutIndices.push_back((Tri + 1) % Longitude + 1);
			OutIndices.push_back(Tri + 1);
		}

		// Middle faces
		for (uint32 i = 0; i < Latitude - 1; ++i)
		{
			int CurrLineStart = Longitude * i + 1;
			int NextLineStart = CurrLineStart + Longitude;

			for (uint32 l = 0; l < Longitude; ++l) {
				int curr0, curr1, next0, next1;
				curr0 = l + CurrLineStart;
				curr1 = (l + 1) % Longitude + CurrLineStart;
				next0 = l + NextLineStart;
				next1 = (l + 1) % Longitude + NextLineStart;
				OutIndices.push_back(curr0);
				OutIndices.push_back(curr1);
				OutIndices.push_back(next0);

				OutIndices.push_back(next0);
				OutIndices.push_back(curr1);
				OutIndices.push_back(next1);
			}
		}

		// Last Latitude
		int32 LastPoint = (int32)OutPositions.size() - 1;
		int32 LastLatStart = (Latitude - 1) * Longitude;
		for (uint32 Tri = 0; Tri < Longitude; ++Tri)
		{
			OutIndices.push_back(LastLatStart + Tri + 1);
			OutIndices.push_back(LastLatStart + (Tri + 1) % Longitude + 1);
			OutIndices.push_back(LastPoint);
		}
	}

	/////////////////////////////////////////////////////////////

	void TShape::CreateICOSphere(
		uint32 Frequency,
		const FFloat3& Center, 
		float Radius, 
		TVector<FFloat3>& OutPositions, 
		TVector<uint32>& OutIndices)
	{
		static TVector<FFloat3> SpherePositions;
		static TVector<uint32> SphereIndices;
		static uint32 LastFrequency = 0;
		if (LastFrequency != Frequency)
		{
			SpherePositions.clear();
			SphereIndices.clear();
			_CreateICOSphere(Frequency, SpherePositions, SphereIndices);
			LastFrequency = Frequency;
		}

		const uint32 IndexOffset = (uint32)(OutPositions.size());
		for (const auto& P : SpherePositions)
		{
			FFloat3 NewP = P * Radius + Center;
			OutPositions.push_back(NewP);
		}
		for (const auto& I : SphereIndices)
		{
			uint32 Index = I + IndexOffset;
			OutIndices.push_back(Index);
		}
	} 

	void TShape::CreateBox(
		const FFloat3& Center,
		const FFloat3& Edges,
		const FQuat& Rotation,
		TVector<FFloat3>& OutPositions,
		TVector<uint32>& OutIndices)
	{
		static TVector<FFloat3> BoxPositions;
		static TVector<uint32> BoxIndices;
		if (BoxPositions.size() == 0)
		{
			_CreateUnitBox(BoxPositions, BoxIndices);
		}

		const uint32 IndexOffset = (uint32)(OutPositions.size());
		if (Rotation == FQuat())
		{
			for (const auto& P : BoxPositions)
			{
				FFloat3 NewP = P * Edges + Center;
				OutPositions.push_back(NewP);
			}
		}
		else
		{
			FMat4 RotMat;
			Rotation.GetMatrix(RotMat);

			for (const auto& P : BoxPositions)
			{
				FFloat3 NewP;
				RotMat.TransformVect(NewP, P * Edges);
				NewP += Center;
				OutPositions.push_back(NewP);
			}
		}
		for (const auto& I : BoxIndices)
		{
			uint32 Index = I + IndexOffset;
			OutIndices.push_back(Index);
		}
	}

	void TShape::CreateCapsule(
		uint32 Latitude,
		uint32 Longitude,
		const FFloat3& Center,
		float Radius,
		float Length,
		const FQuat& Rotation,
		TVector<FFloat3>& OutPositions,
		TVector<uint32>& OutIndices)
	{
		static TVector<FFloat3> CapsulePositions;
		static TVector<uint32> CapsuleIndices;
		static FInt2 LatLong;
		if (LatLong != FInt2(Latitude, Longitude))
		{
			CapsulePositions.clear();
			CapsuleIndices.clear();
			_CreateCapsule(Latitude, Longitude, CapsulePositions, CapsuleIndices);
			LatLong = FInt2(Latitude, Longitude);
		}

		// Apply radius and length
		const uint32 TotalPoints = (uint32)CapsulePositions.size();
		const uint32 HalfPoints = TotalPoints / 2;
		const float HalfLength = Length * 0.5f;
		for (uint32 i = 0 ; i < HalfPoints ; ++ i)
		{
			FFloat3& P = CapsulePositions[i];
			P = P * Radius;
			P.Z += HalfLength;
		}
		for (uint32 i = HalfPoints ; i < TotalPoints ; ++ i)
		{
			FFloat3& P = CapsulePositions[i];
			P = P * Radius;
			P.Z -= HalfLength;
		}

		// Apply rotation
		const uint32 IndexOffset = (uint32)(OutPositions.size());
		if (Rotation == FQuat())
		{
			for (const auto& P : CapsulePositions)
			{
				FFloat3 NewP = P + Center;
				OutPositions.push_back(NewP);
			}
		}
		else
		{
			FMat4 RotMat;
			Rotation.GetMatrix(RotMat);

			for (const auto& P : CapsulePositions)
			{
				FFloat3 NewP;
				RotMat.TransformVect(NewP, P);
				NewP += Center;
				OutPositions.push_back(NewP);
			}
		}
		for (const auto& I : CapsuleIndices)
		{
			uint32 Index = I + IndexOffset;
			OutIndices.push_back(Index);
		}
	}

	void TShape::RecalcNormal(
		const TVector<FFloat3>& Positions,
		const TVector<uint32>& Indices,
		TVector<FFloat3>& OutNormals)
	{
		OutNormals.clear();
		OutNormals.resize(Positions.size());

		const uint32 TotalIndices = (uint32)Indices.size();
		for (uint32 i = 0 ; i < TotalIndices ; i += 3)
		{
			uint32 i0 = Indices[i];
			uint32 i1 = Indices[i + 1];
			uint32 i2 = Indices[i + 2];

			const FFloat3& P0 = Positions[i0];
			const FFloat3& P1 = Positions[i1];
			const FFloat3& P2 = Positions[i2];

			FFloat3& N0 = OutNormals[i0];
			FFloat3& N1 = OutNormals[i1];
			FFloat3& N2 = OutNormals[i2];

			FFloat3 N;
			N = (P1 - P0).Cross(P2 - P0);
			N0 += N.Normalize();
			N = (P2 - P1).Cross(P0 - P1);
			N1 += N.Normalize();
			N = (P0 - P2).Cross(P1 - P2);
			N2 += N.Normalize();
		}

		for (auto& N : OutNormals)
		{
			N.Normalize();
		}
	}
}