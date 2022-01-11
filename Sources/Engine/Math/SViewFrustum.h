/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	struct SViewFrustum
	{
		enum VFPLANES
		{
			//! Far plane of the frustum. That is the plane farest away from the eye.
			VF_FAR_PLANE = 0,
			//! Near plane of the frustum. That is the plane nearest to the eye.
			VF_NEAR_PLANE,
			//! Left plane of the frustum.
			VF_LEFT_PLANE,
			//! Right plane of the frustum.
			VF_RIGHT_PLANE,
			//! Bottom plane of the frustum.
			VF_BOTTOM_PLANE,
			//! Top plane of the frustum.
			VF_TOP_PLANE,

			//! Amount of planes enclosing the view frustum. Should be 6.
			VF_PLANE_COUNT
		};

		//! Hold a copy of important transform matrices
		enum E_TRANSFORMATION_STATE_3
		{
			ETS_VIEW_PROJECTION_3 = ETS_COUNT,
			//ETS_VIEW_MODEL_INVERSE_3,
			//ETS_CURRENT_3,
			ETS_COUNT_3
		};

		//! Default Constructor
		SViewFrustum() {}

		//! This constructor creates a view frustum based on a projection and/or
		//! view matrix.
		SViewFrustum(const FMat4& mat);

		//! This constructor creates a view frustum based on a projection and/or
		//! view matrix.
		inline void SetFrom(const FMat4& mat);

		//! transforms the frustum by the matrix
		/** \param mat Matrix by which the view frustum is transformed.*/
		void Transform(const FMat4& mat);

		//! returns the point which is on the far left upper corner inside the the
		//! view frustum.
		FFloat3 GetFarLeftUp() const;

		//! returns the point which is on the far left bottom corner inside the the
		//! view frustum.
		FFloat3 GetFarLeftDown() const;

		//! returns the point which is on the far right top corner inside the the
		//! view frustum.
		FFloat3 GetFarRightUp() const;

		//! returns the point which is on the far right bottom corner inside the the
		//! view frustum.
		FFloat3 GetFarRightDown() const;

		//! returns the point which is on the far left upper corner inside the the
		//! view frustum.
		FFloat3 GetNearLeftUp() const;

		//! returns the point which is on the far left bottom corner inside the the
		//! view frustum.
		FFloat3 GetNearLeftDown() const;

		//! returns the point which is on the far right top corner inside the the
		//! view frustum.
		FFloat3 GetNearRightUp() const;

		//! returns the point which is on the far right bottom corner inside the the
		//! view frustum.
		FFloat3 GetNearRightDown() const;

		//! returns a bounding box enclosing the whole view frustum
		const FAABBox<float32> &GetBoundingBox() const;

		//! recalculates the bounding box member based on the planes
		inline void RecalculateBoundingBox();

		//! update the given state's matrix
		void SetTransformState( E_TRANSFORMATION_STATE state);

		//!
		bool TestPlane(uint32 i, const FBox& bbox) const;

		//!
		bool IntersectsWithoutBoxTest(const FBox& bbox) const;

		//!
		bool Intersects3(const FBox& bbox) const;

		//!
		bool IntersectsWithoutBoxTest3(const FBox& bbox) const;

		//!
		E_CULLING_RESULT IntersectsExWithoutBoxTest3(const FBox& bbox) const;

		//!
		bool Intersects(const FBox& bbox) const;

		//!
		bool TestInsidePlane(uint32 i, const FBox& bbox) const;

		//!
		bool IsFullInsideWithoutBoxTest(const FBox& bbox) const;

		//!
		bool IsFullInside(const FBox& bbox) const;

		//!
		E_CULLING_RESULT IntersectsExWithoutBoxTest(const FBox& bbox) const;

		//!
		E_CULLING_RESULT IntersectsEx(const FBox& bbox) const;

		//! the position of the camera
		FFloat3 CameraPosition;

		//! all planes enclosing the view frustum.
		FPlane Planes[VF_PLANE_COUNT];

		//! bounding box around the view frustum
		FBox BoundingBox;

		//! Hold a copy of important transform matrices
		FMat4 Matrices[ETS_COUNT_3];
	};


	inline SViewFrustum::SViewFrustum(const FMat4& mat)
	{
		SetFrom ( mat );
	}


	inline void SViewFrustum::Transform(const FMat4& mat)
	{
		for (uint32 i=0; i<VF_PLANE_COUNT; ++i)
			mat.TransformPlane(Planes[i]);

		mat.TransformVect(CameraPosition);
		RecalculateBoundingBox();
	}


	inline FFloat3 SViewFrustum::GetFarLeftUp() const
	{
		FFloat3 p;
		Planes[SViewFrustum::VF_FAR_PLANE].GetIntersectionWithPlanes(
			Planes[SViewFrustum::VF_TOP_PLANE],
			Planes[SViewFrustum::VF_LEFT_PLANE], p);

		return p;
	}

	inline FFloat3 SViewFrustum::GetFarLeftDown() const
	{
		FFloat3 p;
		Planes[SViewFrustum::VF_FAR_PLANE].GetIntersectionWithPlanes(
			Planes[SViewFrustum::VF_BOTTOM_PLANE],
			Planes[SViewFrustum::VF_LEFT_PLANE], p);

		return p;
	}

	inline FFloat3 SViewFrustum::GetFarRightUp() const
	{
		FFloat3 p;
		Planes[SViewFrustum::VF_FAR_PLANE].GetIntersectionWithPlanes(
			Planes[SViewFrustum::VF_TOP_PLANE],
			Planes[SViewFrustum::VF_RIGHT_PLANE], p);

		return p;
	}

	inline FFloat3 SViewFrustum::GetFarRightDown() const
	{
		FFloat3 p;
		Planes[SViewFrustum::VF_FAR_PLANE].GetIntersectionWithPlanes(
			Planes[SViewFrustum::VF_BOTTOM_PLANE],
			Planes[SViewFrustum::VF_RIGHT_PLANE], p);

		return p;
	}

	inline FFloat3 SViewFrustum::GetNearLeftUp() const
	{
		FFloat3 p;
		Planes[SViewFrustum::VF_NEAR_PLANE].GetIntersectionWithPlanes(
			Planes[SViewFrustum::VF_TOP_PLANE],
			Planes[SViewFrustum::VF_LEFT_PLANE], p);

		return p;
	}

	inline FFloat3 SViewFrustum::GetNearLeftDown() const
	{
		FFloat3 p;
		Planes[SViewFrustum::VF_NEAR_PLANE].GetIntersectionWithPlanes(
			Planes[SViewFrustum::VF_BOTTOM_PLANE],
			Planes[SViewFrustum::VF_LEFT_PLANE], p);

		return p;
	}

	inline FFloat3 SViewFrustum::GetNearRightUp() const
	{
		FFloat3 p;
		Planes[SViewFrustum::VF_NEAR_PLANE].GetIntersectionWithPlanes(
			Planes[SViewFrustum::VF_TOP_PLANE],
			Planes[SViewFrustum::VF_RIGHT_PLANE], p);

		return p;
	}

	inline FFloat3 SViewFrustum::GetNearRightDown() const
	{
		FFloat3 p;
		Planes[SViewFrustum::VF_NEAR_PLANE].GetIntersectionWithPlanes(
			Planes[SViewFrustum::VF_BOTTOM_PLANE],
			Planes[SViewFrustum::VF_RIGHT_PLANE], p);

		return p;
	}



	inline const FAABBox<float32> &SViewFrustum::GetBoundingBox() const
	{
		return BoundingBox;
	}

	inline void SViewFrustum::RecalculateBoundingBox()
	{
		BoundingBox.Reset ( CameraPosition );

		BoundingBox.AddInternalPoint(GetFarLeftUp());
		BoundingBox.AddInternalPoint(GetFarRightUp());
		BoundingBox.AddInternalPoint(GetFarLeftDown());
		BoundingBox.AddInternalPoint(GetFarRightDown());
	}

	/*
	//! This constructor creates a view frustum based on a projection
	//! and/or view matrix.
	inline void SViewFrustum::setFrom(const FMat4& mat)
	{
		// left clipping plane
		Planes[SViewFrustum::VF_LEFT_PLANE].Normal.X = -(mat(0,3) + mat(0,0));
		Planes[SViewFrustum::VF_LEFT_PLANE].Normal.Y = -(mat(1,3) + mat(1,0));
		Planes[SViewFrustum::VF_LEFT_PLANE].Normal.Z = -(mat(2,3) + mat(2,0));
		Planes[SViewFrustum::VF_LEFT_PLANE].D = -(mat(3,3) + mat(3,0));
		
		// right clipping plane
		Planes[SViewFrustum::VF_RIGHT_PLANE].Normal.X = -(mat(0,3) - mat(0,0));
		Planes[SViewFrustum::VF_RIGHT_PLANE].Normal.Y = -(mat(1,3) - mat(1,0));
		Planes[SViewFrustum::VF_RIGHT_PLANE].Normal.Z = -(mat(2,3) - mat(2,0));
		Planes[SViewFrustum::VF_RIGHT_PLANE].D =        -(mat(3,3) - mat(3,0));

		// top clipping plane
		Planes[SViewFrustum::VF_TOP_PLANE].Normal.X = -(mat(0,3) - mat(0,1));
		Planes[SViewFrustum::VF_TOP_PLANE].Normal.Y = -(mat(1,3) - mat(1,1));
		Planes[SViewFrustum::VF_TOP_PLANE].Normal.Z = -(mat(2,3) - mat(2,1));
		Planes[SViewFrustum::VF_TOP_PLANE].D =        -(mat(3,3) - mat(3,1));

		// bottom clipping plane
		Planes[SViewFrustum::VF_BOTTOM_PLANE].Normal.X = -(mat(0,3) + mat(0,1));
		Planes[SViewFrustum::VF_BOTTOM_PLANE].Normal.Y = -(mat(1,3) + mat(1,1));
		Planes[SViewFrustum::VF_BOTTOM_PLANE].Normal.Z = -(mat(2,3) + mat(2,1));
		Planes[SViewFrustum::VF_BOTTOM_PLANE].D =        -(mat(3,3) + mat(3,1));

		// near clipping plane
		Planes[SViewFrustum::VF_NEAR_PLANE].Normal.X = -mat(0,2);
		Planes[SViewFrustum::VF_NEAR_PLANE].Normal.Y = -mat(1,2);
		Planes[SViewFrustum::VF_NEAR_PLANE].Normal.Z = -mat(2,2);
		Planes[SViewFrustum::VF_NEAR_PLANE].D =        -mat(3,2);

		// far clipping plane
		Planes[SViewFrustum::VF_FAR_PLANE].Normal.X = -(mat(0,3) - mat(0,2));
		Planes[SViewFrustum::VF_FAR_PLANE].Normal.Y = -(mat(1,3) - mat(1,2));
		Planes[SViewFrustum::VF_FAR_PLANE].Normal.Z = -(mat(2,3) - mat(2,2));
		Planes[SViewFrustum::VF_FAR_PLANE].D =        -(mat(3,3) - mat(3,2));
		// normalize normals

		for (s32 i=0; i<6; ++i)
		{
			const float32 len = reciprocal_squareroot(
				Planes[i].Normal.GetLengthSQ() );
			Planes[i].Normal *= len;
			Planes[i].D *= len;
		}

		// make bounding box
		recalculateBoundingBox();
	}
	*/

	inline void SViewFrustum::SetFrom(const FMat4& mat)
	{
		// left clipping plane
		Planes[VF_LEFT_PLANE].Normal.X = mat[3 ] + mat[0];
		Planes[VF_LEFT_PLANE].Normal.Y = mat[7 ] + mat[4];
		Planes[VF_LEFT_PLANE].Normal.Z = mat[11] + mat[8];
		Planes[VF_LEFT_PLANE].D =        mat[15] + mat[12];

		// right clipping plane
		Planes[VF_RIGHT_PLANE].Normal.X = mat[3 ] - mat[0];
		Planes[VF_RIGHT_PLANE].Normal.Y = mat[7 ] - mat[4];
		Planes[VF_RIGHT_PLANE].Normal.Z = mat[11] - mat[8];
		Planes[VF_RIGHT_PLANE].D =        mat[15] - mat[12];

		// top clipping plane
		Planes[VF_TOP_PLANE].Normal.X = mat[3 ] - mat[1];
		Planes[VF_TOP_PLANE].Normal.Y = mat[7 ] - mat[5];
		Planes[VF_TOP_PLANE].Normal.Z = mat[11] - mat[9];
		Planes[VF_TOP_PLANE].D =        mat[15] - mat[13];

		// bottom clipping plane
		Planes[VF_BOTTOM_PLANE].Normal.X = mat[3 ] + mat[1];
		Planes[VF_BOTTOM_PLANE].Normal.Y = mat[7 ] + mat[5];
		Planes[VF_BOTTOM_PLANE].Normal.Z = mat[11] + mat[9];
		Planes[VF_BOTTOM_PLANE].D =        mat[15] + mat[13];

		// far clipping plane
		Planes[VF_FAR_PLANE].Normal.X = mat[3 ] - mat[2];
		Planes[VF_FAR_PLANE].Normal.Y = mat[7 ] - mat[6];
		Planes[VF_FAR_PLANE].Normal.Z = mat[11] - mat[10];
		Planes[VF_FAR_PLANE].D =        mat[15] - mat[14];

		// near clipping plane
		Planes[VF_NEAR_PLANE].Normal.X = mat[2];
		Planes[VF_NEAR_PLANE].Normal.Y = mat[6];
		Planes[VF_NEAR_PLANE].Normal.Z = mat[10];
		Planes[VF_NEAR_PLANE].D =        mat[14];

		// normalize normals
		uint32 i;
		for ( i=0; i != VF_PLANE_COUNT; ++i)
		{
			const float32 len = -TMath::ReciprocalSquareroot(
				Planes[i].Normal.GetLengthSQ());
			Planes[i].Normal *= len;
			Planes[i].D *= len;
		}

		// make bounding box
		RecalculateBoundingBox();
	}

	inline void SViewFrustum::SetTransformState(E_TRANSFORMATION_STATE state)
	{
		switch ( state )
		{
			case ETS_VIEW:
				Matrices[ETS_VIEW_PROJECTION_3].Setbyproduct_nocheck(
					Matrices[ETS_PROJECTION],
					Matrices[ETS_VIEW]);
				//Matrices[ETS_VIEW_MODEL_INVERSE_3] = Matrices[ETS_VIEW];
				//Matrices[ETS_VIEW_MODEL_INVERSE_3].makeInverse();
				break;

			case ETS_WORLD:
				//Matrices[ETS_CURRENT_3].setbyproduct(
				//	Matrices[ETS_VIEW_PROJECTION_3 ],
				//	Matrices[ETS_WORLD]);
				break;
			default:
				break;
		}
	}

	inline bool SViewFrustum::TestPlane(uint32 i, const FBox& bbox) const
	{
		//http://www.lighthouse3d.com/tutorials/view-frustum-culling/geometric-approach-testing-boxes-ii/
		// get the "nearest" corner to the frustum along Planes[i]'s normal
		FFloat3
			p(Planes[i].Normal.X >= 0 ? bbox.Min.X : bbox.Max.X,
			  Planes[i].Normal.Y >= 0 ? bbox.Min.Y : bbox.Max.Y,
			  Planes[i].Normal.Z >= 0 ? bbox.Min.Z : bbox.Max.Z);

		// if the nearest corner to the frustum is outside, then the bbox is
		// outside.
		if (Planes[i].GetDistanceTo(p) > 0)
		{
			return false;
		}
		return true;
	}

	inline bool SViewFrustum::IntersectsWithoutBoxTest(const FBox& bbox) const
	{
		for (uint32 i = 0; i < VF_PLANE_COUNT; ++i)
		{
			if (!TestPlane(i, bbox))
			{
				return false;
			}
		}
		return true;
	}

	inline bool SViewFrustum::Intersects(const FBox& bbox) const
	{
		if (BoundingBox.IntersectsWithBox(bbox))
		{
			return IntersectsWithoutBoxTest(bbox);
		}
		return false;
	}

	inline bool SViewFrustum::TestInsidePlane(uint32 i, const FBox& bbox) const
	{
		// get the "farthest" corner to the frustum along Planes[i]'s normal
		FFloat3
			p(Planes[i].Normal.X >= 0 ? bbox.Max.X : bbox.Min.X,
			  Planes[i].Normal.Y >= 0 ? bbox.Max.Y : bbox.Min.Y,
			  Planes[i].Normal.Z >= 0 ? bbox.Max.Z : bbox.Min.Z);

		// if the nearest corner to the frustum is outside, then the bbox is
		// outside.
		if (Planes[i].GetDistanceTo(p) > 0)
		{
			return false;
		}
		return true;
	}

	inline bool SViewFrustum::IsFullInsideWithoutBoxTest(const FBox& bbox) const
	{
		for (uint32 i = 0; i < VF_PLANE_COUNT; ++i)
		{
			if (!TestInsidePlane(i, bbox))
			{
				return false;
			}
		}
		return true;
	}

	inline bool SViewFrustum::IsFullInside(const FBox& bbox) const
	{
		if (bbox.IsFullInside(BoundingBox))
		{
			return IsFullInsideWithoutBoxTest(bbox);
		}
		return false;
	}

	inline bool SViewFrustum::IntersectsWithoutBoxTest3(const FBox& bbox) const
	{
		if (TestPlane(VF_LEFT_PLANE, bbox)
			&& TestPlane(VF_RIGHT_PLANE, bbox)
			&& TestPlane(VF_FAR_PLANE, bbox))
		{
			return true;
		}
		return false;
	}

	inline bool SViewFrustum::Intersects3(const FBox& bbox) const
	{
		if (BoundingBox.IntersectsWithBox(bbox))
		{
			return IntersectsWithoutBoxTest3(bbox);
		}
		return false;
	}

	inline E_CULLING_RESULT SViewFrustum::IntersectsExWithoutBoxTest3(const FBox& bbox) const
	{
		E_CULLING_RESULT result = ECR_INSIDE;
		// p: "nearest" corner to the frustum along Planes[i]'s normal
		// n: farthest
		FFloat3 p, n;
		static const VFPLANES planes[] = {VF_FAR_PLANE, VF_LEFT_PLANE, VF_RIGHT_PLANE};
		for (int j = 0; j < 3; ++j)
		{
			const VFPLANES i = planes[j];
			if (Planes[i].Normal.X >= 0)
			{
				p.X = bbox.Min.X;
				n.X = bbox.Max.X;
			}
			else
			{
				p.X = bbox.Max.X;
				n.X = bbox.Min.X;
			}

			if (Planes[i].Normal.Y >= 0)
			{
				p.Y = bbox.Min.Y;
				n.Y = bbox.Max.Y;
			}
			else
			{
				p.Y = bbox.Max.Y;
				n.Y = bbox.Min.Y;
			}

			if (Planes[i].Normal.Z >= 0)
			{
				p.Z = bbox.Min.Z;
				n.Z = bbox.Max.Z;
			}
			else
			{
				p.Z = bbox.Max.Z;
				n.Z = bbox.Min.Z;
			}

			// if the nearest point to the frustum is outside, then the bbox is
			// outside.
			if (Planes[i].GetDistanceTo(p) > 0)
			{
				return ECR_OUTSIDE;
			}
			// here, p is inside, thus if n is outside, we intersect
			if (Planes[i].GetDistanceTo(n) > 0)
			{
				result = ECR_INTERSECT;
			}
		}
		return result;
	}

	inline E_CULLING_RESULT SViewFrustum::IntersectsExWithoutBoxTest(const FBox& bbox) const
	{
		E_CULLING_RESULT result = ECR_INSIDE;
		// p: "nearest" corner to the frustum along Planes[i]'s normal
		// n: farthest
		FFloat3 p, n;
		for (uint32 i = 0; i < VF_PLANE_COUNT; ++i)
		{
			if (Planes[i].Normal.X >= 0)
			{
				p.X = bbox.Min.X;
				n.X = bbox.Max.X;
			}
			else
			{
				p.X = bbox.Max.X;
				n.X = bbox.Min.X;
			}

			if (Planes[i].Normal.Y >= 0)
			{
				p.Y = bbox.Min.Y;
				n.Y = bbox.Max.Y;
			}
			else
			{
				p.Y = bbox.Max.Y;
				n.Y = bbox.Min.Y;
			}

			if (Planes[i].Normal.Z >= 0)
			{
				p.Z = bbox.Min.Z;
				n.Z = bbox.Max.Z;
			}
			else
			{
				p.Z = bbox.Max.Z;
				n.Z = bbox.Min.Z;
			}

			// if the nearest point to the frustum is outside, then the bbox is
			// outside.
			if (Planes[i].GetDistanceTo(p) > 0)
			{
				return ECR_OUTSIDE;
			}
			// here, p is inside, thus if n is outside, we intersect
			if (Planes[i].GetDistanceTo(n) > 0)
			{
				result = ECR_INTERSECT;
			}
		}
		return result;
	}

	inline E_CULLING_RESULT SViewFrustum::IntersectsEx(const FBox& bbox) const
	{
		if (BoundingBox.IntersectsWithBox(bbox))
		{
			return IntersectsExWithoutBoxTest(bbox);
		}
		return ECR_OUTSIDE;
	}

}
