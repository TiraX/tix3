//-*-c++-*-
// Copyright (C) 2002-2008 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h
#pragma once
#ifndef __IRR_QUATERNION_H_INCLUDED__
#define __IRR_QUATERNION_H_INCLUDED__

namespace tix
{
	//! Quaternion class for representing rotations.
	/** It provides cheap combinations and avoids gimbal locks.
		Also useful for interpolations. */
	class quaternion
	{
	public:

		//! Default Constructor
		quaternion() : X(0.0f), Y(0.0f), Z(0.0f), W(1.0f) {}

		//! Constructor
		quaternion(float32 x, float32 y, float32 z, float32 w) : X(x), Y(y), Z(z), W(w) { }

		//! Constructor which converts euler angles (radians) to a quaternion
		quaternion(float32 x, float32 y, float32 z);

		//! Constructor which converts euler angles (radians) to a quaternion
		quaternion(const FFloat3& vec);

		//! Constructor which converts a matrix to a quaternion
		quaternion(const matrix4& mat);

		//!
		float32& operator [] (uint32 i)
		{
			TI_ASSERT(i < 4);
			return getDataPtr()[i];
		}

		//!
		const float32& operator [] (uint32 i) const
		{
			TI_ASSERT(i < 4);
			return getDataPtr()[i];
		}

		//! Equalilty operator
		bool operator==(const quaternion& other) const;
		bool operator!=(const quaternion& other) const;

		//! Assignment operator
		inline quaternion& operator=(const quaternion& other);

		//! Matrix assignment operator
		inline quaternion& operator=(const matrix4& other);

		//! Add operator
		quaternion operator+(const quaternion& other) const;

		//! Multiplication operator
		quaternion operator*(const quaternion& other) const;

		//! Multiplication operator with scalar
		quaternion operator*(float32 s) const;

		//! Multiplication operator with scalar
		quaternion& operator*=(float32 s);

		//! Multiplication operator
		FFloat3 operator*(const FFloat3& v) const;

		//! Multiplication operator
		quaternion& operator*=(const quaternion& other);

		//! Calculates the dot product
		inline float32 Dot(const quaternion& other) const;

		//! Sets newly quaternion
		inline quaternion& set(float32 x, float32 y, float32 z, float32 w);

		//! Sets newly quaternion based on euler angles (radians)
		inline quaternion& set(float32 x, float32 y, float32 z);

		//! Sets newly quaternion based on euler angles (radians)
		inline quaternion& set(const FFloat3& vec);

		//! Normalizes the quaternion
		inline quaternion& normalize();

		//! Creates a matrix from this quaternion
		matrix4 getMatrix() const;
		matrix4 getMatrix_transposed() const;

		//! Creates a matrix from this quaternion
		void getMatrix(matrix4& dest) const;

		//! Creates a matrix from this quaternion
		void getMatrix_transposed(matrix4& dest) const;

		//! Inverts this quaternion
		quaternion& makeInverse();

		//! Set this quaternion to the result of the interpolation between two quaternions
		quaternion& slerp(quaternion q1, quaternion q2, float32 interpolate);

		//! Create quaternion from rotation angle and rotation axis.
		/** Axis must be unit length.
			The quaternion representing the rotation is
			q = cos(A/2)+sin(A/2)*(x*i+y*j+z*k).
			\param angle Rotation Angle in radians.
			\param axis Rotation axis. */
		quaternion& fromAngleAxis(float32 angle, const FFloat3& axis);

		//! Fills an angle (radians) around an axis (unit vector)
		void toAngleAxis(float32& angle, FFloat3& axis) const;

		//! Output this quaternion to an euler angle (radians)
		void toEuler(FFloat3& euler) const;

		//! Output this quaternion to an euler angle (360 degree)
		void toEulerDegree(FFloat3& euler) const;

		//! Set quaternion to identity
		quaternion& makeIdentity();

