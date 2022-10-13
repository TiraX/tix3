/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	template <typename T>
	class FVec2
	{
	public:
		FVec2()
			: X(0)
			, Y(0)
		{}

		FVec2(T nx, T ny)
			: X(nx)
			, Y(ny)
		{}

		explicit FVec2(T n)
			: X(n)
			, Y(n)
		{}

		FVec2(const FVec2<T>& other)
			: X(other.X)
			, Y(other.Y)
		{}

		template <typename T2>
		explicit FVec2(const FVec2<T2>& other)
			: X(T(other.X))
			, Y(T(other.Y))
		{}

		T& operator [] (uint32 i)
		{
			TI_ASSERT(i < 2);
			return Data()[i];
		}

		const T& operator [] (uint32 i) const
		{
			TI_ASSERT(i < 2);
			return Data()[i];
		}

		FVec2<T> operator-() const
		{
			return FVec2<T>(-X, -Y);
		}

		FVec2<T>& operator=(const FVec2<T>& other)
		{
			X = other.X;
			Y = other.Y;
			return *this;
		}

		FVec2<T> operator+(const FVec2<T>& other) const
		{
			return FVec2<T>(X + other.X, Y + other.Y);
		}

		FVec2<T>& operator+=(const FVec2<T>& other)
		{
			X += other.X;
			Y += other.Y;
			return *this;
		}

		FVec2<T> operator+(const T v) const
		{
			return FVec2<T>(X + v, Y + v);
		}

		FVec2<T>& operator+=(const T v)
		{
			X += v;
			Y += v;
			return *this;
		}

		FVec2<T> operator-(const FVec2<T>& other) const
		{
			return FVec2<T>(X - other.X, Y - other.Y);
		}

		FVec2<T>& operator-=(const FVec2<T>& other)
		{
			X -= other.X;
			Y -= other.Y;
			return *this;
		}

		FVec2<T> operator-(const T v) const
		{
			return FVec2<T>(X - v, Y - v);
		}

		FVec2<T>& operator-=(const T v)
		{
			X -= v;
			Y -= v;
			return *this;
		}

		FVec2<T> operator*(const FVec2<T>& other) const
		{
			return FVec2<T>(X * other.X, Y * other.Y);
		}

		FVec2<T>& operator*=(const FVec2<T>& other)
		{
			X *= other.X;
			Y *= other.Y;
			return *this;
		}

		FVec2<T> operator*(const T v) const
		{
			return FVec2<T>(X * v, Y * v);
		}

		FVec2<T>& operator*=(const T v)
		{
			X *= v;
			Y *= v;
			return *this;
		}

		FVec2<T> operator/(const FVec2<T>& other) const
		{
			return FVec2<T>(X / other.X, Y / other.Y);
		}

		FVec2<T>& operator/=(const FVec2<T>& other)
		{
			X /= other.X;
			Y /= other.Y;
			return *this;
		}

		FVec2<T> operator/(const T v) const
		{
			return FVec2<T>(X / v, Y / v);
		}

		FVec2<T>& operator/=(const T v)
		{
			X /= v;
			Y /= v;
			return *this;
		}

		// overload operator< for std::map compare
		bool operator<(const FVec2<T>& other) const
		{
			if (X != other.X)
				return X < other.X;
			return Y < other.Y;
		}

		bool operator==(const FVec2<T>& other) const
		{
			return other.X == X && other.Y == Y;
		}

		bool operator!=(const FVec2<T>& other) const
		{
			return other.X != X || other.Y != Y;
		}

		bool Equals(const FVec2<T>& other) const
		{
			return TMath::Equals(X, other.X) && TMath::Equals(Y, other.Y);
		}

		T GetLength() const
		{
			return (T)TMath::Sqrt((float64)(X * X + Y * Y));
		}

		T GetLengthSQ() const
		{
			return X * X + Y * Y;
		}

		T Dot(const FVec2<T>& other) const
		{
			return X * other.X + Y * other.Y;
		}

		FVec2<T>& RotateBy(float degrees, const FVec2<T>& center)
		{
			degrees = TMath::DegToRad(degrees);
			const T cs = (T)cos(degrees);
			const T sn = (T)sin(degrees);

			X -= center.X;
			Y -= center.Y;

			Set(X * cs - Y * sn, X * sn + Y * cs);

			X += center.X;
			Y += center.Y;
			return *this;
		}

		FVec2<T>& Normalize()
		{
			T l = X * X + Y * Y;
			if (l == 0)
				return *this;
			l = TMath::ReciprocalSquareroot((float32)l);
			X *= l;
			Y *= l;
			return *this;
		}

		//! Calculates the angle of this vector in degrees in the trigonometric sense.
		/** 0 is to the left (9 o'clock), values increase clockwise.
		This method has been suggested by Pr3t3nd3r.
		\return Returns a value between 0 and 360. */
		float GetAngleTrig() const
		{
			if (X == 0)
				return Y < 0 ? 270 : 90;
			else
				if (Y == 0)
					return X < 0 ? 180 : 0;

			if (Y > 0)
				if (X > 0)
					return TMath::RadToDeg(atan(Y / X));
				else
					return 180.0 - TMath::RadToDeg(atan(Y / -X));
			else
				if (X > 0)
					return 360.0 - TMath::RadToDeg(atan(-Y / X));
				else
					return 180.0 + TMath::RadToDeg(atan(-Y / -X));
		}

		//! Calculates the angle of this vector in degrees in the counter trigonometric sense.
		/** 0 is to the right (3 o'clock), values increase counter-clockwise.
		\return Returns a value between 0 and 360. */
		inline float GetAngle() const
		{
			if (Y == 0) // corrected thanks to a suggestion by Jox
				return X < 0 ? 180 : 0;
			else if (X == 0)
				return Y < 0 ? 90 : 270;

			float64 tmp = Y / GetLength();
			tmp = TMath::RadToDeg(atan(sqrt(1 - tmp * tmp) / tmp));

			if (X > 0 && Y > 0)
				return tmp + 270;
			else
				if (X > 0 && Y < 0)
					return tmp + 90;
				else
					if (X < 0 && Y < 0)
						return 90 - tmp;
					else
						if (X < 0 && Y>0)
							return 270 - tmp;

			return tmp;
		}

		//! Calculates the angle between this vector and another one in degree.
		/** \param b Other vector to test with.
		\return Returns a value between 0 and 90. */
		inline float GetAngleWith(const FVec2<T>& b) const
		{
			float64 tmp = X * b.X + Y * b.Y;

			if (tmp == 0.0)
				return 90.0;

			tmp = tmp / sqrt((float64)((X * X + Y * Y) * (b.X * b.X + b.Y * b.Y)));
			if (tmp < 0.0)
				tmp = -tmp;

			return TMath::RadToDeg(atan(sqrt(1 - tmp * tmp) / tmp));
		}

		const T* Data() const
		{
			return reinterpret_cast<const T*>(this);
		}

		T* Data()
		{
			return reinterpret_cast<T*>(this);
		}

		T X, Y;
	};

	//! Typedef for float32 2d vector.
	typedef FVec2<float32> FFloat2;
	typedef FVec2<float16> FHalf2;
	//! Typedef for integer 2d vector.
	typedef FVec2<int32> FInt2;
	typedef FVec2<uint32> FUInt2;
	typedef FVec2<int16> FHInt2;

	typedef FVec2<int64> FDInt2;

	template<class S, class T>
	FVec2<T> operator*(const S scalar, const FVec2<T>& vector)
	{
		return vector * scalar;
	}

} // end namespace tix

namespace std
{
	template <typename T>
	struct hash < tix::FVec2<T> >
	{
		std::size_t operator() (const tix::FVec2<T>& K) const
		{
			return ((hash<T>()(K.X) ^ (hash<T>()(K.Y) << 1)) >> 1);
		}
	};
}

