/*
	TiX Engine v2.0 Copyright (C) 2018~2021
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once
#include "FUniformBuffer.h"

namespace tix
{
	BEGIN_UNIFORM_BUFFER_STRUCT(FViewUniformBuffer)
		DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FMatrix, ViewProjection)
		DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FFloat4, ViewDir)
		DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FFloat4, ViewPos)
		DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FFloat4, MainLightDirection)
		DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FFloat4, MainLightColor)
		DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_ARRAY(FFloat4, SkyIrradiance, [7])
	END_UNIFORM_BUFFER_STRUCT(FViewUniformBuffer)
}
