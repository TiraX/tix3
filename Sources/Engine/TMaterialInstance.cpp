/*
TiX Engine v3.0 Copyright (C) 2022~2025
By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"

namespace tix
{
	TMaterialInstance::TMaterialInstance()
		: TResource(ERES_MATERIAL_INSTANCE)
	{}

	TMaterialInstance::~TMaterialInstance()
	{
	}

	void TMaterialInstance::InitRenderThreadResource()
	{
		if (ParamValueBuffer != nullptr || ParamTextures.size() > 0)
		{
			TI_ASSERT(ArgumentBuffer == nullptr);
			TI_ASSERT(LinkedMaterial->GetDesc().Shader != nullptr);
			int32 NumBuffers = ParamValueBuffer != nullptr ? 1 : 0;
			int32 NumTextures = (int32)ParamTextures.size();
			int32 NumArguments = NumBuffers + NumTextures;
			ArgumentBuffer = FRHI::Get()->CreateArgumentBuffer(NumArguments);

			// Number Parameters
			if (ParamValueBuffer != nullptr)
			{
				uint32 UBFlag = (uint32)(ParamValueBuffer->GetLength() < 4096 ? EGPUResourceFlag::Intermediate : EGPUResourceFlag::None);
				FUniformBufferPtr UniformBuffer = ti_new FUniformBuffer(TMath::Align(ParamValueBuffer->GetLength(), FUniformBuffer::Alignment), 1, UBFlag);
				UniformBuffer->SetResourceName(GetResourceName());

				TStreamPtr _ParamValueBuffer = ParamValueBuffer;
				ENQUEUE_RENDER_COMMAND(UpdateMIUniformBuffer)(
					[UniformBuffer, _ParamValueBuffer]()
					{
						FRHICmdList* CmdList = FRHI::Get()->GetDefaultCmdList();
						UniformBuffer->CreateGPUBuffer(CmdList, _ParamValueBuffer);
					});

				ArgumentBuffer->SetBuffer(0, UniformBuffer);
			}

			// Texture Parameters
			for (int32 i = 0; i < (int32)ParamTextures.size(); ++i)
			{
				TI_ASSERT(ParamTextures[i]->TextureResource != nullptr);
				ArgumentBuffer->SetTexture(i + NumBuffers, ParamTextures[i]->TextureResource);
			}

			FShaderPtr MaterialShader = LinkedMaterial->GetDesc().Shader->ShaderResource;

			FArgumentBufferPtr _ArgumentBuffer = ArgumentBuffer;
			ENQUEUE_RENDER_COMMAND(UpdateMIArgumentBuffer)(
				[_ArgumentBuffer, MaterialShader]()
				{
					FRHI::Get()->UpdateHardwareResourceAB(_ArgumentBuffer, MaterialShader);
				});
		}
	}

	void TMaterialInstance::DestroyRenderThreadResource()
	{
		if (ArgumentBuffer != nullptr)
		{
			FArgumentBufferPtr _ArgumentBuffer = ArgumentBuffer;
			ENQUEUE_RENDER_COMMAND(TMIDestroyArgumentBuffer)(
				[_ArgumentBuffer]()
				{
					//_ArgumentBuffer = nullptr;
				});
			ArgumentBuffer = nullptr;
		}
	}

	static const int32 ParamTypeLength[MIPT_COUNT] =
	{
		0,	//MIPT_UNKNOWN,
		4,	//MIPT_INT,
		4,	//MIPT_FLOAT,
		16,	//MIPT_INT4,
		16,	//MIPT_FLOAT4,
		8,	//MIPT_TEXTURE ((int32) + (int16) * 2),
	};

	int32 TMaterialInstance::GetParamTypeBytes(E_MI_PARAM_TYPE Type)
	{
		return ParamTypeLength[Type];
	}
}
