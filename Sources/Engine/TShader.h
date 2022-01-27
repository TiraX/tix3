/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	enum E_SHADER_TYPE
	{
		EST_RENDER,
		EST_COMPUTE,
		EST_SHADERLIB,

		EST_COUNT,
	};
	enum E_SHADER_STAGE
	{
		ESS_COMPUTE_SHADER = 0,
		ESS_SHADER_LIB = 0,
		ESS_VERTEX_SHADER = 0,
		ESS_PIXEL_SHADER,
		ESS_DOMAIN_SHADER,
		ESS_HULL_SHADER,
		ESS_GEOMETRY_SHADER,

		ESS_COUNT,
	};

	struct TShaderNames
	{
		TString ShaderNames[ESS_COUNT];

		TString GetSearchKey() const
		{
			return ShaderNames[ESS_VERTEX_SHADER] +
				ShaderNames[ESS_PIXEL_SHADER] +
				ShaderNames[ESS_DOMAIN_SHADER] +
				ShaderNames[ESS_HULL_SHADER] +
				ShaderNames[ESS_GEOMETRY_SHADER];
		}
	};

	class TShader : public TResource
	{
	public:
		// Used for Single Compute Shader or RTX Shaderlib
		TShader(const TString& InShaderName, E_SHADER_TYPE InType);

		// Used for Graphics pipeline with vs, gs, ps etc
		TShader(const TShaderNames& InNames, E_SHADER_TYPE InType = EST_RENDER);
		virtual ~TShader();
		
		E_SHADER_TYPE GetType() const
		{
			return Type;
		}

		void LoadShaderCode();

		const TVector<TStreamPtr>& GetShaderCodes()
		{
			return ShaderCodes;
		}

		virtual void InitRenderThreadResource() override;
		virtual void DestroyRenderThreadResource() override;

		FShaderPtr ShaderResource;

	protected:
		void ReleaseShaderCode();

	protected:
		E_SHADER_TYPE Type;
		TShaderNames Names;
		TVector<TStreamPtr> ShaderCodes;
	};
}
