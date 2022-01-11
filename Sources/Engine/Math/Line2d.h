/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	template <class T>
	class FLine2D
	{
	public:
		FLine2D() 
			: Start(0, 0)
			, End(1, 1) 
		{}

		FLine2D(T xa, T ya, T xb, T yb) 
			: Start(xa, ya)
			, End(xb, yb) 
		{}

		// Line-Line Intersection
		bool IsIntersectWithLine2d(const FLine2D<T>& other)
		{
			// algorithm from
			// http://stackoverflow.com/questions/563198/how-do-you-detect-where-two-line-segments-intersect
			T s1_x, s1_y, s2_x, s2_y;
			s1_x = End.X - Start.X;
			s1_y = End.Y - Start.Y;
			s2_x = other.End.X - other.Start.X;
			s2_y = other.End.Y - other.Start.Y;

			float s, t;
			float det = (float)(-s2_x * s1_y + s1_x * s2_y);
			s = (-s1_y * (Start.X - other.Start.X) + s1_x * (Start.Y - other.Start.Y)) / det;
			t = (s2_x * (Start.Y - other.Start.Y) - s2_y * (Start.X - other.Start.X)) / det;

			if (s >= 0 && s <= 1 && t >= 0 && t <= 1)
			{
				// Collision detected
				//if (i_x != NULL)
				//	*i_x = p0_x + (t * s1_x);
				//if (i_y != NULL)
				//	*i_y = p0_y + (t * s1_y);
				return true;
			}

			return false; // No collision
		}


		// member variables

		//! Start point of line
		FVec2<T> Start;
		//! End point of line
		FVec2<T> End;
	};

	//! Typedef for an float32 line.
	typedef FLine2D<float32> FLine2;

}
