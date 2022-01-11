// Copyright (C) 2002-2010 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __IRR_TRIANGLE_3D_H_INCLUDED__
#define __IRR_TRIANGLE_3D_H_INCLUDED__

namespace tix
{

	//! 3d triangle template class for doing collision detection and other things.
	template <class T>
	class triangle3d
	{
	public:

		//! Constructor for an all 0 triangle
		triangle3d() {}
		//! Constructor for triangle with given three vertices
		triangle3d(FVec3<T> v1, FVec3<T> v2, FVec3<T> v3) : pointA(v1), pointB(v2), pointC(v3) {}

		//! Equality operator
		bool operator==(const triangle3d<T>& other) const
		{
			return other.pointA==pointA && other.pointB==pointB && other.pointC==pointC;
		}

		//! Inequality operator
		bool operator!=(const triangle3d<T>& other) const
		{
			return !(*this==other);
		}

		//! Determines if the triangle is totally inside a bounding box.
		/** \param box Box to check.
		\return True if triangle is within the box, otherwise false. */
		bool isTotalInsideBox(const FAABBox<T>& box) const
		{
			return (box.isPointInside(pointA) &&
				box.isPointInside(pointB) &&
				box.isPointInside(pointC));
		}

		//! Determines if the triangle is totally outside a bounding box.
		/** \param box Box to check.
		\return True if triangle is outside the box, otherwise false. */
		bool isTotalOutsideBox(const FAABBox<T>& box) const
		{
			return ((pointA.X > box.Max.X && pointB.X > box.Max.X && pointC.X > box.Max.X) ||

				(pointA.Y > box.Max.Y && pointB.Y > box.Max.Y && pointC.Y > box.Max.Y) ||
				(pointA.Z > box.Max.Z && pointB.Z > box.Max.Z && pointC.Z > box.Max.Z) ||
				(pointA.X < box.Min.X && pointB.X < box.Min.X && pointC.X < box.Min.X) ||
				(pointA.Y < box.Min.Y && pointB.Y < box.Min.Y && pointC.Y < box.Min.Y) ||
				(pointA.Z < box.Min.Z && pointB.Z < box.Min.Z && pointC.Z < box.Min.Z));
		}

		//! Determines if the triangle is intersect with a bounding box
		// For any two convex meshes, to find whether they intersect, 
		// you need to check if there exist a separating plane.
		// If it does, they do not intersect.The plane can be picked from 
		// any face of either shape, or the edge cross - products.
		// The plane is defined as a normal and an offset from Origo.
		// So, you only have to check three faces of the AABB, and one face of the triangle.
		//https://stackoverflow.com/questions/17458562/efficient-aabb-triangle-intersection-in-c-sharp
		/** \param box Box to check.
		\return True if triangle is intersect the box, otherwise false. */
		bool isIntersectWithBox(const FAABBox<T>& box) const
		{
			T TriangleMin, TriangleMax;
			T BoxMin, BoxMax;

			// Test the box normals (x-, y- and z-axes)
			FVec3<T> BoxNormals[3] =
			{
				FVec3<T>(1,0,0),
				FVec3<T>(0,1,0),
				FVec3<T>(0,0,1)
			};

			auto ProjectTriangle = [](const triangle3d<T>& Tri, const FVec3<T>& Axis, T& MinValue, T& MaxValue)
			{
				MinValue = FLT_MAX;
				MaxValue = FLT_MIN;

				const FVec3<T> Points[] =
				{
					Tri.pointA,
					Tri.pointB,
					Tri.pointC
				};

				for (int32 i = 0; i < 3; ++i)
				{
					T V = Axis.Dot(Points[i]);
					if (V < MinValue)
						MinValue = V;
					if (V > MaxValue)
						MaxValue = V;
				}
			};
			auto ProjecFAABBox = [](const FAABBox<T>& Box, const FVec3<T>& Axis, T& MinValue, T& MaxValue)
			{
				MinValue = FLT_MAX;
				MaxValue = FLT_MIN;

				const FVec3<T> Points[] =
				{
					FVec3<T>(Box.Min.X, Box.Min.Y, Box.Min.Z),
					FVec3<T>(Box.Max.X, Box.Min.Y, Box.Min.Z),
					FVec3<T>(Box.Min.X, Box.Max.Y, Box.Min.Z),
					FVec3<T>(Box.Max.X, Box.Max.Y, Box.Min.Z),

					FVec3<T>(Box.Min.X, Box.Min.Y, Box.Max.Z),
					FVec3<T>(Box.Max.X, Box.Min.Y, Box.Max.Z),
					FVec3<T>(Box.Min.X, Box.Max.Y, Box.Max.Z),
					FVec3<T>(Box.Max.X, Box.Max.Y, Box.Max.Z)
				};

				for (int32 i = 0; i < 8; ++i)
				{
					T V = Axis.Dot(Points[i]);
					if (V < MinValue)
						MinValue = V;
					if (V > MaxValue)
						MaxValue = V;
				}
			};
			for (int32 i = 0; i < 3; i++)
			{
				const FVec3<T>& N = BoxNormals[i];
				ProjectTriangle(*this, BoxNormals[i], TriangleMin, TriangleMax);
				if (TriangleMax < box.Min[i] || TriangleMin > box.Max[i])
					return false; // No intersection possible.
			}

			// Test the triangle normal
			FVec3<T> TriN = getNormal().Normalize();
			T TriangleOffset = TriN.Dot(pointA);
			ProjecFAABBox(box, TriN, BoxMin, BoxMax);
			if (BoxMax < TriangleOffset || BoxMin > TriangleOffset)
				return false; // No intersection possible.

			// Test the nine edge cross-products
			FVec3<T> TriangleEdges[] =
			{
				pointA - pointB,
				pointB - pointC,
				pointC - pointA
			};
			for (int i = 0; i < 3; i++)
			{
				for (int j = 0; j < 3; j++)
				{
					// The box normals are the same as it's edge tangents
					FVec3<T> Axis = TriangleEdges[i].Cross(BoxNormals[j]);
					ProjecFAABBox(box, Axis, BoxMin, BoxMax);
					ProjectTriangle(*this, Axis, TriangleMin, TriangleMax);
					if (BoxMax <= TriangleMin || BoxMin >= TriangleMax)
						return false; // No intersection possible
				}
			}

			// No separating axis found.
			return true;
		}

