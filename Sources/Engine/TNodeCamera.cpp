/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "TNodeCamera.h"

namespace tix
{
	TNodeCamera::TNodeCamera(TNode* parent)
		: TNode(TNodeCamera::NODE_TYPE, parent)
		, CameraFlags(ECAMF_MAT_PROJECTION_DIRTY | ECAMF_MAT_VIEW_DIRTY)
		, UpVector(0.0f, 0.0f, 1.0f)
		, ZNear(1.0f)
		, ZFar(3000.0f)
		, Fovy(PI / 4.0f)
		, Aspect(1280.f / 720.f)
	{
		SetPosition(FFloat3(-1, 2, 1.0));
		SetTarget(FFloat3(0, 0, 0.25f));
	}

	TNodeCamera::~TNodeCamera()
	{
	}

	void TNodeCamera::SetPosition(const FFloat3& pos)
	{
		TNode::SetPosition(pos);
		CameraFlags |= ECAMF_MAT_VIEW_DIRTY;
	}

	void TNodeCamera::UpdateAllTransformation()
	{
		TNode::UpdateAllTransformation();
		CameraFlags &= ~ECAMF_MAT_UPDATED;
		if ((CameraFlags & ECAMF_MAT_PROJECTION_DIRTY) != 0)
		{
			RecalculateProjectionMatrix();
			CameraFlags &= ~ECAMF_MAT_PROJECTION_DIRTY;
			CameraFlags	|= ECAMF_MAT_PROJ_UPDATED;
		}
		if ((CameraFlags & ECAMF_MAT_VIEW_DIRTY) != 0)
		{
			RecalculateViewMatrix();
			CameraFlags &= ~ECAMF_MAT_VIEW_DIRTY;
			CameraFlags |= ECAMF_MAT_VIEW_UPDATED;
		}
		//if ((CameraFlags & ECAMF_MAT_VP_DIRTY) != 0)
		//{
		//	ViewArea.Matrices[ETS_VP] = ViewArea.Matrices[ETS_PROJECTION] * ViewArea.Matrices[ETS_VIEW];
		//	CameraFlags &= ~ECAMF_MAT_VP_DIRTY;
		//	CameraFlags |= ECAMF_MAT_VP_UPDATED;
		//}

		if ((CameraFlags & ECAMF_MAT_UPDATED) != 0)
		{
			// Notify render thread
			FViewProjectionInfo Info;
			Info.MatProj = ViewArea.Matrices[ETS_PROJECTION];
			Info.MatView = ViewArea.Matrices[ETS_VIEW];
			Info.CamPos = GetAbsolutePosition();
			Info.CamDir = CamDir;
			Info.HorVector = HorVector;
			Info.VerVector = VerVector;
			Info.Fov = Fovy;

			ENQUEUE_RENDER_COMMAND(UpdateViewProjectionRenderThread)(
				[Info]()
				{
					FScene* Scene = FRenderThread::Get()->GetRenderScene();
					Scene->SetViewProjection(Info);
				});
		}
	}

	//! Sets the projection matrix of the camera. The FMat4 class has some methods
	//! to build a projection matrix. e.g: FMat4::buildProjectionMatrixPerspectiveFov
	//! \param projection The newly projection matrix of the camera.
	void TNodeCamera::SetProjectionMatrix(const FMat4& projection, bool isOrthogonal)
	{
		//IsOrthogonal = isOrthogonal;
		ViewArea.Matrices[ETS_PROJECTION] = projection;
		ViewArea.SetTransformState(ETS_PROJECTION);
	}

	//! Gets the current projection matrix of the camera
	//! \return Returns the current projection matrix of the camera.
	const FMat4& TNodeCamera::GetProjectionMatrix() const
	{
		return ViewArea.Matrices[ETS_PROJECTION];
	}

	//! Gets the current view matrix of the camera
	//! \return Returns the current view matrix of the camera.
	const FMat4& TNodeCamera::GetViewMatrix() const
	{
		return ViewArea.Matrices[ETS_VIEW];
	}

