/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	enum class EShaderType : uint8
	{
		Standard,	// VS, HS, DS, GS, PS
		AmpMesh,	// Amp, Mesh, PS
		Compute,	// CS
		ShaderLib,	// For Raytracing
	};
	enum E_SHADER_STAGE
	{
		ESS_COMPUTE_SHADER = 0,
		ESS_SHADER_LIB = 0,
		ESS_VERTEX_SHADER = 0,
		ESS_AMPLIFICATION_SHADER = 0,

		ESS_HULL_SHADER = 1,
		ESS_MESH_SHADER = 1,

		ESS_DOMAIN_SHADER = 2,
		ESS_GEOMETRY_SHADER = 3,
		ESS_PIXEL_SHADER = 4,

		ESS_COUNT,
	};

	struct TShaderNames
	{
		TString ShaderNames[ESS_COUNT];

		TString GetSearchKey() const
		{
			return ShaderNames[ESS_VERTEX_SHADER] +
				ShaderNames[ESS_HULL_SHADER] +
				ShaderNames[ESS_DOMAIN_SHADER] +
				ShaderNames[ESS_GEOMETRY_SHADER] +
				ShaderNames[ESS_PIXEL_SHADER];
		}
	};

	class TI_API TShader : public TResource
	{
	public:
		// Used for Single Compute Shader or RTX Shaderlib
		TShader(const TString& InShaderName, EShaderType InType);

		// Used for Graphics pipeline with vs, gs, ps etc
		TShader(const TShaderNames& InNames, EShaderType InType = EShaderType::Standard);
		virtual ~TShader();
		
		EShaderType GetType() const
		{
			return Type;
		}

		static TStreamPtr LoadShaderBlob(const TString& ShaderName);
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
		EShaderType Type;
		TShaderNames Names;
		TVector<TStreamPtr> ShaderCodes;
	};
}
