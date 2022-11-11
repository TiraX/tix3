/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "FRHIDx12.h"
#include "FShaderDx12.h"

#if COMPILE_WITH_RHI_DX12

namespace tix
{
	FShaderDx12::FShaderDx12(const TString& InShaderName, EShaderType InType)
		: FShader(InShaderName, InType)
	{
	}

	FShaderDx12::FShaderDx12(const TShaderNames& RenderShaderNames, EShaderType InType)
		: FShader(RenderShaderNames, InType)
	{
	}

	FShaderDx12::~FShaderDx12()
	{
		ShaderBinding = nullptr;
	}

	void FShaderDx12::ReleaseShaderCode()
	{
		for (int32 s = 0; s < ESS_COUNT; ++s)
		{
			ShaderCodes[s] = nullptr;
		}
	}
}

#endif	// COMPILE_WITH_RHI_DX12