/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "FShader.h"

namespace tix
{
	FShader::FShader(const TString& InShaderName, E_SHADER_TYPE InType)
		: FRenderResource(ERenderResourceType::Shader)
		, Type(InType)
	{
		ShaderNames.ShaderNames[0] = InShaderName;
	}

	FShader::FShader(const TShaderNames& RenderShaderNames, E_SHADER_TYPE InType)
		: FRenderResource(ERenderResourceType::Shader)
		, ShaderNames(RenderShaderNames)
		, Type(InType)
	{
	}

	FShader::~FShader()
	{
        TI_ASSERT(IsRenderThread());
	}
}
