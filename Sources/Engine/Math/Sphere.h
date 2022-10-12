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
	static inline void ConstructFromSpheres(FSphere<T>& ThisSphere, const FSphere<T>* Spheres, int32 Count)
	{
		TI_ASSERT(Count > 0);

		// Min/max points of AABB
		int32 MinIndex[3] = { 0, 0, 0 };
		int32 MaxIndex[3] = { 0, 0, 0 };

		for (int i = 0; i < Count; i++)
		{
			for (int k = 0; k < 3; k++)
			{
				MinIndex[k] = Spheres[i].Center[k] - Spheres[i].W < Spheres[MinIndex[k]].Center[k] - Spheres[MinIndex[k]].W ? i : MinIndex[k];
				MaxIndex[k] = Spheres[i].Center[k] + Spheres[i].W > Spheres[MaxIndex[k]].Center[k] + Spheres[MaxIndex[k]].W ? i : MaxIndex[k];
			}
		}

		T LargestDist = 0.0f;
		int32 LargestAxis = 0;
		for (int k = 0; k < 3; k++)
		{
			FSphere<T> SphereMin = Spheres[MinIndex[k]];
			FSphere<T> SphereMax = Spheres[MaxIndex[k]];

			T Dist = (SphereMax.Center - SphereMin.Center).GetLength() + SphereMin.W + SphereMax.W;
			if (Dist > LargestDist)
			{
				LargestDist = Dist;
				LargestAxis = k;
			}
		}

		ThisSphere = Spheres[MinIndex[LargestAxis]];
		ThisSphere += Spheres[MaxIndex[LargestAxis]];

		// Adjust to fit all spheres
		for (int i = 0; i < Count; i++)
		{
			ThisSphere += Spheres[i];
		}
	}

	template<typename T>
	class FSphere
	{
	public:
		FSphere()
			: W(0)
		{}

		FSphere(const FVec3<T>& InCenter, T InRadius)
			: Center(InCenter)
			, W(InRadius)
		{}

		FSphere(const FVec3<T>*Points, int32 Count)
		{
			ConstructFromPoints(*this, Points, Count);
		}

		FSphere(const FSphere<T>* Spheres, int32 Count)
		{
			ConstructFromSpheres(*this, Spheres, Count);
		}

		bool IsPointInsideSphere(const FVec3<T>& P) const
		{
			return (Center - P).GetLength() <= W;
		}

		/**
		 * Check whether sphere is inside of another.
		 *
		 * @param Other The other sphere.
		 * @param Tolerance Error Tolerance.
		 * @return true if sphere is inside another, otherwise false.
		 */
		bool IsSphereInside(const FSphere<T>& Other, T Tolerance = KINDA_SMALL_NUMBER) const
		{
			if (W > Other.W + Tolerance)
			{
				return false;
			}

			return (Center - Other.Center).GetLengthSQ() <= TMath::Sqr(Other.W + Tolerance - W);
		}

		bool operator == (const FSphere<T>& Other) const
		{
			return Center == Other.Center && W == Other.W;
		}

		FSphere<T>& operator+=(const FSphere<T>& Other)
		{
			if (W == 0.f)
			{
				*this = Other;
				return *this;
			}

			FVec3<T> ToOther = Other.Center - Center;
			T DistSqr = ToOther.GetLengthSQ();

			if (TMath::Sqr(W - Other.W) + KINDA_SMALL_NUMBER >= DistSqr)
			{
				// Pick the smaller
				if (W < Other.W)
				{
					*this = Other;
				}
			}
			else
			{
				T Dist = TMath::Sqrt(DistSqr);

				FSphere<T> NewSphere;
				NewSphere.W = (Dist + Other.W + W) * 0.5f;
				NewSphere.Center = Center;

				if (Dist > SMALL_NUMBER)
				{
					NewSphere.Center += ToOther * ((NewSphere.W - W) / Dist);
				}

				// make sure both are inside afterwards
				TI_ASSERT(Other.IsSphereInside(NewSphere, 1.f));
				TI_ASSERT(IsSphereInside(NewSphere, 1.f));

				*this = NewSphere;
			}

			return *this;
		}

	public:
		FVec3<T> Center;
		T W = 0;
	};

	typedef FSphere<float> FSpheref;
}