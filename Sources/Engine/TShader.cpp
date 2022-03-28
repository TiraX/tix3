/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "TShader.h"

namespace tix
{
	TShader::TShader(const TString& InShaderName, E_SHADER_TYPE InType)
		: TResource(ERES_SHADER)
		, Type(InType)
	{
		Names.ShaderNames[0] = InShaderName;
		ShaderCodes.resize(ESS_COUNT);
	}

	TShader::TShader(const TShaderNames& InNames, E_SHADER_TYPE InType)
		: TResource(ERES_SHADER)
		, Type(InType)
	{
		for (int32 s = 0; s < ESS_COUNT; ++s)
		{
			Names.ShaderNames[s] = InNames.ShaderNames[s];
		}
		ShaderCodes.resize(ESS_COUNT);
	}

	TShader::~TShader()
	{
	}

	TStreamPtr TShader::LoadShaderBlob(const TString& InShaderName)
	{
		TString ShaderName = InShaderName;
#if defined (COMPILE_WITH_RHI_DX12)
		if (ShaderName.rfind(".cso") == TString::npos)
			ShaderName += ".cso";
		// Load shader code
		TFile File;
		if (File.Open(ShaderName, EFA_READ))
		{
			TStreamPtr Blob = ti_new TStream;
			Blob->Put(File);
			File.Close();
			return Blob;
		}
		else
		{
			_LOG(ELog::Error, "Failed to load shader code [%s].\n", ShaderName.c_str());
		}
#elif defined (COMPILE_WITH_RHI_METAL)
		// Metal need shader name only
#else
		TI_ASSERT(0);
#endif
		return nullptr;
	}

	void TShader::LoadShaderCode()
	{
		for (int32 s = 0; s < ESS_COUNT; ++s)
		{
			if (!Names.ShaderNames[s].empty())
			{
				ShaderCodes[s] = LoadShaderBlob(Names.ShaderNames[s]);
			}
		}
	}

	void TShader::InitRenderThreadResource()
	{
		TI_ASSERT(ShaderResource == nullptr);
		ShaderResource = FRHI::Get()->CreateShader(Names, Type);

		FShaderPtr Shader_RT = ShaderResource;
		TVector<TStreamPtr> Codes = ShaderCodes;
		ENQUEUE_RENDER_COMMAND(TShaderUpdateResource)(
			[Shader_RT, Codes]()
			{
				// Add TShader -> Shader Codes herer.
				FRHI::Get()->UpdateHardwareResourceShader(Shader_RT, Codes);
			});
		ReleaseShaderCode();
	}

	void TShader::ReleaseShaderCode()
	{
		for (int32 s = 0; s < ESS_COUNT; ++s)
		{
			ShaderCodes[s] = nullptr;
		}
	}

	void TShader::DestroyRenderThreadResource()
	{
		TI_ASSERT(ShaderResource != nullptr);

		FShaderPtr Shader_RT = ShaderResource;
		ENQUEUE_RENDER_COMMAND(TShaderDestroyFShader)(
			[Shader_RT]()
			{
				//Shader_RT = nullptr;
			});
		ShaderResource = nullptr;
	}
}
