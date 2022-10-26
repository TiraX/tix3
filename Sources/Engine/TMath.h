/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

#include <math.h>	// system math lib
#include <cmath>

#define F32_AS_int32(f)		(*((int32 *) &(f)))
#define F32_AS_uint32(f)		(*((uint32 *) &(f)))
#define F32_AS_uint32_POINTER(f)	( ((uint32 *) &(f)))

#define F32_VALUE_0		0x00000000
#define F32_VALUE_1		0x3f800000
#define F32_SIGN_BIT		0x80000000U
#define F32_EXPON_MANTISSA	0x7FFFFFFFU

//! code is taken from IceFPU
//! Integer representation of a floating-point value.
#define IR(x)				((uint32&)(x))

//! Absolute integer representation of a floating-point value
#define AIR(x)				(IR(x)&0x7fffffff)

//! Floating-point representation of an integer value.
#define FR(x)				((float32&)(x))

//! integer representation of 1.0
#define IEEE_1_0			0x3f800000
//! integer representation of 255.0
#define IEEE_255_0			0x437f0000

#define	F32_LOWER_0(f)		(F32_AS_uint32(f) >  F32_SIGN_BIT)
#define	F32_LOWER_EQUAL_0(f)	(F32_AS_int32(f) <= F32_VALUE_0)
#define	F32_GREATER_0(f)	(F32_AS_int32(f) >  F32_VALUE_0)
#define	F32_GREATER_EQUAL_0(f)	(F32_AS_uint32(f) <= F32_SIGN_BIT)
#define	F32_EQUAL_1(f)		(F32_AS_uint32(f) == F32_VALUE_1)
#define	F32_EQUAL_0(f)		( (F32_AS_uint32(f) & F32_EXPON_MANTISSA ) == F32_VALUE_0)

// only same sign
#define	F32_A_GREATER_B(a,b)	(F32_AS_int32((a)) > F32_AS_int32((b)))


const float32 ROUNDING_ERROR_32 = 0.00005f;
const float64 ROUNDING_ERROR_64 = 0.000005;

#ifdef PI // make sure we don't collide with a define
#undef PI
#endif
//! Constant for PI.
const float32 PI = 3.14159265359f;

const float32 SMALL_NUMBER = 1.e-8f;
const float32 KINDA_SMALL_NUMBER = 1.e-4f;

//! Constant for reciprocal of PI.
const float32 RECIPROCAL_PI = 1.0f/PI;

//! Constant for half of PI.
const float32 HALF_PI = PI/2.0f;

//! 32bit Constant for converting from degrees to radians
const float32 DEGTORAD = PI / 180.0f;

//! 32bit constant for converting from radians to degrees (formally known as GRAD_PI)
const float32 RADTODEG = 180.0f / PI;

#ifdef PI64 // make sure we don't collide with a define
#undef PI64
#endif
//! Constant for 64bit PI.
const float64 PI64		= 3.1415926535897932384626433832795028841971693993751;

//! Constant for 64bit reciprocal of PI.
const float64 RECIPROCAL_PI64 = 1.0/PI64;


namespace tix
{
	class TMath
	{
	public:
		static FORCEINLINE int32 Round(float n)
		{
			if (n >= 0.f)
			{
				return (int)(n + 0.5f);
			}
			else
			{
				return (int)(n - 0.5f);
			}
		}

		/**
		* Converts a float to the nearest integer. Rounds up when the fraction is .5
		* @param F		Floating point value to convert
		* @return		The nearest integer to 'F'.
		*/
		static FORCEINLINE float RoundToFloat(float F)
		{
			return floorf(F + 0.5f);
		}

		template< class T >
		static FORCEINLINE void Swap(T& a, T& b)
		{
			T Tmp = a;
			a = b;
			b = Tmp;
		}

		template< class T >
		static FORCEINLINE T Sqr(T v)
		{
			return v * v;
		}

