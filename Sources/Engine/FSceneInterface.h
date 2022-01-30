/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

#include "FUniformBufferView.h"

namespace tix
{
	class FRendererInterface;
	class TI_API FSceneInterface
	{
	public:
		FSceneInterface() {};
		virtual ~FSceneInterface() {};

		// Camera info
		virtual void SetViewProjection(const FViewProjectionInfo& Info) = 0;

		// Lights info
		virtual void SetEnvironmentInfo(const FEnvironmentInfo& Info) = 0;
		virtual void SetEnvLight(FEnvLightPtr InEnvLight) = 0;
		virtual FEnvLightPtr GetEnvLight() = 0;

		// Scene Primitives
		virtual void AddPrimitive(FPrimitivePtr InPrim) = 0;
	private:

	private:
	};
} // end namespace tix
