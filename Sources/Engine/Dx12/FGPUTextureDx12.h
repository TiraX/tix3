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
			: ResourceState(EGPUResourceState::Common)
		{}

		virtual ~FGPUTextureDx12()
		{}

		virtual void Init(const FGPUTextureDesc& Desc, const TVector<TImagePtr>& Data) override;
		ID3D12Resource* GetResource()
		{
			return Resource.Get();
		}
	private:
		ComPtr<ID3D12Resource> Resource;
		EGPUResourceState ResourceState;

		friend class FRHIDx12;
	};
}
#endif	// COMPILE_WITH_RHI_DX12