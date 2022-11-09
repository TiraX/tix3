/*
TiX Engine v3.0 Copyright (C) 2022~2025
By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	class TNodeCameraNav : public TNodeCamera, public TEventHandler
	{
		DECLARE_NODE_WITH_CONSTRUCTOR(CameraNav);
	public:
		virtual ~TNodeCameraNav();

		virtual void Tick(float Dt);
		virtual bool OnEvent(const TEvent& e);
		
		float GetMoveSpeed() const
		{
			return MoveSpeed;
		}
		float GetDollySpeed() const
		{
			return DollySpeed;
		}
		void SetDollySpeed(float s)
		{
			DollySpeed	= s;
		}
		void SetModeSpeed(float s)
		{
			MoveSpeed	= s;
		}

	protected:
		void UpdateCameraAction();

	protected:
		enum E_CAM_ACTION
		{
			ECA_NONE,
			ECA_DOLLY,
			ECA_ROLL,
			ECA_MOVE,
		};

		E_CAM_ACTION Action;
		FFloat3 OldTarget;
		FFloat3 OldPosition;

		float RotateSpeed;

		float MoveSpeed;
		float DollySpeed;

		FInt2 MouseStartPoint;
		FInt2 MouseCurrentPoint;

		int32 HoldKey;
	};

} // end namespace ti

