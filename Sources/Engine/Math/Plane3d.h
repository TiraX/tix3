/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{

	//! Enumeration for intersection relations of 3d objects
	enum EIntersectionRelation3D
	{
		ISREL3D_FRONT = 0,
		ISREL3D_BACK,
		ISREL3D_PLANAR,
		ISREL3D_SPANNING,
		ISREL3D_CLIPPED
	};

	//! Template plane class with some intersection testing methods.
	template <class T>
	class FPlane3D
	{
	public:
		FPlane3D()
			: Normal(0, 1, 0)
		{
			RecalculateD(FVec3<T>(0, 0, 0));
		}

		FPlane3D(const FVec3<T>& MPoint, const FVec3<T>& Normal)
			: Normal(Normal)
		{
			RecalculateD(MPoint);
		}

		FPlane3D(T px, T py, T pz, T nx, T ny, T nz)
			: Normal(nx, ny, nz)
		{
			RecalculateD(FVec3<T>(px, py, pz));
		}

		FPlane3D(const FVec3<T>& point1, const FVec3<T>& point2, const FVec3<T>& point3)
		{
			SetPlane(point1, point2, point3);
		}

		inline bool operator==(const FPlane3D<T>& other) const
		{
			return (D == other.D && Normal == other.Normal);
		}

		inline bool operator!=(const FPlane3D<T>& other) const
		{
			return !(D == other.D && Normal == other.Normal);
		}

		void SetPlane(const FVec3<T>& point, const FVec3<T>& nvector)
		{
			Normal = nvector;
			RecalculateD(point);
		}

		void SetPlane(const FVec3<T>& nvect, T d)
		{
			Normal = nvect;
			D = d;
		}

		void SetPlane(const FVec3<T>& point1, const FVec3<T>& point2, const FVec3<T>& point3)
		{
			// creates the plane from 3 memberpoints
			Normal = (point2 - point1).Cross(point3 - point1);
			Normal.Normalize();

			RecalculateD(point1);
		}

		//! Get an intersection with a 3d line.
		/** \param lineVect Vector of the line to intersect with.
			\param linePoint Point of the line to intersect with.
			\param outIntersection Place to store the intersection point, if there is one.
			\return True if there was an intersection, false if there was not.
		*/
		bool GetIntersectionWithLine(const FVec3<T>& linePoint,
			const FVec3<T>& lineVect,
			FVec3<T>& outIntersection) const
		{
			T t2 = Normal.Dot(lineVect);

			if (t2 == 0)
				return false;

			T t = -(Normal.Dot(linePoint) + D) / t2;
			outIntersection = linePoint + (lineVect * t);
			return true;
		}

		//! Get percentage of line between two points where an intersection with this plane happens.
		/** Only useful if known that there is an intersection.
			\param linePoint1 Point1 of the line to intersect with.
			\param linePoint2 Point2 of the line to intersect with.
			\return Where on a line between two points an intersection with this plane happened.
			For example, 0.5 is returned if the intersection happened exactly in the middle of the two points.
		*/
		float32 GetKnownIntersectionWithLine(const FVec3<T>& linePoint1,
			const FVec3<T>& linePoint2) const
		{
			FVec3<T> vect = linePoint2 - linePoint1;
			T t2 = (float32)Normal.Dot(vect);
			return (float32)-((Normal.Dot(linePoint1) + D) / t2);
		}

		//! Get an intersection with a 3d line, limited between two 3d points.
		/** \param linePoint1 Point 1 of the line.
			\param linePoint2 Point 2 of the line.
			\param outIntersection Place to store the intersection point, if there is one.
			\return True if there was an intersection, false if there was not.
		*/
		bool GetIntersectionWithLimitedLine(
			const FVec3<T>& linePoint1,
			const FVec3<T>& linePoint2,
			FVec3<T>& outIntersection) const
		{
			return (getIntersectionWithLine(linePoint1, linePoint2 - linePoint1, outIntersection) &&
				outIntersection.isBetweenPoints(linePoint1, linePoint2));
		}

		//! Classifies the relation of a point to this plane.
		/** \param point Point to classify its relation.
			\return ISREL3D_FRONT if the point is in front of the plane,
			ISREL3D_BACK if the point is behind of the plane, and
			ISREL3D_PLANAR if the point is within the plane. */
		EIntersectionRelation3D ClassifyPointRelation(const FVec3<T>& point) const
		{
			const T d = Normal.Dot(point) + D;

			if (d < -ROUNDING_ERROR_32)
				return ISREL3D_BACK;

			if (d > ROUNDING_ERROR_32)
				return ISREL3D_FRONT;

			return ISREL3D_PLANAR;
		}

		//! Recalculates the distance from origin by applying a newly member point to the plane.
		void RecalculateD(const FVec3<T>& MPoint)
		{
			D = -MPoint.Dot(Normal);
		}

		//! Gets a member point of the plane.
		FVec3<T> GetMemberPoint() const
		{
			return Normal * -D;
		}

		//! Tests if there is an intersection with the other plane
		/** \return True if there is a intersection. */
		bool ExistsIntersection(const FPlane3D<T>& other) const
		{
			FVec3<T> cross = other.Normal.Cross(Normal);
			return cross.getLength() > ROUNDING_ERROR_32;
		}

		//! Intersects this plane with another.
		/** \param other Other plane to intersect with.
			\param outLinePoint Base point of intersection line.
			\param outLineVect Vector of intersection.
			\return True if there is a intersection, false if not. */
		bool GetIntersectionWithPlane(const FPlane3D<T>& other,
			FVec3<T>& outLinePoint,
			FVec3<T>& outLineVect) const
		{
			const T fn00 = Normal.GetLength();
			const T fn01 = Normal.Dot(other.Normal);
			const T fn11 = other.Normal.GetLength();
			const float64 det = fn00 * fn11 - fn01 * fn01;

			if (fabs(det) < ROUNDING_ERROR_64)
				return false;

			const float64 invdet = 1.0 / det;
			const float64 fc0 = (fn11 * -D + fn01 * other.D) * invdet;
			const float64 fc1 = (fn00 * -other.D + fn01 * D) * invdet;

			outLineVect = Normal.Cross(other.Normal);
			outLinePoint = Normal * (T)fc0 + other.Normal * (T)fc1;
			return true;
		}

		//! Get the intersection point with two other planes if there is one.
		bool GetIntersectionWithPlanes(const FPlane3D<T>& o1,
			const FPlane3D<T>& o2, FVec3<T>& outPoint) const
		{
			FVec3<T> linePoint, lineVect;
			if (GetIntersectionWithPlane(o1, linePoint, lineVect))
				return o2.GetIntersectionWithLine(linePoint, lineVect, outPoint);

			return false;
		}

		//! Test if the triangle would be front or backfacing from any point.
		/** Thus, this method assumes a camera position from
			which the triangle is definitely visible when looking into
			the given direction.
			Note that this only works if the normal is Normalized.
			Do not use this method with points as it will give wrong results!
			\param lookDirection Look direction.
			\return True if the plane is front facing and
			false if it is backfacing. */
		bool IsFrontFacing(const FVec3<T>& lookDirection) const
		{
			const float32 d = Normal.Dot(lookDirection);
			return F32_LOWER_EQUAL_0(d);
		}

		//! Get the distance to a point.
		/** Note that this only works if the normal is normalized. */
		T GetDistanceTo(const FVec3<T>& point) const
		{
			return point.Dot(Normal) + D;
		}

		//! Normal vector of the plane.
		FVec3<T> Normal;
		//! Distance from origin.
		T D;
	};

	typedef FPlane3D<float32> FPlane;

}
