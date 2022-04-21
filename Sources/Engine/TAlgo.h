/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{

#define TSort std::sort
#define TFill std::fill
#define TFind std::find

	class TAlgo
	{
	public:
		template< class T >
		static inline void Sort(TVector<T>& V)
		{
			TSort(V.begin(), V.end());
		}

		template< class T >
		static inline void Fill(TVector<T>& V)
		{
			TFill(V.begin(), V.end());
		}

		// Sort Values, output an order
		template<class T>
		static inline void ArgSort(const TVector<T>& Values, TVector<int32>& OutOrders)
		{
			const int32 Count = (int32)Values.size();
			OutOrders.clear();
			OutOrders.resize(Count);
			TVector<int32> Order;
			Order.resize(Count);
			for (int32 i = 0; i < Count; i++)
			{
				Order[i] = i;
			}

			TSort(Order.begin(), Order.end(),
				[&Values](int32 p0, int32 p1)
				{
					return Values[p0] < Values[p1];
				});

			// Give this kind of order, easy for reorder index
			for (int32 i = 0; i < Count; i++)
			{
				OutOrders[Order[i]] = i;
			}
		}

		// Rearrange values by given order(usually from ArgSort)
		template<class T>
		static inline void Reorder(TVector<T>& Values, const TVector<int32>& Orders)
		{
			TI_ASSERT(Values.size() == Orders.size());
			TVector<T> Result;
			Result.resize(Orders.size());

			const int32 Num = (int32)Orders.size();
			for (int32 i = 0; i < Num; i++)
			{
				Result[Orders[i]] = Values[i];
			}
			Values.swap(Result);
		}
	};
}
