//-*-c++-*-
// Copyright (C) 2002-2008 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h
#pragma once
#ifndef __IRR_LINE_3D_H_INCLUDED__
#define __IRR_LINE_3D_H_INCLUDED__

namespace tix
{

//! 3D line between two points with intersection methods.
template <class T>
class line3d
{
public:

	//! Default constructor
	/** line from (0,0,0) to (1,1,1) */
	line3d() : start(0,0,0), end(1,1,1) {}
	//! Constructor with two points
	line3d(T xa, T ya, T za, T xb, T yb, T zb) : start(xa, ya, za), end(xb, yb, zb) {}
	//! Constructor with two points as vectors
	line3d(const FVec3<T>& start, const FVec3<T>& end) : start(start), end(end) {}

	// operators

	line3d<T> operator+(const FVec3<T>& point) const { return line3d<T>(start + point, end + point); }
	line3d<T>& operator+=(const FVec3<T>& point) { start += point; end += point; return *this; }

	line3d<T> operator-(const FVec3<T>& point) const { return line3d<T>(start - point, end - point); }
	line3d<T>& operator-=(const FVec3<T>& point) { start -= point; end -= point; return *this; }

	bool operator==(const line3d<T>& other) const
	{ return (start==other.start && end==other.end) || (end==other.start && start==other.end);}
	bool operator!=(const line3d<T>& other) const
	{ return !(start==other.start && end==other.end) || (end==other.start && start==other.end);}

	// functions
	//! Set this line to a newly line going through the two points.
	void setLine(const T& xa, const T& ya, const T& za, const T& xb, const T& yb, const T& zb)
	{start.set(xa, ya, za); end.set(xb, yb, zb);}
	//! Set this line to a newly line going through the two points.
	void setLine(const FVec3<T>& nstart, const FVec3<T>& nend)
	{start.set(nstart); end.set(nend);}
	//! Set this line to newly line given as parameter.
	void setLine(const line3d<T>& line)
	{start.set(line.start); end.set(line.end);}

	//! Get length of line
	/** \return Length of line. */
	T getLength() const { return start.getDistanceFrom(end); }

	//! Get squared length of line
	/** \return Squared length of line. */
	T GetLengthSQ() const { return start.getDistanceFromSQ(end); }

	//! Get middle of line
	/** \return Center of line. */
	FVec3<T> getMiddle() const
	{
		return (start + end) * (T)0.5;
	}

	//! Get vector of line
	/** \return vector of line. */
	FVec3<T> getVector() const
	{
		return end - start;
	}

	//! Check if the given point is between start and end of the line.
	/** Assumes that the point is already somewhere on the line.
		\param point The point to test.
		\return True if point is on the line between start and end, else false.
	*/
	bool isPointBetweenStartAndEnd(const FVec3<T>& point) const
	{
		return point.isBetweenPoints(start, end);
	}

	//! Get the closest point on this line to a point
	/** \param point The point to compare to.
		\return The nearest point which is part of the line. */
	FVec3<T> getClosestPoint(const FVec3<T>& point) const
	{
		FVec3<T> c = point - start;
		FVec3<T> v = end - start;
		T d = (T)v.getLength();
		v /= d;
		T t = v.Dot(c);

		if (t < (T)0.0)
			return start;
		if (t > d)
			return end;

		v *= t;
		return start + v;
	}

	//! Check if the line intersects with a shpere
	/** \param sorigin Origin of the shpere.
		\param sradius Radius of the sphere.
		\param outdistance The distance to the first intersection point.
		\return True if there is an intersection.
		If there is one, the distance to the first intersection point
		is stored in outdistance. */
	bool getIntersectionWithSphere(FVec3<T> sorigin, T sradius, float32& outdistance) const
	{
		const FVec3<T> q = sorigin - start;
		T c = q.getLength();
		T v = q.Dot(getVector().Normalize());
		T d = sradius * sradius - (c*c - v*v);

		if (d < 0.0)
			return false;

		outdistance = v - sqrt((float32)d);
		return true;
	}

	// member variables

	//! Start point of line
	FVec3<T> start;
	//! End point of line
	FVec3<T> end;
};

//! Typedef for an float32 line.
typedef line3d<float32> line3df;
//! Typedef for an integer line.
typedef line3d<int32> line3di;

} // end namespace ti

#endif