		static FORCEINLINE float Sqrt(float v)
		{
			return sqrtf(v);
		}

		static FORCEINLINE float DegToRad(const float Degree)
		{
			return Degree * DEGTORAD;
		}

		static FORCEINLINE float RadToDeg(const float Radian)
		{
			return Radian * RADTODEG;
		}

		template< class T >
		static FORCEINLINE bool IsNaN(const T v)
		{
			return isnan(v);
		}

		template< class T >
		static FORCEINLINE bool IsInF(const T v)
		{
			return isinf(v);
		}

		template< class T >
		static FORCEINLINE T Lerp(const T src, const T dest, const float t)
		{
			return (T)((dest - src) * t + src);
		}

		template< class T >
		static FORCEINLINE T LerpStable(const T src, const T dest, const float t)
		{
			return (T)((src * (1.0f - t)) + (dest * t));
		}

		template< class T >
		static FORCEINLINE T Max(const T a, const T b)
		{
			return a > b ? a : b;
		}

		template< class T >
		static FORCEINLINE T Min(const T a, const T b)
		{
			return a < b ? a : b;
		}

		template< class T >
		static FORCEINLINE T Max3(const T a, const T b, const T c)
		{
			return Max(Max(a, b), c);
		}

		template< class T >
		static FORCEINLINE T Min3(const T a, const T b, const T c)
		{
			return Min(Min(a, b), c);
		}

		template< class T >
		static FORCEINLINE T Clamp(const T X, const T Min, const T Max)
		{
			return X < Min ? Min : X < Max ? X : Max;
		}

		template< class T >
		static FORCEINLINE T Abs(const T x)
		{
			if (x > 0)
				return x;

			return -x;
		}

		template< class T >
		static FORCEINLINE T Modf(const T x)
		{
			float Temp;
			return modf(x, &Temp);
		}

		template< class T >
		static FORCEINLINE T Frac(const T x)
		{
			return Modf(x);
		}

		template< class T >
		static FORCEINLINE T Floor(T x)
		{
			return floor(x);
		}

		static FORCEINLINE int32 FloorToInt(float x)
		{
			return (int32)floor(x);
		}

		static FORCEINLINE float Ceil(float x)
		{
			return ceil(x);
		}

		static FORCEINLINE int32 CeilToInt(float x)
		{
			return (int32)ceil(x);
		}

		static FORCEINLINE uint32 CeilLogTwo(uint32 Arg)
		{
			int32 Bitmask = ((int32)(CountLeadingZeros(Arg) << 26)) >> 31;
			return (32 - CountLeadingZeros(Arg - 1)) & (~Bitmask);
		}

		static FORCEINLINE uint32 RoundUpToPowerOfTwo(uint32 Arg)
		{
			return 1 << CeilLogTwo(Arg);
		}

		static FORCEINLINE float Pow(float X, float Y)
		{
			return powf(X, Y);
		}

		static FORCEINLINE int32 CountBits64(uint64 Bits)
		{
			// https://en.wikipedia.org/wiki/Hamming_weight
			Bits -= (Bits >> 1) & 0x5555555555555555ull;
			Bits = (Bits & 0x3333333333333333ull) + ((Bits >> 2) & 0x3333333333333333ull);
			Bits = (Bits + (Bits >> 4)) & 0x0f0f0f0f0f0f0f0full;
			return (Bits * 0x0101010101010101) >> 56;
		}

		static FORCEINLINE int32 CountBits32(uint32 value)
		{
			int32 num = 0;
			while (value)
			{
				value &= (value - 1);
				++num;
			}
			return num;
		}

		static FORCEINLINE int32 Log(uint32 value)
		{
			int num = 0;
			while (value)
			{
				value >>= 1;
				++num;
			}
			return num;
		}