		//! Set quaternion to represent a rotation from one vector to another.
		quaternion& rotationFromTo(const FFloat3& from, const FFloat3& to);
		//! parameters are normalized, do not need normalize in function
		quaternion& rotationFromToFast(const FFloat3& from_normalized, const FFloat3& to_normalized);

		void setX(float32 val) { X = val; }
		void setY(float32 val) { Y = val; }
		void setZ(float32 val) { Z = val; }
		void setW(float32 val) { W = val; }

		//!
		const float32* getDataPtr() const
		{
			return reinterpret_cast<const float32*>(this);
		}

		//!
		float32* getDataPtr()
		{
			return reinterpret_cast<float32*>(this);
		}

		//! Quaternion elements.
		float32 X, Y, Z, W;
	};


	// Constructor which converts euler angles to a quaternion
	inline quaternion::quaternion(float32 x, float32 y, float32 z)
	{
		set(x, y, z);
	}


	// Constructor which converts euler angles to a quaternion
	inline quaternion::quaternion(const FFloat3& vec)
	{
		set(vec.X, vec.Y, vec.Z);
	}


	// Constructor which converts a matrix to a quaternion
	inline quaternion::quaternion(const matrix4& mat)
	{
		(*this) = mat;
	}


	// equal operator
	inline bool quaternion::operator==(const quaternion& other) const
	{
		return ((X == other.X) &&
			(Y == other.Y) &&
			(Z == other.Z) &&
			(W == other.W));
	}
	inline bool quaternion::operator!=(const quaternion& other) const
	{
		return ((X != other.X) ||
			(Y != other.Y) ||
			(Z != other.Z) ||
			(W != other.W));
	}


	// assignment operator
	inline quaternion& quaternion::operator=(const quaternion& other)
	{
		X = other.X;
		Y = other.Y;
		Z = other.Z;
		W = other.W;
		return *this;
	}


