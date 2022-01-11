/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

#if COMPILE_WITH_RHI_DX12

namespace tix
{
	class FShaderDx12 : public FShader
	{
	public:
		FShaderDx12(const TString& InShaderName, E_SHADER_TYPE InType);
		FShaderDx12(const TShaderNames& RenderShaderNames, E_SHADER_TYPE InType);
		virtual ~FShaderDx12();

		void ReleaseShaderCode();
	protected:

	protected:
		TStream ShaderCodes[ESS_COUNT];

		friend class FRHIDx12;
	};
}

#endif	// COMPILE_WITH_RHI_DX12
