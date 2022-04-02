/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	// Axis aligned bounding box in 3d dimensional space.
	template <class T>
	class FAABBox
	{
	public:
		FAABBox()
		{}

		FAABBox(const FVec3<T>& min, const FVec3<T>& max)
			: Min(min)
			, Max(max)
		{}

		explicit FAABBox(const FVec3<T>& init)
			: Min(init)
			, Max(init)
		{}

		FAABBox(T minx, T miny, T minz, T maxx, T maxy, T maxz)
			: Min(minx, miny, minz)
			, Max(maxx, maxy, maxz)
		{}

		inline bool operator==(const FAABBox<T>& other) const
		{
			return (Min == other.Min && other.Max == Max);
		}

		inline bool operator!=(const FAABBox<T>& other) const
		{
			return !(Min == other.Min && other.Max == Max);
		}

		// overload operator< for std::map compare
		bool operator<(const FAABBox<T>& other) const
		{
			if (Min != other.Min)
				return Min < other.Min;
			return Max < other.Max;
		}

		inline FAABBox<T>& operator+=(const FVec3<T>& other)
		{
			Min += other;
			Max += other;
			return *this;
		}

		void AddInternalPoint(const FVec3<T>& p)
		{
			AddInternalPoint(p.X, p.Y, p.Z);
		}

		void AddInternalBox(const FAABBox<T>& b)
		{
			AddInternalPoint(b.Max);
			AddInternalPoint(b.Min);
		}

		void Reset(T x, T y, T z)
		{
			Max.set(x, y, z);
			Min = Max;
		}

		void Reset(const FAABBox<T>& initValue)
		{
			*this = initValue;
		}

		void Reset(const FVec3<T>& initValue)
		{
			Max = initValue;
			Min = initValue;
		}

		void Invalidate()
		{
			Max = FVec3<T>(std::numeric_limits<T>::min(),
				std::numeric_limits<T>::min(),
				std::numeric_limits<T>::min());
			Min = FVec3<T>(std::numeric_limits<T>::max(),
				std::numeric_limits<T>::max(),
				std::numeric_limits<T>::max());
		}

		void AddInternalPoint(T x, T y, T z)
		{
			if (x > Max.X) Max.X = x;
			if (y > Max.Y) Max.Y = y;
			if (z > Max.Z) Max.Z = z;

			if (x < Min.X) Min.X = x;
			if (y < Min.Y) Min.Y = y;
			if (z < Min.Z) Min.Z = z;
		}

		bool IsPointInside(const FVec3<T>& p) const
		{
			return (p.X >= Min.X && p.X <= Max.X &&
				p.Y >= Min.Y && p.Y <= Max.Y &&
				p.Z >= Min.Z && p.Z <= Max.Z);
		}

		bool IsPointTotalInside(const FVec3<T>& p) const
		{
			return (p.X > Min.X && p.X < Max.X&&
				p.Y > Min.Y && p.Y < Max.Y&&
				p.Z > Min.Z && p.Z < Max.Z);
		}

		bool IntersectsWithBox(const FAABBox<T>& other) const
		{
			// Min <= other.Max && Max >= other.Min
			return Min.X <= other.Max.X && Min.Y <= other.Max.Y && Min.Z <= other.Max.Z &&
				Max.X >= other.Min.X && Max.Y >= other.Min.Y && Max.Z >= other.Min.Z;
		}

		bool IsFullInside(const FAABBox<T>& other) const
		{
			// Min >= other.Min && Max <= other.Max;
			return Min.X >= other.Min.X && Min.Y >= other.Min.Y && Min.Z >= other.Min.Z &&
				Max.X <= other.Max.X && Max.Y <= other.Max.Y && Max.Z <= other.Max.Z;
		}

		bool IntersectsWithPoint(const FVec3<T>& point) const
		{
			bool r;
			r = (Min.X <= point.X && point.X <= Max.X);
			r &= (Min.Y <= point.Y && point.Y <= Max.Y);
			r &= (Min.Z <= point.Z && point.Z <= Max.Z);
			return r;
		}

		bool IntersectsWithLine(const FVec3<T>& linemiddle,
			const FVec3<T>& linevect,
			T halflength) const
		{
			const FVec3<T> e = GetExtent() * (T)0.5;
			const FVec3<T> t = GetCenter() - linemiddle;

			if ((fabs(t.X) > e.X + halflength * fabs(linevect.X)) ||
				(fabs(t.Y) > e.Y + halflength * fabs(linevect.Y)) ||
				(fabs(t.Z) > e.Z + halflength * fabs(linevect.Z)))
				return false;

			T r = e.Y * (T)fabs(linevect.Z) + e.Z * (T)fabs(linevect.Y);
			if (fabs(t.Y * linevect.Z - t.Z * linevect.Y) > r)
				return false;

			r = e.X * (T)fabs(linevect.Z) + e.Z * (T)fabs(linevect.X);
			if (fabs(t.Z * linevect.X - t.X * linevect.Z) > r)
				return false;

			r = e.X * (T)fabs(linevect.Y) + e.Y * (T)fabs(linevect.X);
			if (fabs(t.X * linevect.Y - t.Y * linevect.X) > r)
				return false;

			return true;
		}

		bool IntersectsWithLine_impl_1d(T  bmin,		// min value of the bounding box
			T  bmax,		// max value of the bounding box
			T  si,		    // start of the line segment
			T  ei,         // end of the line segment
			T& fst,        // given start value to compare (start with 0)
			T& fet         // given end value to compare (start with 1)
		) const
		{
			// The algorithm need to know which of the start or the end of the 
			// segment is smaller; the variable could be swapped, but it's faster
			// to duplicate the code.
			T   st, et;
			T   di = ei - si;

			if (si < ei)
			{
				if (si > bmax || ei < bmin)
					return false;   // outside AABB
				st = (si < bmin) ? (bmin - si) / di : 0; // cut / inclusion 
				et = (ei > bmax) ? (bmax - si) / di : 1; // cut / inclusion 
			}
			else
			{
				if (ei > bmax || si < bmin)
					return false;   //  outside AABB
				st = (si > bmax) ? (bmax - si) / di : 0;  // cut / inclusion  
				et = (ei < bmin) ? (bmin - si) / di : 1;  // cut / inclusion
			}

			if (st > fst)   // Compare with prev results - the furthest the start, the better
				fst = st;
			if (et < fet)   // Compare with prev results - the closest the end, the better
				fet = et;

			if (fet < fst)  // result turned to be outside.
				return false;

			return true;    // collision exist
		}

		bool IntersectsWithLine(const FLine3D<T>& line) const
		{
			T fst, fet;
			return IntersectsWithSegment(line, fst, fet);
		}

		bool IntersectsWithSegment(const FLine3D<T>& line, T& fst, T& fet) const
		{
			fst = 0;
			fet = 1;
			return IntersectsWithLine_impl_1d(Min.X, Max.X, line.Start.X, line.End.X, fst, fet)
				&& IntersectsWithLine_impl_1d(Min.Y, Max.Y, line.Start.Y, line.End.Y, fst, fet)
				&& IntersectsWithLine_impl_1d(Min.Z, Max.Z, line.Start.Z, line.End.Z, fst, fet);
		}

		FVec3<T> GetCenter() const
		{
			return (Min + Max) / 2;
		}

		FVec3<T> GetExtent() const
		{
			return Max - Min;
		}

		void Extend(float ratio)
		{
			FVec3<T> center = GetCenter();
			FVec3<T> extent = GetExtent();
			extent *= ratio * 0.5f;
			Max = center + extent;
			Min = center - extent;
		}

		void Extend(float x, float y, float z)
		{
			FVec3<T> center = GetCenter();
			FVec3<T> extent = GetExtent();
			extent *= FFloat3(x, y, z) * 0.5f;
			Max = center + extent;
			Min = center - extent;
		}

		bool IsEmpty() const
		{
			return Min.Equals(Max);
		}

		void Repair()
		{
			if (Min.X > Max.X)
			{
				TMath::Swap(Min.X, Max.X);
			}
			if (Min.Y > Max.Y)
			{
				TMath::Swap(Min.Y, Max.Y);
			}
			if (Min.Z > Max.Z)
			{
				TMath::Swap(Min.Z, Max.Z);
			}
		}

		T GetVolume() const
		{
			const FVec3<T> e = GetExtent();
			return e.X * e.Y * e.Z;
		}

		T GetArea() const
		{
			const FVec3<T> e = GetExtent();
			return 2 * (e.X * e.Y + e.X * e.Z + e.Y * e.Z);
		}

		void Move(const FVec3<T>& pos)
		{
			Min += pos;
			Max += pos;
		}

		FVec3<T> Min, Max;
	};

	//! Typedef for a float32 3d bounding box.
	typedef FAABBox<float> FBox;

}
