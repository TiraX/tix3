/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	template <class T>
	class FLine3D
	{
	public:
		FLine3D()
			: Start(0, 0, 0)
			, End(1, 1, 1)
		{}

		FLine3D(T xa, T ya, T za, T xb, T yb, T zb) 
			: Start(xa, ya, za)
			, End(xb, yb, zb) 
		{}

		FLine3D(const FVec3<T>& Start, const FVec3<T>& End) 
			: Start(Start)
			, End(End) 
		{}

		FLine3D<T> operator+(const FVec3<T>& point) const 
		{ 
			return FLine3D<T>(Start + point, End + point); 
		}

		FLine3D<T>& operator+=(const FVec3<T>& point) 
		{ 
			Start += point; 
			End += point; 
			return *this; 
		}

		FLine3D<T> operator-(const FVec3<T>& point) const 
		{ 
			return FLine3D<T>(Start - point, End - point); 
		}
		
		FLine3D<T>& operator-=(const FVec3<T>& point) 
		{ 
			Start -= point; 
			End -= point; 
			return *this; 
		}

		bool operator==(const FLine3D<T>& other) const
		{
			return 
				(Start == other.Start && End == other.End) || 
				(End == other.Start && Start == other.End);
		}

		bool operator!=(const FLine3D<T>& other) const
		{
			return 
				!(Start == other.Start && End == other.End) || 
				(End == other.Start && Start == other.End);
		}

		void SetLine(const T& xa, const T& ya, const T& za, const T& xb, const T& yb, const T& zb)
		{
			Start.Set(xa, ya, za); End.Set(xb, yb, zb);
		}

		void SetLine(const FVec3<T>& nstart, const FVec3<T>& nend)
		{
			Start.set(nstart); End.set(nend);
		}

		void SetLine(const FLine3D<T>& line)
		{
			Start.set(line.Start); End.set(line.End);
		}

		T GetLength() const { return Start.getDistanceFrom(End); }
		T GetLengthSQ() const { return Start.getDistanceFromSQ(End); }

		FVec3<T> GetMiddle() const
		{
			return (Start + End) * (T)0.5;
		}

		FVec3<T> GetVector() const
		{
			return End - Start;
		}

		bool IsPointBetweenStartAndEnd(const FVec3<T>& point) const
		{
			return point.IsBetweenPoints(Start, End);
		}

		//! Get the closest point on this line to a point
		/** \param point The point to compare to.
			\return The nearest point which is part of the line. */
		FVec3<T> GetClosestPoint(const FVec3<T>& point) const
		{
			FVec3<T> c = point - Start;
			FVec3<T> v = End - Start;
			T d = (T)v.getLength();
			v /= d;
			T t = v.Dot(c);

			if (t < (T)0.0)
				return Start;
			if (t > d)
				return End;

			v *= t;
			return Start + v;
		}

		//! Check if the line intersects with a shpere
		/** \param sorigin Origin of the shpere.
			\param sradius Radius of the sphere.
			\param outdistance The distance to the first intersection point.
			\return True if there is an intersection.
			If there is one, the distance to the first intersection point
			is stored in outdistance. */
		bool GetIntersectionWithSphere(FVec3<T> sorigin, T sradius, float32& outdistance) const
		{
			const FVec3<T> q = sorigin - Start;
			T c = q.GetLength();
			T v = q.Dot(GetVector().Normalize());
			T d = sradius * sradius - (c * c - v * v);

			if (d < 0.0)
				return false;

			outdistance = v - TMath::Sqrt((float32)d);
			return true;
		}

		FVec3<T> Start, End;
	};

	typedef FLine3D<float32> FLine3;

}