	//! Sets the look at tarGet of the camera
	//! \param pos Look at tarGet of the camera.
	void TNodeCamera::SetTarget(const FFloat3& pos)
	{
		Target = pos;
		CameraFlags |= ECAMF_MAT_VIEW_DIRTY;
	}

	//! sets the Rotator of camera in radian
	//! \param rotator include Pitch Yaw Roll in Radian.
	void TNodeCamera::SetRotator(const FFloat3& rotator)
	{
		Rotator = rotator;
		CameraFlags |= ECAMF_MAT_VIEW_DIRTY;
	}

	//! Gets the current look at tarGet of the camera
	//! \return Returns the current look at tarGet of the camera
	const FFloat3& TNodeCamera::GetTarget() const
	{
		return Target;
	}

	//! Sets the up vector of the camera
	//! \param pos New upvector of the camera.
	void TNodeCamera::SetUpVector(const FFloat3& pos)
	{
		UpVector = pos;
	}

	//! Gets the up vector of the camera.
	//! \return Returns the up vector of the camera.
	const FFloat3& TNodeCamera::GetUpVector() const
	{
		return UpVector;
	}

	float32 TNodeCamera::GetNearValue() const 
	{
		return ZNear;
	}

	float32 TNodeCamera::GetFarValue() const 
	{
		return ZFar;
	}

	float32 TNodeCamera::GetAspectRatio() const 
	{
		return Aspect;
	}

	float32 TNodeCamera::GetFOV() const 
	{
		return Fovy;
	}

	void TNodeCamera::SetNearValue(float32 f)
	{
		ZNear = f;
		CameraFlags |= ECAMF_MAT_PROJECTION_DIRTY;
	}

	void TNodeCamera::SetFarValue(float32 f)
	{
		ZFar = f;
		CameraFlags |= ECAMF_MAT_PROJECTION_DIRTY;
	}

	void TNodeCamera::SetAspectRatio(float32 f)
	{
		Aspect = f;
		CameraFlags |= ECAMF_MAT_PROJECTION_DIRTY;
	}

	void TNodeCamera::SetFOV(float32 f)
	{
		Fovy = f;
		CameraFlags |= ECAMF_MAT_PROJECTION_DIRTY;
	}

	void TNodeCamera::SetFOVX(float32 f)
	{
		Fovy = f / Aspect;
		CameraFlags |= ECAMF_MAT_PROJECTION_DIRTY;
	}

	void TNodeCamera::RecalculateViewMatrix()
	{
		FFloat3 pos = GetAbsolutePosition();
		CamDir = Target - pos;
		CamDir.Normalize();

		FFloat3 up = UpVector;
		up.Normalize();

		float32 dp = CamDir.Dot(up);

		if (TMath::Equals(fabs(dp), 1.f))
		{
			up.X = up.X + 0.5f;
		}

		ViewArea.Matrices[ETS_VIEW] = BuildCameraLookAtMatrix(pos, Target, up);;
		ViewArea.SetTransformState(ETS_VIEW);

		RecalculateViewArea();

		// calculate hor and ver vector for billboard
		HorVector	= CamDir.Cross(UpVector);
		HorVector.Normalize();
		VerVector	= HorVector.Cross(CamDir);
		VerVector.Normalize();
	}

	void TNodeCamera::RecalculateProjectionMatrix()
	{
		ViewArea.Matrices[ETS_PROJECTION] = BuildProjectionMatrixPerspectiveFov(Fovy, Aspect, ZNear, ZFar);
		//const FMat4& mat	= ViewArea.Matrices[ETS_PROJECTION];

		ViewArea.SetTransformState(ETS_PROJECTION);
	}

	//! returns the view frustum. needed sometimes by bsp or lod render nodes.
	const SViewFrustum* TNodeCamera::GetViewFrustum() const
	{
		return &ViewArea;
	}

	void TNodeCamera::RecalculateViewArea()
	{
		ViewArea.CameraPosition = GetAbsolutePosition();
		ViewArea.SetFrom(ViewArea.Matrices[SViewFrustum::ETS_VIEW_PROJECTION_3]);
	}

