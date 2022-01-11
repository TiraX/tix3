/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	template <class T>
	class FRect2D
	{
	public:
		FRect2D() 
			: Upper(0)
			, Left(0)
			, Lower(0)
			, Right(0) 
		{}

		FRect2D(T x, T y, T x2, T y2)
			: Upper(y)
			, Left(x)
			, Lower(y2)
			, Right(x2)
		{}

		void Reset(T x, T y)
		{
			Left = x;
			Right = x;
			Upper = y;
			Lower = y;
		}

		void Reset(const FVec2<T>& p)
		{
			reset(p.X, p.Y);
		}

		bool operator==(const FRect2D<T>& other) const
		{
			return (Upper == other.Upper &&
				Left == other.Left &&
				Lower == other.Lower &&
				Right == other.Right);
		}


		bool operator!=(const FRect2D<T>& other) const
		{
			return (Upper != other.Upper ||
				Left != other.Left ||
				Lower != other.Lower ||
				Right != other.Right);
		}

		FRect2D<T>& operator+=(const FRect2D<T>& other)
		{
			addInternalPoint(other.Upper, other.Left);
			addInternalPoint(other.Lower, other.Right);
			return *this;
		}

		FRect2D<T> operator * (T num) const
		{
			FRect2D<T> rc;
			rc.Left = this->Left * num;
			rc.Right = this->Right * num;
			rc.Upper = this->Upper * num;
			rc.Lower = this->Lower * num;

			return rc;
		}

		FRect2D<T> operator * (const FRect2D<T>& other) const
		{
			FRect2D<T> rc;
			rc.Left = Left * other.Left;
			rc.Right = Right * other.Right;
			rc.Upper = Upper * other.Upper;
			rc.Lower = Lower * other.Lower;

			return rc;
		}

		// compares size of rectangles
		bool operator<(const FRect2D<T>& other) const
		{
			return GetArea() < other.GetArea();
		}

		//! Returns size of rectangle
		T GetArea() const
		{
			return GetWidth() * GetHeight();
		}

		//! Returns if a 2d point is within this rectangle.
		//! \return Returns true if the position is within the rectangle, false if not.
		bool IsPointInside(T x, T y) const
		{
			return (x >= Left && x <= Right &&
				y >= Upper && y <= Lower);
		}

		//! Returns if the rectangle collides with another rectangle.
		bool IsRectCollided(const FRect2D<T>& other) const
		{
			return (Lower > other.Upper &&
				Upper < other.Lower&&
				Right > other.Left &&
				Left < other.Right);
		}

		//! Returns if the rectangle collides with a circle
		bool IsRectCollidedWithCircle(const FVec2<T>& point, T radius) const
		{
			FVec2<T> center = GetCenter();
			return	TMath::Abs(center.X - point.X) < radius + GetWidth() / 2 &&
				TMath::Abs(center.Y - point.Y) < radius + GetHeight() / 2;
		}

		bool IsCollide_1d(T max0, T min0, T max1, T min1) const
		{
			T tmp;
			if (max0 < min0)
			{
				tmp = max0;
				max0 = min0;
				min0 = tmp;
			}
			if (max1 < min1)
			{
				tmp = max1;
				max1 = min1;
				min1 = tmp;
			}

			if (max1 < min0 || min1 > max0)
			{
				return false;
			}

			return true;
		}

		//! Returns if the rectangle collides with a line
		bool IsRectCollidedWithLine2d(const FLine2D<T>& line) const
		{
			// rough test first
			if (IsCollide_1d(Right, Left, line.End.X, line.Start.X) &&
				IsCollide_1d(Lower, Upper, line.End.Y, line.Start.Y))
			{
				// accurate test, test line across 2 lines
				FLine2D<T> l;
				l.Start.X = Left;
				l.Start.Y = Upper;
				l.End.X = Right;
				l.End.Y = Lower;

				if (l.IsIntersectWithLine2d(line))
				{
					return true;
				}

				l.Start.X = Right;
				l.Start.Y = Upper;
				l.End.X = Left;
				l.End.Y = Lower;
				if (l.IsIntersectWithLine2d(line))
				{
					return true;
				}
			}
			return  false;
		}

		//! Clips this rectangle with another one.
		void ClipAgainst(const FRect2D<T>& other)
		{
			if (other.Right < Right)
				Right = other.Right;
			if (other.Lower < Lower)
				Lower = other.Lower;

			if (other.Left > Left)
				Left = other.Left;
			if (other.Upper > Upper)
				Upper = other.Upper;

			// correct possible invalid rect resulting from clipping
			if (Upper > Lower)
				Upper = Lower;
			if (Left > Right)
				Left = Right;
		}

		//! Moves this rectangle to fit inside another one.
		//! \return: returns true on success, false if not possible
		bool ConstrainTo(const FRect2D<T>& other)
		{
			if (other.GetWidth() < GetWidth() || other.GetHeight() < GetHeight())
				return false;

			T diff = other.Right - Right;
			if (diff < 0)
			{
				Right = Right + diff;
				Left = Left + diff;
			}

			diff = other.Lower - Lower;
			if (diff < 0)
			{
				Lower = Lower + diff;
				Upper = Upper + diff;
			}

			diff = Left - other.Left;
			if (diff < 0)
			{
				Left = Left - diff;
				Right = Right - diff;
			}

			diff = Upper - other.Upper;
			if (diff < 0)
			{
				Upper = Upper - diff;
				Lower = Lower - diff;
			}

			return true;
		}

		//! Returns width of rectangle.
		T GetWidth() const
		{
			return Right - Left;
		}

		//! Returns height of rectangle.
		T GetHeight() const
		{
			return Lower - Upper;
		}

		//! If the lower right corner of the rect is smaller then the
		//! upper left, the points are swapped.
		void Repair()
		{
			if (Right < Left)
			{
				T t = Right;
				Right = Left;
				Left = t;
			}

			if (Lower < Upper)
			{
				T t = Lower;
				Lower = Upper;
				Upper = t;
			}
		}

		//! Returns if the rect is valid to draw. It could be invalid
		//! if the UpperLeftCorner is lower or more right than the
		//! LowerRightCorner, or if any dimension is 0.
		bool IsValid() const
		{
			return ((Right >= Left) &&
				(Lower >= Upper));
		}

		//! Returns the center of the rectangle
		FVec2<T> GetCenter() const
		{
			return FVec2<T>((Left + Right) / 2,
				(Upper + Lower) / 2);
		}

		//! Returns the dimensions of the rectangle
		FVec2<T> GetSize() const
		{
			return FVec2<T>(GetWidth(), GetHeight());
		}


		//! Adds a point to the rectangle, causing it to grow bigger,
		//! if point is outside of the box
		//! \param p Point to add into the box.
		void AddInternalPoint(const FVec2<T>& p)
		{
			AddInternalPoint(p.X, p.Y);
		}

		//! Adds a point to the bounding rectangle, causing it to grow bigger,
		//! if point is outside of the box.
		//! \param x X Coordinate of the point to add to this box.
		//! \param y Y Coordinate of the point to add to this box.
		void AddInternalPoint(T x, T y)
		{
			if (x > Right)
				Right = x;
			if (y > Lower)
				Lower = y;

			if (x < Left)
				Left = x;
			if (y < Upper)
				Upper = y;
		}

		//! Adds a rectangle to the bounding rectangle, causing it to grow bigger,
		void AddInternalRect(const FRect2D<T>& rc)
		{
			AddInternalPoint(rc.Left, rc.Upper);
			AddInternalPoint(rc.Right, rc.Lower);
		}

		void Move(T x, T y)
		{
			Left += x;
			Right += x;
			Upper += y;
			Lower += y;
		}

		void Move(const FVec2<T>& off)
		{
			Left += off.X;
			Right += off.X;
			Upper += off.Y;
			Lower += off.Y;
		}

		void Scale(T w, T h)
		{
			Left = Left * w;
			Upper = Upper * h;
			Right = Right * w;
			Lower = Lower * h;
		}

		void ScaleFromCenter(float w, float h)
		{
			FVec2<T> center = GetCenter();
			T w_h = (T)(GetWidth() * w / 2);
			T h_h = (T)(GetHeight() * h / 2);
			Left = center.X - w_h;
			Right = center.X + w_h;
			Upper = center.Y - h_h;
			Lower = center.Y + h_h;
		}


		T Upper, Left;
		T Lower, Right;
	};

	typedef FRect2D<float32> FRect;
	typedef FRect2D<int32> FRecti;
}
