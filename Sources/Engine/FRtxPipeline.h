/*
TiX Engine v3.0 Copyright (C) 2022~2025
By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	class FRtxPipeline : public FRenderResource
	{
	public:
		FRtxPipeline(FShaderPtr InShader);
		virtual ~FRtxPipeline();

		FShaderPtr GetShaderLib()
		{
			return ShaderLib;
		}
	protected:

	protected:
		FShaderPtr ShaderLib;
	};
}
