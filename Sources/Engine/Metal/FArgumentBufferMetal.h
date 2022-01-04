/*
	TiX Engine v2.0 Copyright (C) 2018~2019
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

#if COMPILE_WITH_RHI_METAL

namespace tix
{
	// Metal has argument buffer
	class FArgumentBufferMetal : public FArgumentBuffer
	{
	public:
		FArgumentBufferMetal(int32 ReservedSlots);
		virtual ~FArgumentBufferMetal();
	protected:

	private:
        id<MTLBuffer> ArgumentBuffer;
		friend class FRHIMetal;
	};
}

#endif	// COMPILE_WITH_RHI_METAL
