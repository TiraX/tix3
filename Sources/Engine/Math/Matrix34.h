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

		~FMat34()
		{}

		void SetTranslation(const FFloat3& translation)
		{
			M[3] = translation.X;
			M[7] = translation.Y;
			M[11] = translation.Z;
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