	void TNodeCamera::GetRayFrom2DPoint(const FInt2& pos, FLine3&ray, float length)
	{
		TI_ASSERT(0);
		//FFloat3 orig,dir;
		//const FMat4& matProj	= ViewArea.Matrices[ETS_PROJECTION];
		//const FMat4& matView	= ViewArea.Matrices[ETS_VIEW];

		//FMat4 m;
		//matView.getInverse(m);
		//const FRecti &vp			= TiEngine::Get()->GetRenderer()->GetActiveRenderer()->GetViewport();

		//// Compute the vector of the Pick ray in screen space
		//FFloat3 v;
		//v.X = ( ( ( 2.0f * (pos.X - vp.Left) ) / vp.getWidth() ) - 1 ) / matProj(0, 0);
		//v.Y = -( ( ( 2.0f * (pos.Y - vp.Upper) ) / vp.getHeight() ) - 1 ) / matProj(1, 1);
		//v.Z = 1.0f;

		//FFloat3 pStart, pEnd;
		//m.TransformVect(pStart, v * 1.0f);
		//m.TransformVect(pEnd, v * length);
		//ray.start = pStart;
		//ray.end = pEnd;
	}

	void TNodeCamera::GetRayFrom2DPoint(const FFloat2& pos, FLine3&ray, float length)
	{
		TI_ASSERT(0);
		//FFloat3 orig,dir;
		//const FMat4& matProj	= ViewArea.Matrices[ETS_PROJECTION];
		//const FMat4& matView	= ViewArea.Matrices[ETS_VIEW];

		//FMat4 m;
		//matView.getInverse(m);
		//const FRecti &vp			= TiEngine::Get()->GetRenderer()->GetActiveRenderer()->GetViewport();

		//// Compute the vector of the Pick ray in screen space
		//FFloat3 v;
		//v.X = ( ( ( 2.0f * (pos.X - vp.Left) ) / vp.getWidth() ) - 1 ) / matProj(0, 0);
		//v.Y = -( ( ( 2.0f * (pos.Y - vp.Upper) ) / vp.getHeight() ) - 1 ) / matProj(1, 1);
		//v.Z = 1.0f;

		//FFloat3 pStart, pEnd;
		//m.TransformVect(pStart, v * 1.0f);
		//m.TransformVect(pEnd, v * length);
		//ray.start = pStart;
		//ray.end = pEnd;
	}

	void TNodeCamera::GetRayFrom2DPointWithViewport(const FRect& vp, const FFloat2& pos, FLine3&ray, float length)
	{
		FFloat3 orig,dir;
		const FMat4& matProj	= ViewArea.Matrices[ETS_PROJECTION];
		const FMat4& matView	= ViewArea.Matrices[ETS_VIEW];

		FMat4 m;
		matView.GetInverse(m);

		// Compute the vector of the Pick ray in screen space
		FFloat3 v;
		v.X = ( ( ( 2.0f * (pos.X - vp.Left) ) / vp.GetWidth() ) - 1 ) / matProj(0, 0);
		v.Y = -( ( ( 2.0f * (pos.Y - vp.Upper) ) / vp.GetHeight() ) - 1 ) / matProj(1, 1);
		v.Z = 1.0f;

		FFloat3 pStart, pEnd;
		m.TransformVect(pStart, v * 1.0f);
		m.TransformVect(pEnd, v * length);
		ray.Start = pStart;
		ray.End = pEnd;
	}

	FFloat2 TNodeCamera::Convert3Dto2D(const FFloat3& pos)
	{
		TI_ASSERT(0);
		FFloat2 _pos_2d;
		//float _proj_pos[4];

		//const FMat4& matVP	= ViewArea.Matrices[ETS_VP];
		//matVP.TransformVect(_proj_pos, pos);

		//_proj_pos[0]			/= _proj_pos[3];
		//_proj_pos[1]			/= _proj_pos[3];

		//const FRecti& vp			= TiEngine::Get()->GetRenderer()->GetActiveRenderer()->GetViewport();

		//_pos_2d.X				= (_proj_pos[0] * 0.5f + 0.5f) * vp.getWidth();
		//_pos_2d.Y				= (0.5f - _proj_pos[1] * 0.5f) * vp.getHeight();

		return _pos_2d;
	}
}
