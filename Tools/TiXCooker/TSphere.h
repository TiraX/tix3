/*
	TiX Engine v2.0 Copyright (C) 2018~2021
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

struct TSphere
{
	FFloat3 Center;
	float Radius;

	TSphere()
		: Radius(0.f)
	{}

	TSphere(const FFloat3& InCenter, float InRadius)
		: Center(InCenter)
		, Radius(InRadius)
	{}

	bool IsPointInsideSphere(const FFloat3& P) const
	{
		return (Center - P).getLength() <= Radius;
	}

	bool operator == (const TSphere& Other) const
	{
		return Center == Other.Center && Radius == Other.Radius;
	}
};