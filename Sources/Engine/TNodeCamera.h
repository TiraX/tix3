/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	struct FViewProjectionInfo
	{
		FMat4 MatView;
		FMat4 MatProj;
		FFloat3 CamPos;
		FFloat3 CamDir;
		FFloat3 HorVector;
		FFloat3 VerVector;
		float Fov;
	};

	class TNodeCamera : public TNode
	{
		DECLARE_NODE_WITH_CONSTRUCTOR(Camera);
	public:
		enum E_CAMERA_FLAG
		{
			ECAMF_MAT_VIEW_DIRTY		= 1 << 0,
			ECAMF_MAT_PROJECTION_DIRTY	= 1 << 1,
			ECAMF_MAT_VP_DIRTY			= (ECAMF_MAT_VIEW_DIRTY | ECAMF_MAT_PROJECTION_DIRTY),

			ECAMF_MAT_VIEW_UPDATED		= 1 << 2,
			ECAMF_MAT_PROJ_UPDATED		= 1 << 3,
			ECAMF_MAT_VP_UPDATED		= 1 << 4,
			ECAMF_MAT_UPDATED			= (ECAMF_MAT_VIEW_UPDATED | ECAMF_MAT_VP_UPDATED | ECAMF_MAT_PROJ_UPDATED),
		};
	public:
		virtual ~TNodeCamera();

		virtual void SetPosition(const FFloat3& pos) override;
		virtual void UpdateAllTransformation() override;

		//! Sets the projection matrix of the camera.
		/** The FMat4 class has some methods
			to build a projection matrix. e.g: FMat4::buildProjectionMatrixPerspectiveFov.
			Note that the matrix will only stay as set by this method until one of
			the following Methods are called: setNearValue, setFarValue, setAspectRatio, setFOV.
			\param projection The newly projection matrix of the camera.
			\param isOrthogonal Set this to true if the matrix is an orthogonal one (e.g.
			from FMat4::buildProjectionMatrixOrtho(). */
		virtual void SetProjectionMatrix(const FMat4& projection, bool isOrthogonal = false);

		//! Gets the current projection matrix of the camera
		//! \return Returns the current projection matrix of the camera.
		virtual const FMat4& GetProjectionMatrix() const;

		//! Gets the current view matrix of the camera
		//! \return Returns the current view matrix of the camera.
		virtual const FMat4& GetViewMatrix() const;

		//! sets the look at target of the camera
		//! \param pos Look at target of the camera.
		virtual void SetTarget(const FFloat3& pos);

		//! sets the Rotator of camera in radian
		//! \param rotator include Pitch Yaw Roll in Radian.
		virtual void SetRotator(const FFloat3& rotator);

		//! Gets the current look at target of the camera
		//! \return Returns the current look at target of the camera
		virtual const FFloat3& GetTarget() const;

		//! Sets the up vector of the camera.
		//! \param pos New upvector of the camera.
		virtual void SetUpVector(const FFloat3& pos);

		//! Gets the up vector of the camera.
		//! \return Returns the up vector of the camera.
		virtual const FFloat3& GetUpVector() const;

		//! Gets distance from the camera to the near plane.
		//! \return Value of the near plane of the camera.
		virtual float32 GetNearValue() const;

		//! Gets the distance from the camera to the far plane.
		//! \return Value of the far plane of the camera.
		virtual float32 GetFarValue() const;

		//! Get the aspect ratio of the camera.
		//! \return The aspect ratio of the camera.
		virtual float32 GetAspectRatio() const;

		//! Gets the field of view of the camera.
		//! \return Field of view of the camera
		virtual float32 GetFOV() const;

		virtual const FFloat3& GetCamDir()
		{
			return	CamDir;
		}
		virtual const FFloat3& GetHorVector()
		{
			return	HorVector;
		}
		virtual const FFloat3& GetVerVector()
		{
			return	VerVector;
		}

		//! Sets the value of the near clipping plane. (default: 1.0f)
		virtual void SetNearValue(float32 zn);

		//! Sets the value of the far clipping plane (default: 2000.0f)
		virtual void SetFarValue(float32 zf);

		//! Sets the aspect ratio (default: 4.0f / 3.0f)
		virtual void SetAspectRatio(float32 aspect);

		//! Sets the field of view in vertical (Default: PI / 3.5f)
		virtual void SetFOV(float32 fovy);

		//! Sets the field of view in X
		virtual void SetFOVX(float32 fovy);

		//! Returns the view area. Sometimes needed by bsp or lod render nodes.
		virtual const SViewFrustum* GetViewFrustum() const;

		//! Get a ray from the screen coord.
		virtual void GetRayFrom2DPoint(const FInt2& pos, line3df &ray, float length = 1000.f);
		virtual void GetRayFrom2DPoint(const FFloat2& pos, line3df &ray, float length = 1000.f);
		virtual void GetRayFrom2DPointWithViewport(const rectf& vp, const FFloat2& pos, line3df &ray, float length = 1000.f);

		//! Convert 3d position into 2d screen space
		TI_API FFloat2 Convert3Dto2D(const FFloat3& pos);

		const SViewFrustum&	GetFrustum()
		{
			return	ViewArea;
		}

		uint32	GetCameraFlags()
		{
			return CameraFlags;
		}
	protected:

		virtual void RecalculateProjectionMatrix();
		virtual void RecalculateViewMatrix();
		virtual void RecalculateViewArea();

	protected:
		uint32 CameraFlags;

		FFloat3 Target;
		FFloat3 UpVector;
		FFloat3 Rotator;

		FFloat3 CamDir;
		FFloat3 HorVector;
		FFloat3 VerVector;

		float32 Fovy;	// Field of view, in radians. 

		float32 Aspect;	// Aspect ratio. 
		float32 ZNear;	// value of the near view-plane. 
		float32 ZFar;	// Z-value of the far view-plane.

		SViewFrustum ViewArea;
	};

} // end namespace tix