	// matrix assignment operator
	inline quaternion& quaternion::operator=(const matrix4& m)
	{
		// Determine which of w, x, y, or z has the largest absolute value
		float fourWSquaredMinus1 = m(0, 0) + m(1, 1) + m(2, 2);
		float fourXSquaredMinus1 = m(0, 0) - m(1, 1) - m(2, 2);
		float fourYSquaredMinus1 = m(1, 1) - m(0, 0) - m(2, 2);
		float fourZSquaredMinus1 = m(2, 2) - m(0, 0) - m(1, 1);

		int biggestIndex = 0;
		float fourBiggestSquaredMinus1 = fourWSquaredMinus1;
		if (fourXSquaredMinus1 > fourBiggestSquaredMinus1)
		{
			fourBiggestSquaredMinus1 = fourXSquaredMinus1;
			biggestIndex = 1;
		}
		if (fourYSquaredMinus1 > fourBiggestSquaredMinus1)
		{
			fourBiggestSquaredMinus1 = fourYSquaredMinus1;
			biggestIndex = 2;
		}
		if (fourZSquaredMinus1 > fourBiggestSquaredMinus1)
		{
			fourBiggestSquaredMinus1 = fourZSquaredMinus1;
			biggestIndex = 3;
		}

		// Perform square root and division
		float biggestVal = sqrt(fourBiggestSquaredMinus1 + 1.0f) * 0.5f;
		float mult = 0.25f / biggestVal;
		// Apply table to compute quaternion values
		switch (biggestIndex)
		{
		case 0:
			W = biggestVal;
			X = (m(1, 2) - m(2, 1)) * mult;
			Y = (m(2, 0) - m(0, 2)) * mult;
			Z = (m(0, 1) - m(1, 0)) * mult;
			break;
		case 1:
			X = biggestVal;
			W = (m(1, 2) - m(2, 1)) * mult;
			Y = (m(0, 1) + m(1, 0)) * mult;
			Z = (m(2, 0) + m(0, 2)) * mult;
			break;
		case 2:
			Y = biggestVal;
			W = (m(2, 0) - m(0, 2)) * mult;
			X = (m(0, 1) + m(1, 0)) * mult;
			Z = (m(1, 2) + m(2, 1)) * mult;
			break;
		case 3:
			Z = biggestVal;
			W = (m(0, 1) - m(1, 0)) * mult;
			X = (m(2, 0) + m(0, 2)) * mult;
			Y = (m(1, 2) + m(2, 1)) * mult;
			break;
		}


		//	const float32 diag = m(0,0) + m(1,1) + m(2,2);

		//	if( diag > 0.0f )
		//	{
		//		float32 scale = sqrtf(1.0f + diag); // get scale from diagonal

		//		// TODO: speed this up
		//		W = 0.5f * scale;
		//		scale = 0.5f / scale;
		//		X = ( m(2,1) - m(1,2)) * scale;
		//		Y = ( m(0,2) - m(2,0)) * scale;
		//		Z = ( m(1,0) - m(0,1)) * scale;
		//		
		//	}
		//	else if ( m(0,0) > m(1,1) && m(0,0) > m(2,2))
		//	{
		//		// 1st element of diag is greatest value
		//		// find scale according to 1st element
		//		float32 scale = sqrtf( 1.0f + m(0,0) - m(1,1) - m(2,2));

		//		X = 0.5f * scale;
		//		scale = 0.5f / scale;
		//		Y = (m(0,1) + m(1,0)) * scale;
		//		Z = (m(2,0) + m(0,2)) * scale;
		//		W = (m(2,1) - m(1,2)) * scale;
		//	}
		//	else if ( m(1,1) > m(2,2))
		//	{
		//		// 2nd element of diag is greatest value
		//		// find scale according to 2nd element
		//		float32 scale = sqrtf( 1.0f + m(1,1) - m(0,0) - m(2,2));

		//		// TODO: speed this up
		//		Y = 0.5f * scale;
		//		scale = 0.5f / scale;
		//		X = (m(0,1) + m(1,0) ) * scale;
		//		Z = (m(1,2) + m(2,1) ) * scale;
		//		W = (m(0,2) - m(2,0) ) * scale;
		//	}
		//	else
		//	{
		//		// 3rd element of diag is greatest value
		//		// find scale according to 3rd element
		//		float32 scale = sqrtf( 1.0f + m(2,2) - m(0,0) - m(1,1));

		//		// TODO: speed this up
		//		Z = 0.5f * scale;
		//		scale = 0.5f / scale;
		//		X = (m(0,2) + m(2,0)) * scale;
		//		Y = (m(1,2) + m(2,1)) * scale;
		//		W = (m(1,0) - m(0,1)) * scale;
		//	}
		//#if !(TI_USE_RH)
		//	makeInverse();
		//#endif

		return normalize();
	}


	// multiplication operator
	inline quaternion quaternion::operator*(const quaternion& other) const
	{
		quaternion tmp;
#if TI_USE_RH
		tmp.W = (other.W * W) - (other.X * X) - (other.Y * Y) - (other.Z * Z);
		tmp.X = (other.W * X) + (other.X * W) + (other.Y * Z) - (other.Z * Y);
		tmp.Y = (other.W * Y) + (other.Y * W) + (other.Z * X) - (other.X * Z);
		tmp.Z = (other.W * Z) + (other.Z * W) + (other.X * Y) - (other.Y * X);
#else
		tmp.W = (W * other.W) - (X * other.X) - (Y * other.Y) - (Z * other.Z);
		tmp.X = (W * other.X) + (X * other.W) + (Y * other.Z) - (Z * other.Y);
		tmp.Y = (W * other.Y) + (Y * other.W) + (Z * other.X) - (X * other.Z);
		tmp.Z = (W * other.Z) + (Z * other.W) + (X * other.Y) - (Y * other.X);
#endif
		return tmp;
	}


	// multiplication operator
	inline quaternion quaternion::operator*(float32 s) const
	{
		return quaternion(s * X, s * Y, s * Z, s * W);
	}

