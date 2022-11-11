/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	class TI_API FShader : public FRenderResource
	{
	public:
		FShader(const TString& ShaderName, EShaderType InType);
		FShader(const TShaderNames& RenderShaderNames, EShaderType InType);
		virtual ~FShader();

		EShaderType GetShaderType() const
		{
			return Type;
		}

		const TString& GetShaderName(E_SHADER_STAGE Stage) const
		{
			return ShaderNames.ShaderNames[Stage];
		}

		const TString& GetComputeShaderName() const
		{
			return ShaderNames.ShaderNames[ESS_COMPUTE_SHADER];
		}

		const TString& GetRtxShaderLibName() const
		{
			return ShaderNames.ShaderNames[ESS_SHADER_LIB];
		}

		void SetShaderBinding(FShaderBindingPtr InShaderBinding)
		{
			ShaderBinding = InShaderBinding;
		}
		FShaderBindingPtr GetShaderBinding()
		{
			return ShaderBinding;
		}

		// Add Local Shader Binding, return index
		uint32 AddLocalShaderBinding(FShaderBindingPtr InShaderBinding)
		{
			LocalShaderBindings.push_back(InShaderBinding);
			return (uint32)(LocalShaderBindings.size() - 1);
		}
		FShaderBindingPtr GetLocalShaderBinding(uint32 Index)
		{
			TI_ASSERT(Index < (uint32)LocalShaderBindings.size());
			return LocalShaderBindings[Index];
		}
	protected:

	protected:
		TShaderNames ShaderNames;
		EShaderType Type;
		FShaderBindingPtr ShaderBinding;
		TVector<FShaderBindingPtr> LocalShaderBindings;
	};
}
