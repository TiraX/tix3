/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once
#include "FUniformBufferView.h"

namespace tix
{
	class FDefaultScene : public FSceneInterface
	{
	public:
		FDefaultScene();
		virtual ~FDefaultScene();

		// Scene flags clear every frames
		enum SceneFlag
		{
			ViewProjectionDirty = 1 << 0,
			ScenePrimitivesDirty = 1 << 1,
			EnvironmentDirty = 1 << 2,
			SceneTileDirty = 1 << 3,
			SceneBLASDirty = 1 << 4,

			ViewUniformDirty = (ViewProjectionDirty | EnvironmentDirty),
		};

		virtual void SetViewProjection(const FViewProjectionInfo& Info) override;
		virtual const FViewProjectionInfo& GetViewProjection() const override
		{
			return ViewProjection;
		}
		virtual void SetEnvironmentInfo(const FEnvironmentInfo& Info) override;
		virtual const FEnvironmentInfo& GetEnvironmentInfo() const override
		{
			return EnvInfo;
		}
		virtual void SetEnvLight(FEnvLightPtr InEnvLight) override;
		virtual FEnvLightPtr GetEnvLight() override;

		virtual void AddPrimitive(FPrimitivePtr InPrim) override;
		virtual void AddShadowPrimitive(FPrimitivePtr InPrim) override;
		virtual void ClearAllPrimitives() override;

		bool HasSceneFlag(SceneFlag Flag) const
		{
			return (SceneFlags & Flag) != 0;
		}

		void SetSceneFlag(SceneFlag Flag)
		{
			SceneFlags |= Flag;
		}

		void ClearSceneFlags()
		{
			SceneFlags = 0;
		}



	private:

	private:
		// Scene flags per frame, will be cleared by the end of this frame
		uint32 SceneFlags;

		FViewProjectionInfo ViewProjection;
		FEnvironmentInfo EnvInfo;
		// Env Lights, leave ONE envlight temp, should support multi env light in future
		FEnvLightPtr EnvLight;

		// Primitives
		TVector<FPrimitivePtr> ShadowPrimitives;
		TVector<FPrimitivePtr> Primitives;

		friend class FDefaultRenderer;
	};
} // end namespace tix