	// multiplication operator
	inline quaternion& quaternion::operator*=(float32 s)
	{
		X *= s;
		Y *= s;
		Z *= s;
		W *= s;
		return *this;
	}

	// multiplication operator
	inline quaternion& quaternion::operator*=(const quaternion& other)
	{
		return (*this = other * (*this));
	}

	// add operator
	inline quaternion quaternion::operator+(const quaternion& b) const
	{
		return quaternion(X + b.X, Y + b.Y, Z + b.Z, W + b.W);
	}


	// Creates a matrix from this quaternion
	inline matrix4 quaternion::getMatrix() const
	{
		matrix4 m(matrix4::EM4CONST_NOTHING);
#if TI_USE_RH
		getMatrix_transposed(m);
#else
		getMatrix(m);
#endif
		return m;
	}

	inline matrix4 quaternion::getMatrix_transposed() const
	{
		matrix4 m(matrix4::EM4CONST_NOTHING);
		getMatrix_transposed(m);
		return m;
	}

	// Creates a matrix from this quaternion
	inline void quaternion::getMatrix(matrix4& dest) const
	{
		const float32 _2xx = 2.0f * X * X;
		const float32 _2yy = 2.0f * Y * Y;
		const float32 _2zz = 2.0f * Z * Z;
		const float32 _2xy = 2.0f * X * Y;
		const float32 _2xz = 2.0f * X * Z;
		const float32 _2xw = 2.0f * X * W;
		const float32 _2yz = 2.0f * Y * Z;
		const float32 _2yw = 2.0f * Y * W;
		const float32 _2zw = 2.0f * Z * W;

		dest[0] = 1.0f - _2yy - _2zz;
		dest[1] = _2xy + _2zw;
		dest[2] = _2xz - _2yw;
		dest[3] = 0.0f;

		dest[4] = _2xy - _2zw;
		dest[5] = 1.0f - _2xx - _2zz;
		dest[6] = _2yz + _2xw;
		dest[7] = 0.0f;

		dest[8] = _2xz + _2yw;
		dest[9] = _2yz - _2xw;
		dest[10] = 1.0f - _2yy - _2xx;
		dest[11] = 0.0f;

		dest[12] = 0.f;
		dest[13] = 0.f;
		dest[14] = 0.f;
		dest[15] = 1.f;
	}

	// Creates a matrix from this quaternion
	inline void quaternion::getMatrix_transposed(matrix4& dest) const
	{
		const float32 _2xx = 2.0f * X * X;
		const float32 _2yy = 2.0f * Y * Y;
		const float32 _2zz = 2.0f * Z * Z;
		const float32 _2xy = 2.0f * X * Y;
		const float32 _2xz = 2.0f * X * Z;
		const float32 _2xw = 2.0f * X * W;
		const float32 _2yz = 2.0f * Y * Z;
		const float32 _2yw = 2.0f * Y * W;
		const float32 _2zw = 2.0f * Z * W;

		dest[0] = 1.0f - _2yy - _2zz;
		dest[4] = _2xy + _2zw;
		dest[8] = _2xz - _2yw;
		dest[12] = 0.0f;

		dest[1] = _2xy - _2zw;
		dest[5] = 1.0f - _2xx - _2zz;
		dest[9] = _2yz + _2xw;
		dest[13] = 0.0f;

		dest[2] = _2xz + _2yw;
		dest[6] = _2yz - _2xw;
		dest[10] = 1.0f - _2yy - _2xx;
		dest[14] = 0.0f;

		dest[3] = 0.f;
		dest[7] = 0.f;
		dest[11] = 0.f;
		dest[15] = 1.f;
	}



	// Inverts this quaternion
	inline quaternion& quaternion::makeInverse()
	{
		X = -X; Y = -Y; Z = -Z;
		return *this;
	}