		//! Get the closest point on a triangle to a point on the same plane.
		/** \param p Point which must be on the same plane as the triangle.
		\return The closest point of the triangle */
		FVec3<T> closestPointOnTriangle(const FVec3<T>& p) const
		{
			const FVec3<T> rab = line3d<T>(pointA, pointB).getClosestPoint(p);
			const FVec3<T> rbc = line3d<T>(pointB, pointC).getClosestPoint(p);
			const FVec3<T> rca = line3d<T>(pointC, pointA).getClosestPoint(p);

			const T d1 = rab.getDistanceFrom(p);
			const T d2 = rbc.getDistanceFrom(p);
			const T d3 = rca.getDistanceFrom(p);

			if (d1 < d2)
				return d1 < d3 ? rab : rca;

			return d2 < d3 ? rbc : rca;
		}

		//! Check if a point is inside the triangle (border-points count also as inside)
		/** NOTE: When working with T='int' you should prefer isPointInsideFast, as 
		isPointInside will run into number-overflows already with coordinates in the 3-digit-range.
		\param p Point to test. Assumes that this point is already
		on the plane of the triangle.
		\return True if the point is inside the triangle, otherwise false. */
		bool isPointInside(const FVec3<T>& p) const
		{
			return (isOnSameSide(p, pointA, pointB, pointC) &&
 				isOnSameSide(p, pointB, pointA, pointC) &&
 				isOnSameSide(p, pointC, pointA, pointB));
		}

		//! Check if a point is inside the triangle (border-points count also as inside)
		/** This method uses a barycentric coordinate system. 
		It is faster than isPointInside but is more susceptible to floating point rounding 
		errors. This will especially be noticable when the FPU is in single precision mode 
		(which is for example set on default by Direct3D).
		\param p Point to test. Assumes that this point is already
		on the plane of the triangle.
		\return True if point is inside the triangle, otherwise false. */
		bool isPointInsideFast(const FVec3<T>& p) const
		{
			const FVec3<T> a = pointC - pointA;
			const FVec3<T> b = pointB - pointA;
			const FVec3<T> c = p - pointA;
			
			const float64 dotAA = a.Dot( a);
			const float64 dotAB = a.Dot( b);
			const float64 dotAC = a.Dot( c);
			const float64 dotBB = b.Dot( b);
			const float64 dotBC = b.Dot( c);
			 
			// get coordinates in barycentric coordinate system
			const float64 invDenom =  1/(dotAA * dotBB - dotAB * dotAB); 
			const float64 u = (dotBB * dotAC - dotAB * dotBC) * invDenom;
			const float64 v = (dotAA * dotBC - dotAB * dotAC ) * invDenom;
		 
			// We count border-points as inside to keep downward compatibility.
			// That's why we use >= and <= instead of > and < as more commonly seen on the web.
			return (u >= 0) && (v >= 0) && (u + v <= 1);

		}


		//! Get an intersection with a 3d line.
		/** \param line Line to intersect with.
		\param outIntersection Place to store the intersection point, if there is one.
		\return True if there was an intersection, false if not. */
		bool getIntersectionWithLimitedLine(const line3d<T>& line,
			FVec3<T>& outIntersection) const
		{
			return getIntersectionWithLine(line.start,
				line.getVector(), outIntersection) &&
				outIntersection.isBetweenPoints(line.start, line.end);
		}


		//! Get an intersection with a 3d line.
		/** Please note that also points are returned as intersection which
		are on the line, but not between the start and end point of the line.
		If you want the returned point be between start and end
		use getIntersectionWithLimitedLine().
		\param linePoint Point of the line to intersect with.
		\param lineVect Vector of the line to intersect with.
		\param outIntersection Place to store the intersection point, if there is one.
		\return True if there was an intersection, false if there was not. */
		bool getIntersectionWithLine(const FVec3<T>& linePoint,
			const FVec3<T>& lineVect, FVec3<T>& outIntersection) const
		{
			if (getIntersectionOfPlaneWithLine(linePoint, lineVect, outIntersection))
				return isPointInside(outIntersection);

			return false;
		}

