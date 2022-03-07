/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

#if COMPILE_WITH_RHI_DX12

namespace tix
{
	// Dx12 need uniform for data, resource table for textures
	class FArgumentBufferDx12 : public FArgumentBuffer
	{
	public:
		FArgumentBufferDx12(int32 ReservedSlots);
		virtual ~FArgumentBufferDx12();

	protected:

	private:
		FRenderResourceTablePtr ResourceTable;

		friend class FRHIDx12;
		friend class FRHICmdListDx12;
	};
}

#endif	// COMPILE_WITH_RHI_DX12
