/*
TiX Engine v3.0 Copyright (C) 2022~2025
By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	class FArgumentBuffer : public FRenderResource
	{
	public:
		static const int32 MaxResourcesInArgumentBuffer = 16;
		FArgumentBuffer(int32 ReservedSlots);
		virtual ~FArgumentBuffer();

		TI_API void SetBuffer(int32 Index, FUniformBufferPtr InUniform);
		TI_API void SetTexture(int32 Index, FTexturePtr InTexture);

		const TVector<FRenderResourcePtr>& GetArguments() const
		{
			return Arguments;
		}

	protected:

	protected:
		TVector<FRenderResourcePtr> Arguments;
	};
}