		bool getIntersectionWithLine( const FVec3<T>& linePoint, 
			const FVec3<T>& lineVect, float* t, float* u, float* v) const
		{
			// Find vectors for two edges sharing vert0
			FVec3<T> edge1 = pointB - pointA;
			FVec3<T> edge2 = pointC - pointA;

			// Begin calculating determinant - also used to calculate U parameter
			FVec3<T> pvec = lineVect.Cross(edge2);

			// If determinant is near zero, ray lies in plane of triangle
			float32 det = edge1.Dot(pvec);

			FVec3<T> tvec;
			if( det > 0 )
			{
				tvec = linePoint - pointA;
			}
			else
			{
				tvec = pointA - linePoint;
				det = -det;
			}

			if( det < 0.0001f )
				return false;

			// Calculate U parameter and test bounds
			*u = tvec.Dot(pvec);
			if( *u < 0.0f || *u > det )
				return false;

			// Prepare to test V parameter
			FVec3<T> qvec = tvec.Cross(edge1);

			// Calculate V parameter and test bounds
			*v = lineVect.Dot(qvec);
			if( *v < 0.0f || *u + *v > det )
				return false;

			// Calculate t, scale parameters, ray intersects triangle
			*t = edge2.Dot(qvec);
			float32 fInvDet = 1.0f / det;
			*t *= fInvDet;
			*u *= fInvDet;
			*v *= fInvDet;

			return true;
		}


		//! Calculates the intersection between a 3d line and the plane the triangle is on.
		/** \param lineVect Vector of the line to intersect with.
		\param linePoint Point of the line to intersect with.
		\param outIntersection Place to store the intersection point, if there is one.
		\return True if there was an intersection, else false. */
		bool getIntersectionOfPlaneWithLine(const FVec3<T>& linePoint,
			const FVec3<T>& lineVect, FVec3<T>& outIntersection) const
		{
			const FVec3<T> normal = getNormal().Normalize();
			T t2;

			if ( iszero ( t2 = normal.Dot(lineVect) ) )
				return false;

			T d = pointA.Dot(normal);
			T t = -(normal.Dot(linePoint) - d) / t2;
			outIntersection = linePoint + (lineVect * t);
			return true;
		}


		//! Get the normal of the triangle.
		/** Please note: The normal is not always normalized. */
		FVec3<T> getNormal() const
		{
			return (pointB - pointA).Cross(pointC - pointA);
		}

		//! Test if the triangle would be front or backfacing from any point.
		/** Thus, this method assumes a camera position from which the
		triangle is definitely visible when looking at the given direction.
		Do not use this method with points as it will give wrong results!
		\param lookDirection Look direction.
		\return True if the plane is front facing and false if it is backfacing. */
		bool isFrontFacing(const FVec3<T>& lookDirection) const
		{
			const FVec3<T> n = getNormal().Normalize();
			const float32 d = (float32)n.Dot(lookDirection);
			return F32_LOWER_EQUAL_0(d);
		}

		//! Get the plane of this triangle.
		plane3d<T> getPlane() const
		{
			return plane3d<T>(pointA, pointB, pointC);
		}

		//! Get the bounding box of this triangle
		FAABBox<T> getBoundingBox() const
		{
			FAABBox<T> Box(pointA);
			Box.addInternalPoint(pointB);
			Box.addInternalPoint(pointC);
			return Box;
		}

		//! Get the area of the triangle
		T getArea() const
		{
			return (pointB - pointA).Cross(pointC - pointA).getLength() * 0.5f;

		}

		T getArea1() const
		{
			float a = (pointB - pointA).getLength();
			float b = (pointC - pointA).getLength();
			float c = (pointB - pointC).getLength();

			float p = (a + b + c) * 0.5f;
			float s = sqrt(p * (p-a) * (p-b) * (p-c));

			return s;
		}

		//! sets the triangle's points
		void set(const FVec3<T>& a, const FVec3<T>& b, const FVec3<T>& c)
		{
			pointA = a;
			pointB = b;
			pointC = c;
		}

		//! the three points of the triangle
		FVec3<T> pointA;
		FVec3<T> pointB;
		FVec3<T> pointC;

	private:
		bool isOnSameSide(const FVec3<T>& p1, const FVec3<T>& p2,
			const FVec3<T>& a, const FVec3<T>& b) const
		{
			FVec3<T> bminusa = b - a;
			FVec3<T> cp1 = bminusa.Cross(p1 - a);
			FVec3<T> cp2 = bminusa.Cross(p2 - a);
			return (cp1.Dot(cp2) >= 0.0f);
		}
	};


	//! Typedef for a float32 3d triangle.
	typedef triangle3d<float32> triangle3df;

	//! Typedef for an integer 3d triangle.
	typedef triangle3d<int32> triangle3di;

} // end namespace ti

#endif

