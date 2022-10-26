/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

#if COMPILE_WITH_RHI_DX12
#include "d3dx12.h"
using namespace Microsoft::WRL;

namespace tix
{
	class FGPUTextureDx12 : public FGPUTexture
	{
	public:
		FGPUTextureDx12()
		{}

		virtual ~FGPUTextureDx12()
		{}

		virtual void Init(FRHICmdList* RHICmdList, const FGPUTextureDesc& Desc, const TVector<TImagePtr>& Data) override;

		virtual uint8* Lock() override;
		virtual void Unlock() override;

		ID3D12Resource* GetResource()
		{
			return Resource.Get();
		}
	private:
		ComPtr<ID3D12Resource> Resource;

		friend class FRHIDx12;
		friend class FRHICmdListDx12;
	};
}
#endif	// COMPILE_WITH_RHI_DX12