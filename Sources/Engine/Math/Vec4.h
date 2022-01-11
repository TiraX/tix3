/*
	TiX Engine v2.0 Copyright (C) 2018~2021
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	template < typename T >
	class FVec4
	{
	public:
		FVec4()
			: X(0)
			, Y(0)
			, Z(0)
			, W(0)
		{}

		FVec4(T nx, T ny, T nz, T nw)
			: X(nx)
			, Y(ny)
			, Z(nz)
			, W(nw)
		{}

		FVec4(const SColor& C)
			: X(C.R)
			, Y(C.G)
			, Z(C.B)
			, W(C.A)
		{}

		FVec4(const SColorf & C)
			: X(C.R)
			, Y(C.G)
			, Z(C.B)
			, W(C.A)
		{}

		FVec4(const FVec3<T> & V3)
			: X(V3.X)
			, Y(V3.Y)
			, Z(V3.Z)
			, W(0.0)
		{}

		FVec4(const FVec4<T>& other)
			: X(other.X)
			, Y(other.Y)
			, Z(other.Z)
			, W(other.W)
		{}

		bool operator==(const FVec4<T>& other) const
		{
			return X == other.X && Y == other.Y && Z == other.Z && W == other.W;
		}

		bool operator!=(const FVec4<T>& other) const
		{
			return X != other.X || Y != other.Y || Z != other.Z || W != other.W;
		}

		FVec4<T> operator+(const FVec4<T>& other) const
		{
			return FVec4<T>(X + other.X, Y + other.Y, Z + other.Z, W + other.W);
		}

		FVec4<T>& operator+=(const FVec4<T>& other)
		{
			X += other.X;
			Y += other.Y;
			Z += other.Z;
			W += other.W;
			return *this;
		}

		FVec4<T> operator-(const FVec4<T>& other) const
		{
			return FVec4<T>(X - other.X, Y - other.Y, Z - other.Z, W - other.W);
		}

		FVec4<T>& operator-=(const FVec4<T>& other)
		{
			X -= other.X;
			Y -= other.Y;
			Z -= other.Z;
			W -= other.W;
			return *this;
		}

		FVec4<T> operator*(const FVec4<T>& other) const
		{
			return FVec4<T>(X * other.X, Y * other.Y, Z * other.Z, W * other.W);
		}

		FVec4<T>& operator*=(const FVec4<T>& other)
		{
			X *= other.X;
			Y *= other.Y;
			Z *= other.Z;
			W *= other.W;
			return *this;
		}

		FVec4<T> operator*(const T v) const
		{
			return FVec4<T>(X * v, Y * v, Z * v, W * v);
		}

		FVec4<T>& operator*=(const T v)
		{
			X *= v;
			Y *= v;
			Z *= v;
			W *= v;
			return *this;
		}

		FVec4<T> operator/(const FVec4<T>& other) const
		{
			return FVec4<T>(X / other.X, Y / other.Y, Z / other.Z, W / other.W);
		}

		FVec4<T>& operator/=(const FVec4<T>& other)
		{
			X /= other.X;
			Y /= other.Y;
			Z /= other.Z;
			W /= other.W;
			return *this;
		}

		FVec4<T> operator/(const T v) const
		{
			T i = (T)1.0 / v;
			return FVec4<T>(X * i, Y * i, Z * i, W * i);
		}

		FVec4<T>& operator/=(const T v)
		{
			T i = (T)1.0 / v;
			X *= i;
			Y *= i;
			Z *= i;
			W *= i;
			return *this;
		}

		T X, Y, Z, W;
	};

	//! Typedef for a float32 4d vector.
	typedef FVec4<float32> FFloat4;
	typedef FVec4<float16> FHalf4;
	//! Typedef for an integer 4d vector.
	typedef FVec4<int32> FInt4;
	typedef FVec4<uint32> FUInt4;

}
