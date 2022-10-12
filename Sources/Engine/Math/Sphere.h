/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	class FSphere
	{
	public:
		FSphere()
			: Radius(0.f)
		{}

		FSphere(const FFloat3& InCenter, float InRadius)
			: Center(InCenter)
			, Radius(InRadius)
		{}

		bool IsPointInsideSphere(const FFloat3& P) const
		{
			return (Center - P).GetLength() <= Radius;
		}

		bool operator == (const FSphere& Other) const
		{
			return Center == Other.Center && Radius == Other.Radius;
		}

	public:
		FFloat3 Center;
		float Radius;
	};
}