	// sets newly quaternion
	inline quaternion& quaternion::set(float32 x, float32 y, float32 z, float32 w)
	{
		X = x;
		Y = y;
		Z = z;
		W = w;
		return *this;
	}


	// sets newly quaternion based on euler angles
	inline quaternion& quaternion::set(float32 x, float32 y, float32 z)
	{
		float64 angle;

		angle = x * 0.5;
		const float64 sr = sin(angle);
		const float64 cr = cos(angle);

		angle = y * 0.5;
		const float64 sp = sin(angle);
		const float64 cp = cos(angle);

		angle = z * 0.5;
		const float64 sy = sin(angle);
		const float64 cy = cos(angle);

		const float64 cpcy = cp * cy;
		const float64 spcy = sp * cy;
		const float64 cpsy = cp * sy;
		const float64 spsy = sp * sy;

		X = (float32)(sr * cpcy - cr * spsy);
		Y = (float32)(cr * spcy + sr * cpsy);
		Z = (float32)(cr * cpsy - sr * spcy);
		W = (float32)(cr * cpcy + sr * spsy);

		return normalize();
	}

	// sets newly quaternion based on euler angles
	inline quaternion& quaternion::set(const FFloat3& vec)
	{
		return set(vec.X, vec.Y, vec.Z);
	}

	// normalizes the quaternion
	inline quaternion& quaternion::normalize()
	{
		const float32 n = X * X + Y * Y + Z * Z + W * W;

		if (n == 1)
			return *this;

		//n = 1.0f / sqrtf(n);
		return (*this *= TMath::ReciprocalSquareroot(n));
	}


	// set this quaternion to the result of the interpolation between two quaternions
	inline quaternion& quaternion::slerp(quaternion q1, quaternion q2, float32 time)
	{
		float32 angle = q1.Dot(q2);

		if (angle < 0.0f)
		{
			q1 *= -1.0f;
			angle *= -1.0f;
		}

		float32 scale;
		float32 invscale;

		if ((angle + 1.0f) > 0.05f)
		{
			if ((1.0f - angle) >= 0.05f) // spherical interpolation
			{
				const float32 theta = acosf(angle);
				const float32 invsintheta = 1.0f / (sinf(theta));
				scale = sinf(theta * (1.0f - time)) * invsintheta;
				invscale = sinf(theta * time) * invsintheta;
				*this = (q1 * scale) + (q2 * invscale);
			}
			else // linear interploation
			{
				scale = 1.0f - time;
				invscale = time;
				*this = (q1 * scale) + (q2 * invscale);
				normalize();
			}
		}
		else
		{
			q2.set(-q1.Y, q1.X, -q1.W, q1.Z);
			scale = sinf(PI * (0.5f - time));
			invscale = sinf(PI * time);
			*this = (q1 * scale) + (q2 * invscale);
		}

		return *this;
	}


	// calculates the dot product
	inline float32 quaternion::Dot(const quaternion& q2) const
	{
		return (X * q2.X) + (Y * q2.Y) + (Z * q2.Z) + (W * q2.W);
	}


	//! axis must be unit length
	//! angle in radians
	inline quaternion& quaternion::fromAngleAxis(float32 angle, const FFloat3& axis)
	{
		const float32 fHalfAngle = 0.5f * angle;
		const float32 fSin = sinf(fHalfAngle);
		W = cosf(fHalfAngle);
		X = fSin * axis.X;
		Y = fSin * axis.Y;
		Z = fSin * axis.Z;
		return *this;
	}


	inline void quaternion::toAngleAxis(float32& angle, FFloat3& axis) const
	{
		const float32 scale = sqrtf(X * X + Y * Y + Z * Z);

		if (TMath::IsZero(scale) || W > 1.0f || W < -1.0f)
		{
			angle = 0.0f;
			axis.X = 0.0f;
			axis.Y = 1.0f;
			axis.Z = 0.0f;
		}
		else
		{
			const float32 invscale = 1.0f / (scale);
			angle = 2.0f * acosf(W);
			axis.X = X * invscale;
			axis.Y = Y * invscale;
			axis.Z = Z * invscale;
		}
	}