		static FORCEINLINE float Loge(float Value) { return logf(Value); }
		/**
		 * Computes the base 2 logarithm of the specified value
		 *
		 * @param Value the value to perform the log on
		 *
		 * @return the base 2 log of the value
		 */
		static FORCEINLINE float Log2(float Value)
		{
			// Cached value for fast conversions
			constexpr float LogToLog2 = 1.44269502f; // 1.f / Loge(2.f)
			// Do the platform specific log and convert using the cached value
			return Loge(Value) * LogToLog2;
		}

		static FORCEINLINE void RandSeed(uint32 Seed)
		{
			srand(Seed);
		}

		//! returns a random integer
		static FORCEINLINE int32 Rand()
		{
			return rand();
		}

		//! returns a float value between 0.0 ~ 1.0
		static FORCEINLINE float RandomUnit()
		{
			const float k_inv = 1.0f / RAND_MAX;
			return rand() * k_inv;
		}

		//! returns if a equals b, taking possible rounding errors into account
		static FORCEINLINE bool Equals(const float a, const float b, const float tolerance = ROUNDING_ERROR_32)
		{
			return (a + tolerance >= b) && (a - tolerance <= b);
		}

		static FORCEINLINE bool GreatEqual(const float a, const float b, const float tolerance = ROUNDING_ERROR_32)
		{
			return (a + tolerance) >= b;
		}

		static FORCEINLINE bool LessEqual(const float a, const float b, const float tolerance = ROUNDING_ERROR_32)
		{
			return a <= (b + tolerance);
		}

		//! returns if a equals zero, taking rounding errors into account
		static FORCEINLINE bool IsZero(const float32 a, const float32 tolerance = ROUNDING_ERROR_32)
		{
			return Equals(a, 0.0f, tolerance);
		}
		
		//! Is power of 2
		static bool IsPowerOf2(int32 n)
		{
			return (n & (n - 1)) == 0;
		}

		static FORCEINLINE float ReciprocalSquareroot(const float32 x)
		{
			// comes from Nvidia
			uint32 tmp = ((unsigned int)(IEEE_1_0 << 1) + IEEE_1_0 - *(uint32*)&x) >> 1;
			float y = *(float*)&tmp;
			return y * (1.47f - 0.47f * x * y * y);
		}

		// from Quake3
		static FORCEINLINE float Q_rsqrt(float number)
		{
			long i;
			float x2, y;
			const float threehalfs = 1.5F;

			x2 = number * 0.5F;
			y = number;
			i = *(long*)&y;   // evil floating point bit level hacking
			i = 0x5f3759df - (i >> 1); // what the fuck?
			y = *(float*)&i;
			y = y * (threehalfs - (x2 * y * y)); // 1st iteration
			// y   = y * ( threehalfs - ( x2 * y * y ) ); // 2nd iteration, this can be removed

			return y;
		}

		static FORCEINLINE float FMod(float x, float y)
		{
			return fmod(x, y);
		}

		template<class T>
		static FORCEINLINE T Align(T n, uint32 align_num)
		{
			return (n + align_num - 1) & (~(align_num - 1));
		}

		template<class T>
		static FORCEINLINE T Align4(T n)
		{
			return Align<T>(n, 4);
		}

		template<class T>
		static FORCEINLINE T Align16(T n)
		{
			return Align<T>(n, 16);
		}

		//! Remap value v's range[s0, s1] to [t0, t1]
		static FORCEINLINE float Fit(float v, float s0, float s1, float t0, float t1)
		{
			TI_ASSERT(s1 - s0 != 0);
			v = TMath::Clamp(v, s0, s1);
			return (v - s0) / (s1 - s0) * (t1 - t0) + t0;
		}

		//! Equals Fit(v, 0, 1, t0, t1)
		static FORCEINLINE float Fit01(float v, float t0, float t1)
		{
			v = TMath::Clamp(v, 0.f, 1.f);
			return v * (t1 - t0) + t0;
		}

