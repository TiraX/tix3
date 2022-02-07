/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	template < typename T >
	class FVec3
	{
	public:
		FVec3()
			: X(0)
			, Y(0)
			, Z(0)
		{}

		FVec3(T nx, T ny, T nz)
			: X(nx)
			, Y(ny)
			, Z(nz)
		{}

		explicit FVec3(T n)
			: X(n)
			, Y(n)
			, Z(n)
		{}

		FVec3(const FVec3<T>& other)
			: X(other.X)
			, Y(other.Y)
			, Z(other.Z)
		{}

		T& operator [] (uint32 i)
		{
			TI_ASSERT(i < 3);
			return Data()[i];
		}

		const T& operator [] (uint32 i) const
		{
			TI_ASSERT(i < 3);
			return Data()[i];
		}

		FVec3<T> operator-() const
		{
			return FVec3<T>(-X, -Y, -Z);
		}

		FVec3<T>& operator=(const FVec3<T>& other)
		{
			X = other.X;
			Y = other.Y;
			Z = other.Z;
			return *this;
		}

		FVec3<T> operator+(const FVec3<T>& other) const
		{
			return FVec3<T>(X + other.X, Y + other.Y, Z + other.Z);
		}

		FVec3<T>& operator+=(const FVec3<T>& other)
		{
			X += other.X;
			Y += other.Y;
			Z += other.Z;
			return *this;
		}

		FVec3<T> operator+(const T val) const
		{
			return FVec3<T>(X + val, Y + val, Z + val);
		}

		FVec3<T>& operator+=(const T val)
		{
			X += val;
			Y += val;
			Z += val;
			return *this;
		}

		FVec3<T> operator-(const FVec3<T>& other) const
		{
			return FVec3<T>(X - other.X, Y - other.Y, Z - other.Z);
		}

		FVec3<T>& operator-=(const FVec3<T>& other)
		{
			X -= other.X;
			Y -= other.Y;
			Z -= other.Z;
			return *this;
		}

		FVec3<T> operator-(const T val) const
		{
			return FVec3<T>(X - val, Y - val, Z - val);
		}

		FVec3<T>& operator-=(const T val)
		{
			X -= val;
			Y -= val;
			Z -= val;
			return *this;
		}

		FVec3<T> operator*(const FVec3<T>& other) const
		{
			return FVec3<T>(X * other.X, Y * other.Y, Z * other.Z);
		}

		FVec3<T>& operator*=(const FVec3<T>& other)
		{
			X *= other.X;
			Y *= other.Y;
			Z *= other.Z;
			return *this;
		}

		FVec3<T> operator*(const T v) const
		{
			return FVec3<T>(X * v, Y * v, Z * v);
		}

		FVec3<T>& operator*=(const T v)
		{
			X *= v;
			Y *= v;
			Z *= v;
			return *this;
		}

		FVec3<T> operator/(const FVec3<T>& other) const
		{
			return FVec3<T>(X / other.X, Y / other.Y, Z / other.Z);
		}

		FVec3<T>& operator/=(const FVec3<T>& other)
		{
			X /= other.X;
			Y /= other.Y;
			Z /= other.Z;
			return *this;
		}

		FVec3<T> operator/(const T v) const
		{
			T i = (T)1.0 / v;
			return FVec3<T>(X * i, Y * i, Z * i);
		}

		FVec3<T>& operator/=(const T v)
		{
			T i = (T)1.0 / v;
			X *= i;
			Y *= i;
			Z *= i;
			return *this;
		}

		bool operator<=(const FVec3<T>& other) const
		{
			return X <= other.X && Y <= other.Y && Z <= other.Z;
		}

		bool operator>=(const FVec3<T>& other) const
		{
			return X >= other.X && Y >= other.Y && Z >= other.Z;
		}

		bool operator<(const FVec3<T>& other) const
		{
			return X < other.X&& Y < other.Y&& Z < other.Z;
		}

		bool operator>(const FVec3<T>& other) const
		{
			return X > other.X && Y > other.Y && Z > other.Z;
		}

		bool operator==(const FVec3<T>& other) const
		{
			return X == other.X && Y == other.Y && Z == other.Z;
		}

		bool operator!=(const FVec3<T>& other) const
		{
			return X != other.X || Y != other.Y || Z != other.Z;
		}

		bool Equals(const FVec3<T>& other, const T tolerance = (T)ROUNDING_ERROR_32) const
		{
			return TMath::Equals(X, other.X, tolerance) &&
				TMath::Equals(Y, other.Y, tolerance) &&
				TMath::Equals(Z, other.Z, tolerance);
		}

		T GetLength() const
		{
			return (T)TMath::Sqrt((X * X + Y * Y + Z * Z));
		}

		T GetLengthSQ() const
		{
			return X * X + Y * Y + Z * Z;
		}

		T Dot(const FVec3<T>& other) const
		{
			return X * other.X + Y * other.Y + Z * other.Z;
		}

		T GetDistanceFrom(const FVec3<T>& other) const
		{
			return FVec3<T>(X - other.X, Y - other.Y, Z - other.Z).GetLength();
		}

		T GetDistanceFromSQ(const FVec3<T>& other) const
		{
			return FVec3<T>(X - other.X, Y - other.Y, Z - other.Z).GetLengthSQ();
		}

		FVec3<T> Cross(const FVec3<T>& p) const
		{
#if TI_USE_RH
			return FVec3<T>(-Y * p.Z + Z * p.Y, -Z * p.X + X * p.Z, -X * p.Y + Y * p.X);
#else
			return FVec3<T>(Y * p.Z - Z * p.Y, Z * p.X - X * p.Z, X * p.Y - Y * p.X);
#endif
		}

		FVec3<T>& Normalize()
		{
			T l = X * X + Y * Y + Z * Z;
			if (l == 0)
				return *this;
			l = (T)TMath::ReciprocalSquareroot((float32)l);
			X *= l;
			Y *= l;
			Z *= l;
			return *this;
		}

		// random a FVec3 in [-1, 1]
		void Random()
		{
			const int rand_range = 0x7fff;
			const int rand_range_h = rand_range / 2;
			X = (T)((rand() & rand_range) - rand_range_h);
			Y = (T)((rand() & rand_range) - rand_range_h);
			Z = (T)((rand() & rand_range) - rand_range_h);
		}
		void RotateXZBy(float degrees, const FVec3<T>& center = FVec3<T>())
		{
			degrees = TMath::DegToRad(degrees);
			T cs = (T)cos(degrees);
			T sn = (T)sin(degrees);
			X -= center.X;
			Z -= center.Z;
			Set(X * cs - Z * sn, Y, X * sn + Z * cs);
			X += center.X;
			Z += center.Z;
		}

		void RotateXYBy(float degrees, const FVec3<T>& center = FVec3<T>())
		{
			degrees = TMath::DegToRad(degrees);
			T cs = (T)cos(degrees);
			T sn = (T)sin(degrees);
			X -= center.X;
			Y -= center.Y;
			Set(X * cs - Y * sn, X * sn + Y * cs, Z);
			X += center.X;
			Y += center.Y;
		}

		void RotateYZBy(float degrees, const FVec3<T>& center = FVec3<T>())
		{
			degrees = TMath::DegToRad(degrees);
			T cs = (T)cos(degrees);
			T sn = (T)sin(degrees);
			Z -= center.Z;
			Y -= center.Y;
			Set(X, Y * cs - Z * sn, Y * sn + Z * cs);
			Z += center.Z;
			Y += center.Y;
		}

		const T* Data() const
		{
			return reinterpret_cast<const T*>(this);
		}

		//!
		T* Data()
		{
			return reinterpret_cast<T*>(this);
		}

		T X, Y, Z;
	};

	//! Typedef for a float32 3d vector.
	typedef FVec3<float32> FFloat3;
	typedef FVec3<float16> FHalf3;

	//! Typedef for an integer 3d vector.
	typedef FVec3<int32> FInt3;
	typedef FVec3<uint32> FUInt3;

	//! Function multiplying a scalar and a vector component-wise.
	template<class S, class T>
	inline
		FVec3<T> operator*(const S scalar, const FVec3<T>& vector)
	{
		return vector * (T)scalar;
	}

} // end namespace tix

namespace std
{
	template <typename T>
	struct hash < tix::FVec3<T> >
	{
		std::size_t operator() (const tix::FVec3<T>& K) const
		{
			return ((hash<T>()(K.X)
				^ (hash<T>()(K.Y) << 1)) >> 1)
				^ (hash<T>()(K.Z) << 1);
		}
	};
}

