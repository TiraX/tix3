/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	template <typename T>
	class TVectorView
	{
	public:
		TVectorView()
			: Pointer(nullptr)
			, Count(0)
		{}

		TVectorView(T * Ptr, int32 InCount)
			: Pointer(Ptr)
			, Count(InCount)
		{}

		TVectorView(TVector<T>& V)
			: Pointer(V.data())
			, Count((int32)V.size())
		{}

		int32 Size() const
		{
			return Count;
		}

		T& operator [] (uint32 i)
		{
			TI_ASSERT(i < (uint32)Count);
			return Pointer[i];
		}

		const T& operator [] (uint32 i) const
		{
			TI_ASSERT(i < (uint32)Count);
			return Pointer[i];
		}

		TVectorView Slice(uint32 Index, uint32 InNum) const
		{
			TI_ASSERT(Index + InNum <= (uint32)Count);
			return TVectorView(Pointer + Index, InNum);
		}

		template<class _Pr>
		void Sort(_Pr _Pred)
		{
			TVector<T> V;
			V.resize(Size());
			for (int32 i = 0; i < Count; i++)
			{
				V[i] = Pointer[i];
			}

			TSort(V.begin(), V.end(), _Pred);
			for (int32 i = 0; i < Count; i++)
			{
				Pointer[i] = V[i];
			}
		}

	protected:

	protected:
		T* Pointer;
		int32 Count;
	};
}