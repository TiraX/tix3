/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	class FMat34
	{
	public:
		static const int32 NumElements = 12;
		FMat34()
		{
			memset(M, 0, sizeof(float) * NumElements);
			M[0] = 1.f;
			M[5] = 1.f;
			M[10] = 1.f;
		}

		FMat34(const FMat34& Other)
		{
			*this = Other;
		}

		~FMat34()
		{}

		void SetTranslation(const FFloat3& translation)
		{
			M[3] = translation.X;
			M[7] = translation.Y;
			M[11] = translation.Z;
		}

		void TransformVect(FFloat3& vect) const
		{
			float vector[3];

			vector[0] = vect.X * M[0] + vect.Y * M[4] + vect.Z * M[8] + M[3];
			vector[1] = vect.X * M[1] + vect.Y * M[5] + vect.Z * M[9] + M[7];
			vector[2] = vect.X * M[2] + vect.Y * M[6] + vect.Z * M[10] + M[11];

			vect.X = vector[0];
			vect.Y = vector[1];
			vect.Z = vector[2];
		}

		void RotateVect(FFloat3& vect) const
		{
			float vector[3];

			vector[0] = vect.X * M[0] + vect.Y * M[4] + vect.Z * M[8];
			vector[1] = vect.X * M[1] + vect.Y * M[5] + vect.Z * M[9];
			vector[2] = vect.X * M[2] + vect.Y * M[6] + vect.Z * M[10];

			vect.X = vector[0];
			vect.Y = vector[1];
			vect.Z = vector[2];
		}

		FMat34& operator = (const FMat34& Other)
		{
			M[0] = Other.M[0];
			M[1] = Other.M[1];
			M[2] = Other.M[2];
			M[3] = Other.M[3];
			M[4] = Other.M[4];
			M[5] = Other.M[5];
			M[6] = Other.M[6];
			M[7] = Other.M[7];
			M[8] = Other.M[8];
			M[9] = Other.M[9];
			M[10] = Other.M[10];
			M[11] = Other.M[11];
			return *this;
		}

		float& operator [] (uint32 Index)
		{
			TI_ASSERT(Index < NumElements);
			return M[Index];
		}

		const float& operator [] (uint32 Index) const
		{
			TI_ASSERT(Index < NumElements);
			return M[Index];
		}

		float* Data()
		{
			return M;
		}

		const float* Data() const
		{
			return M;
		}
	protected:
		float M[NumElements];
	};
}