	inline void quaternion::toEuler(FFloat3& euler) const
	{
		toEulerDegree(euler);
		euler *= DEGTORAD;
	}

	inline void quaternion::toEulerDegree(FFloat3& euler) const
	{
		matrix4 m;
		m.makeIdentity();
		getMatrix(m);
		euler = m.getRotationDegrees();
	}

	inline FFloat3 quaternion::operator* (const FFloat3& v) const
	{
		// nVidia SDK implementation

		FFloat3 uv, uuv;
		FFloat3 qvec(X, Y, Z);
		uv = qvec.Cross(v);
		uuv = qvec.Cross(uv);
		uv *= (2.0f * W);
		uuv *= 2.0f;

		return v + uv + uuv;
	}

	// set quaternion to identity
	inline quaternion& quaternion::makeIdentity()
	{
		W = 1.f;
		X = 0.f;
		Y = 0.f;
		Z = 0.f;
		return *this;
	}

	inline quaternion& quaternion::rotationFromTo(const FFloat3& from, const FFloat3& to)
	{
		// Based on Stan Melax's article in Game Programming Gems
		// Copy, since cannot modify local
		FFloat3 v0 = from;
		FFloat3 v1 = to;
		v0.Normalize();
		v1.Normalize();

		const float32 d = v0.Dot(v1);
		if (d >= 1.0f) // If dot == 1, vectors are the same
		{
			return makeIdentity();
		}

		if (d <= -1.0f) // if dot == -1, vectors are opossite
		{
			FFloat3 axis(1, 0, 0);
			axis = axis.Cross(from);
			if (axis.GetLengthSQ() == 0) // pick another if colinear
			{
				axis = FFloat3(0, 1, 0);
				axis = axis.Cross(from);
			}
			axis.Normalize();
			return fromAngleAxis(PI, axis);
		}

		const float32 s = sqrtf((1 + d) * 2); // optimize inv_sqrt
		const float32 invs = 1.f / s;
		const FFloat3 c = // v0.Cross(v1)*invs;
			FFloat3(v0.Y * v1.Z - v0.Z * v1.Y, v0.Z * v1.X - v0.X * v1.Z, v0.X * v1.Y - v0.Y * v1.X) * invs;


		X = c.X;
		Y = c.Y;
		Z = c.Z;
		W = s * 0.5f;

		return *this;
	}

	inline quaternion& quaternion::rotationFromToFast(const FFloat3& from_normalized, const FFloat3& to_normalized)
	{
		// Based on Stan Melax's article in Game Programming Gems
		// Copy, since cannot modify local
		FFloat3 v0 = from_normalized;
		FFloat3 v1 = to_normalized;

		const float32 d = v0.Dot(v1);
		if (d >= 0.99f) // If dot == 1, vectors are the same
		{
			return makeIdentity();
		}

		if (d <= -0.99f) // if dot == -1, vectors are opossite
		{
			FFloat3 axis(1, 0, 0);
			axis = axis.Cross(from_normalized);
			if (axis.GetLengthSQ() == 0) // pick another if colinear
			{
				axis = FFloat3(0, 1, 0);
				axis = axis.Cross(from_normalized);
			}
			axis.Normalize();
			return fromAngleAxis(PI, axis);
		}

		const float32 s = sqrtf((1 + d) * 2); // optimize inv_sqrt
		const float32 invs = 1.f / s;
		const FFloat3 c = // v0.Cross(v1)*invs;
			FFloat3(v0.Y * v1.Z - v0.Z * v1.Y, v0.Z * v1.X - v0.X * v1.Z, v0.X * v1.Y - v0.Y * v1.X) * invs;


		X = c.X;
		Y = c.Y;
		Z = c.Z;
		W = s * 0.5f;

		return *this;
	}

} // end namespace ti

#endif

