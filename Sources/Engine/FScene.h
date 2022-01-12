/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

#include "FUniformBufferView.h"

namespace tix
{
	class FSceneLights;
	class FScene
	{
	public:
		FScene();
		~FScene();

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

		TI_API void InitRenderFrame();

		TI_API void SetViewProjection(const FViewProjectionInfo& Info);
		TI_API void SetEnvironmentInfo(const FEnvironmentInfo& Info);

		void AddSceneTileInfo(FSceneTileResourcePtr SceneTileResource);
		void RemoveSceneTileInfo(FSceneTileResourcePtr SceneTileResource);

		void AddEnvLight(FTexturePtr CubeTexture, const FFloat3& Position);
		void RemoveEnvLight(FEnvLightPtr InEnvLight);

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

		FSceneLights* GetSceneLights()
		{
			return SceneLights;
		}

		const FViewProjectionInfo& GetViewProjection() const
		{
			return ViewProjection;
		}

		const FEnvironmentInfo& GetEnvironmentInfo() const
		{
			return EnvInfo;
		}

		const THMap<FInt2, FSceneTileResourcePtr>& GetSceneTiles() const
		{
			return SceneTiles;
		}

		FViewUniformBufferPtr GetViewUniformBuffer()
		{
			return ViewUniformBuffer;
		}

		FEnvLightPtr FindNearestEnvLight(const FFloat3& Pos)
		{
			return EnvLight;
		}

		FTopLevelAccelerationStructurePtr GetTLAS()
		{
			return SceneTLAS;
		}
	private:
		void PrepareViewUniforms();
		void UpdateAccelerationStructure();

	private:
		FSceneLights * SceneLights;

		// Scene flags per frame, will be cleared by the end of this frame
		uint32 SceneFlags;

		FViewProjectionInfo ViewProjection;
		FEnvironmentInfo EnvInfo;

		// Uniform buffers
		FViewUniformBufferPtr ViewUniformBuffer;

		// Scene tiles
		THMap<FInt2, FSceneTileResourcePtr> SceneTiles;

		// Scene TLAS
		FTopLevelAccelerationStructurePtr SceneTLAS;

		// Env Lights, leave ONE envlight temp, should support multi env light in futher
		FEnvLightPtr EnvLight;
	};
} // end namespace tix