		//! Equals Fit(v, s0, s1, 0, 1)
		static FORCEINLINE float FitTo01(float v, float s0, float s1)
		{
			TI_ASSERT(s1 - s0 != 0);
			return TMath::Clamp((v - s0) / (s1 - s0), 0.f, 1.f);
		}

		static FORCEINLINE TString IToA(int32 num)
		{
			char NumS[32];
			sprintf(NumS, "%d", num);
			return TString(NumS);
		}

		static FORCEINLINE int32 AToI(const TString& S)
		{
			return atoi(S.c_str());
		}

		// From UE4 
		/**
		 * Computes the base 2 logarithm for an integer value that is greater than 0.
		 * The result is rounded down to the nearest integer.
		 *
		 * @param Value		The value to compute the log of
		 * @return			Log2 of Value. 0 if Value is 0.
		 */
#if defined (TI_PLATFORM_WIN32)
#pragma intrinsic( _BitScanReverse )
#endif
		static FORCEINLINE uint32 FloorLog2(uint32 Value)
		{
#if defined (TI_PLATFORM_WIN32)
			// Use BSR to return the log2 of the integer
			unsigned long Log2;
			if (_BitScanReverse(&Log2, Value) != 0)
			{
				return Log2;
			}

			return 0;
#else
			// see http://codinggorilla.domemtech.com/?p=81 or http://en.wikipedia.org/wiki/Binary_logarithm but modified to return 0 for a input value of 0
			// 686ms on test data
			uint32 pos = 0;
			if (Value >= 1 << 16) { Value >>= 16; pos += 16; }
			if (Value >= 1 << 8) { Value >>= 8; pos += 8; }
			if (Value >= 1 << 4) { Value >>= 4; pos += 4; }
			if (Value >= 1 << 2) { Value >>= 2; pos += 2; }
			if (Value >= 1 << 1) { pos += 1; }
			return (Value == 0) ? 0 : pos;
#endif
		}

		static FORCEINLINE uint32 CountTrailingZeros(uint32 Value)
		{
			if (Value == 0)
			{
				return 32;
			}
			unsigned long BitIndex;	// 0-based, where the LSB is 0 and MSB is 31
			_BitScanForward(&BitIndex, Value);	// Scans from LSB to MSB
			return BitIndex;
		}

		static FORCEINLINE uint32 CountLeadingZeros(uint32 Value)
		{
			unsigned long Log2;
			_BitScanReverse64(&Log2, (uint64(Value) << 1) | 1);
			return 32 - Log2;
		}

		/** Spreads bits to every 3rd. */
		static FORCEINLINE uint32 MortonCode3(uint32 x)
		{
			x &= 0x000003ff;
			x = (x ^ (x << 16)) & 0xff0000ff;
			x = (x ^ (x << 8)) & 0x0300f00f;
			x = (x ^ (x << 4)) & 0x030c30c3;
			x = (x ^ (x << 2)) & 0x09249249;
			return x;
		}

		/** Reverses MortonCode3. Compacts every 3rd bit to the right. */
		static FORCEINLINE uint32 ReverseMortonCode3(uint32 x)
		{
			x &= 0x09249249;
			x = (x ^ (x >> 2)) & 0x030c30c3;
			x = (x ^ (x >> 4)) & 0x0300f00f;
			x = (x ^ (x >> 8)) & 0xff0000ff;
			x = (x ^ (x >> 16)) & 0x000003ff;
			return x;
		}
	};
}



#include "Math/Vec2.h"
#include "Math/Vec3.h"
#include "Math/Vec4.h"
#include "Math/Line2d.h"
#include "Math/Line3d.h"
#include "Math/Rect.h"
#include "Math/Box.h"
#include "Math/Sphere.h"
#include "Math/Plane3d.h"
#include "Math/Matrix4.h"
#include "Math/Matrix3.h"
#include "Math/Matrix34.h"
#include "Math/Quaternion.h"
#include "Math/SViewFrustum.h"
#include "Math/Triangle3d.h"
#include "Math/FSHVector.h"
