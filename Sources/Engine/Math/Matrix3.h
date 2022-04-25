/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	class FMat3
	{
	public:
		static const int32 NumElements = 9;
		FMat3()
		{
			memset(M, 0, sizeof(float) * NumElements);
			M[0] = 1.f;
			M[4] = 1.f;
			M[8] = 1.f;
		}

		FMat3(float a0, float a1, float a2,
			float a3, float a4, float a5,
			float a6, float a7, float a8)
		{
			M[0] = a0;
			M[1] = a1;
			M[2] = a2;
			M[3] = a3;
			M[4] = a4;
			M[5] = a5;
			M[6] = a6;
			M[7] = a7;
			M[8] = a8;
		}

		FMat3(const FMat3& Other)
		{
			*this = Other;
		}

		~FMat3()
		{}

		FMat3& MadeBy(FFloat3 VecForward, FFloat3 VecUp)
		{
			VecUp = VecForward.Cross(VecUp);
			VecUp = VecUp.Cross(VecForward);
			VecUp.Normalize();

			FFloat3 T = VecUp.Cross(VecForward);
			T.Normalize();

			M[0] = T.X;
			M[1] = T.Y;
			M[2] = T.Z;
			M[3] = VecForward.X;
			M[4] = VecForward.Y;
			M[5] = VecForward.Z;
			M[6] = VecUp.X;
			M[7] = VecUp.Y;
			M[8] = VecUp.Z;
			return *this;
		}

		FMat3& operator = (const FMat3& Other)
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
			return *this;
		}

		// returns transposed matrix
		inline FMat3 GetTransposed() const
		{
			FMat3 T;
			T.M[0] = M[0];
			T.M[1] = M[3];
			T.M[2] = M[6];

			T.M[3] = M[1];
			T.M[4] = M[4];
			T.M[5] = M[7];

			T.M[6] = M[2];
			T.M[7] = M[5];
			T.M[8] = M[8];
			return T;
		}	
		
		inline void TransformVect(FFloat3& vect) const
		{
			float vector[3];

			vector[0] = vect.X * M[0] + vect.Y * M[3] + vect.Z * M[6];
			vector[1] = vect.X * M[1] + vect.Y * M[4] + vect.Z * M[7];
			vector[2] = vect.X * M[2] + vect.Y * M[5] + vect.Z * M[8];

			vect.X = vector[0];
			vect.Y = vector[1];
			vect.Z = vector[2];
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
