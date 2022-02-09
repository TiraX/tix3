/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	class FQuat
	{
	public:
		FQuat() 
			: X(0.0f)
			, Y(0.0f)
			, Z(0.0f)
			, W(1.0f) 
		{}

		FQuat(float32 x, float32 y, float32 z, float32 w) 
			: X(x)
			, Y(y)
			, Z(z)
			, W(w) 
		{}

		// Constructor which converts euler angles to a FQuat
		FQuat(float32 x, float32 y, float32 z);
		FQuat(const FFloat3& vec);

		// Constructor which converts a matrix to a FQuat
		FQuat(const FMat4& mat);

		float32& operator [] (uint32 i)
		{
			TI_ASSERT(i < 4);
			return Data()[i];
		}

		const float32& operator [] (uint32 i) const
		{
			TI_ASSERT(i < 4);
			return Data()[i];
		}

		bool operator==(const FQuat& other) const;
		bool operator!=(const FQuat& other) const;
		bool operator<(const FQuat& other) const;

		inline FQuat& operator=(const FQuat& other);
		inline FQuat& operator=(const FMat4& other);
		FQuat operator+(const FQuat& other) const;
		FQuat operator*(const FQuat& other) const;
		FQuat operator*(float32 s) const;
		FQuat& operator*=(float32 s);
		FFloat3 operator*(const FFloat3& v) const;
		FQuat& operator*=(const FQuat& other);

		inline float32 Dot(const FQuat& other) const;

		inline FQuat& Set(float32 x, float32 y, float32 z, float32 w);
		inline FQuat& Set(float32 x, float32 y, float32 z);

		//! Sets newly FQuat based on euler angles (radians)
		inline FQuat& Set(const FFloat3& vec);

		//! Normalizes the FQuat
		inline FQuat& Normalize();

		FMat4 GetMatrix() const;
		FMat4 GetMatrixTransposed() const;

		void GetMatrix(FMat4& dest) const;
		void GetMatrixTransposed(FMat4& dest) const;

		FQuat& MakeInverse();

		FQuat& Slerp(FQuat q1, FQuat q2, float32 interpolate);

		//! Create FQuat from rotation angle and rotation axis.
		/** Axis must be unit length.
			The FQuat representing the rotation is
			q = cos(A/2)+sin(A/2)*(x*i+y*j+z*k).
			\param angle Rotation Angle in radians.
			\param axis Rotation axis. */
		FQuat& FromAngleAxis(float32 angle, const FFloat3& axis);

		void ToAngleAxis(float32& angle, FFloat3& axis) const;
		void ToEuler(FFloat3& euler) const;

		FQuat& MakeIdentity();

		FQuat& RotationFromTo(const FFloat3& from, const FFloat3& to);
		FQuat& RotationFromToFast(const FFloat3& from_normalized, const FFloat3& to_normalized);

		//!
		const float32* Data() const
		{
			return reinterpret_cast<const float32*>(this);
		}

		//!
		float32* Data()
		{
			return reinterpret_cast<float32*>(this);
		}

		//! Quaternion elements.
		float32 X, Y, Z, W;
	};


	// Constructor which converts euler angles to a FQuat
	inline FQuat::FQuat(float32 x, float32 y, float32 z)
	{
		Set(x, y, z);
	}
	// Constructor which converts euler angles to a FQuat
	inline FQuat::FQuat(const FFloat3& vec)
	{
		Set(vec.X, vec.Y, vec.Z);
	}

	// Constructor which converts a matrix to a FQuat
	inline FQuat::FQuat(const FMat4& mat)
	{
		(*this) = mat;
	}

	inline bool FQuat::operator==(const FQuat& other) const
	{
		return ((X == other.X) &&
			(Y == other.Y) &&
			(Z == other.Z) &&
			(W == other.W));
	}
	inline bool FQuat::operator!=(const FQuat& other) const
	{
		return ((X != other.X) ||
			(Y != other.Y) ||
			(Z != other.Z) ||
			(W != other.W));
	}

	// overload operator< for std::map compare
	inline bool FQuat::operator<(const FQuat& other) const
	{
		if (X != other.X)
			return X < other.X;
		if (Y != other.Y)
			return Y < other.Y;
		if (Z != other.Z)
			return Z < other.Z;
		return W < other.W;
	}

	inline FQuat& FQuat::operator=(const FQuat& other)
	{
		X = other.X;
		Y = other.Y;
		Z = other.Z;
		W = other.W;
		return *this;
	}

	inline FQuat& FQuat::operator=(const FMat4& m)
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
		// Apply table to compute FQuat values
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

		return Normalize();
	}

	inline FQuat FQuat::operator*(const FQuat& other) const
	{
		FQuat tmp;
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

	inline FQuat FQuat::operator*(float32 s) const
	{
		return FQuat(s * X, s * Y, s * Z, s * W);
	}

	inline FQuat& FQuat::operator*=(float32 s)
	{
		X *= s;
		Y *= s;
		Z *= s;
		W *= s;
		return *this;
	}

	inline FQuat& FQuat::operator*=(const FQuat& other)
	{
		return (*this = other * (*this));
	}

	inline FQuat FQuat::operator+(const FQuat& b) const
	{
		return FQuat(X + b.X, Y + b.Y, Z + b.Z, W + b.W);
	}

	inline FMat4 FQuat::GetMatrix() const
	{
		FMat4 m(FMat4::EM4CONST_NOTHING);
#if TI_USE_RH
		GetMatrixTransposed(m);
#else
		GetMatrix(m);
#endif
		return m;
	}

	inline FMat4 FQuat::GetMatrixTransposed() const
	{
		FMat4 m(FMat4::EM4CONST_NOTHING);
		GetMatrixTransposed(m);
		return m;
	}

	inline void FQuat::GetMatrix(FMat4& dest) const
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

	inline void FQuat::GetMatrixTransposed(FMat4& dest) const
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

	inline FQuat& FQuat::MakeInverse()
	{
		X = -X; 
		Y = -Y;
		Z = -Z;
		return *this;
	}

	inline FQuat& FQuat::Set(float32 x, float32 y, float32 z, float32 w)
	{
		X = x;
		Y = y;
		Z = z;
		W = w;
		return *this;
	}

	inline FQuat& FQuat::Set(float32 x, float32 y, float32 z)
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

		return Normalize();
	}

	inline FQuat& FQuat::Set(const FFloat3& vec)
	{
		return Set(vec.X, vec.Y, vec.Z);
	}

	inline FQuat& FQuat::Normalize()
	{
		const float32 n = X * X + Y * Y + Z * Z + W * W;

		if (n == 1)
			return *this;

		//n = 1.0f / sqrtf(n);
		return (*this *= TMath::ReciprocalSquareroot(n));
	}

	inline FQuat& FQuat::Slerp(FQuat q1, FQuat q2, float32 time)
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
				Normalize();
			}
		}
		else
		{
			q2.Set(-q1.Y, q1.X, -q1.W, q1.Z);
			scale = sinf(PI * (0.5f - time));
			invscale = sinf(PI * time);
			*this = (q1 * scale) + (q2 * invscale);
		}

		return *this;
	}

	inline float32 FQuat::Dot(const FQuat& q2) const
	{
		return (X * q2.X) + (Y * q2.Y) + (Z * q2.Z) + (W * q2.W);
	}

	//! axis must be unit length
	//! angle in radians
	inline FQuat& FQuat::FromAngleAxis(float32 angle, const FFloat3& axis)
	{
		const float32 fHalfAngle = 0.5f * angle;
		const float32 fSin = sinf(fHalfAngle);
		W = cosf(fHalfAngle);
		X = fSin * axis.X;
		Y = fSin * axis.Y;
		Z = fSin * axis.Z;
		return *this;
	}


	inline void FQuat::ToAngleAxis(float32& angle, FFloat3& axis) const
	{
		const float32 scale = TMath::Sqrt(X * X + Y * Y + Z * Z);

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

	inline void FQuat::ToEuler(FFloat3& euler) const
	{
		FMat4 m;
		m.MakeIdentity();
		GetMatrix(m);
		euler = m.GetRotationRadians();
	}

	inline FFloat3 FQuat::operator* (const FFloat3& v) const
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

	// set FQuat to identity
	inline FQuat& FQuat::MakeIdentity()
	{
		W = 1.f;
		X = 0.f;
		Y = 0.f;
		Z = 0.f;
		return *this;
	}

	inline FQuat& FQuat::RotationFromTo(const FFloat3& from, const FFloat3& to)
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
			return MakeIdentity();
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
			return FromAngleAxis(PI, axis);
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

	inline FQuat& FQuat::RotationFromToFast(const FFloat3& from_normalized, const FFloat3& to_normalized)
	{
		// Based on Stan Melax's article in Game Programming Gems
		// Copy, since cannot modify local
		FFloat3 v0 = from_normalized;
		FFloat3 v1 = to_normalized;

		const float32 d = v0.Dot(v1);
		if (d >= 0.99f) // If dot == 1, vectors are the same
		{
			return MakeIdentity();
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
			return FromAngleAxis(PI, axis);
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

}

