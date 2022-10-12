/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	template<typename T>
	class FSphere;

	// [ Ritter 1990, "An Efficient Bounding Sphere" ]
	template<typename T>
	static inline void ConstructFromPoints(FSphere<T>& ThisSphere, const FVec3<T>* Points, int32 Count)
	{
		TI_ASSERT(Count > 0);

		// Min/max points of AABB
		int32 MinIndex[3] = { 0, 0, 0 };
		int32 MaxIndex[3] = { 0, 0, 0 };

		for (int i = 0; i < Count; i++)
		{
			for (int k = 0; k < 3; k++)
			{
				MinIndex[k] = Points[i][k] < Points[MinIndex[k]][k] ? i : MinIndex[k];
				MaxIndex[k] = Points[i][k] > Points[MaxIndex[k]][k] ? i : MaxIndex[k];
			}
		}

		T LargestDistSqr = 0.0f;
		int32 LargestAxis = 0;
		for (int k = 0; k < 3; k++)
		{
			FVec3<T> PointMin = Points[MinIndex[k]];
			FVec3<T> PointMax = Points[MaxIndex[k]];

			T DistSqr = (PointMax - PointMin).GetLengthSQ();
			if (DistSqr > LargestDistSqr)
			{
				LargestDistSqr = DistSqr;
				LargestAxis = k;
			}
		}

		FVec3<T> PointMin = Points[MinIndex[LargestAxis]];
		FVec3<T> PointMax = Points[MaxIndex[LargestAxis]];

		ThisSphere.Center = 0.5f * (PointMin + PointMax);
		ThisSphere.W = 0.5f * TMath::Sqrt(LargestDistSqr);
		T WSqr = ThisSphere.W * ThisSphere.W;

		// Adjust to fit all points
		for (int i = 0; i < Count; i++)
		{
			T DistSqr = (Points[i] - ThisSphere.Center).GetLengthSQ();

			if (DistSqr > WSqr)
			{
				T Dist = TMath::Sqrt(DistSqr);
				T t = 0.5f + 0.5f * (ThisSphere.W / Dist);

				ThisSphere.Center = TMath::LerpStable(Points[i], ThisSphere.Center, t);
				ThisSphere.W = 0.5f * (ThisSphere.W + Dist);
			}
		}
	}

	template<typename T>
	class FSphere
	{
	public:
		FSphere()
		{}

		FSphere(const FVec3<T>& InCenter, T InRadius)
			: Center(InCenter)
			, W(InRadius)
		{}

		FSphere(const FVec3<T>* Points, int32 Count)
		{
			ConstructFromPoints(*this, Points, Count);
		}

		bool IsPointInsideSphere(const FVec3<T>& P) const
		{
			return (Center - P).GetLength() <= W;
		}

		bool operator == (const FSphere<T>& Other) const
		{
			return Center == Other.Center && W == Other.W;
		}

	public:
		FVec3<T> Center;
		T W = 0;
	};

	typedef FSphere<float> FSpheref;
}