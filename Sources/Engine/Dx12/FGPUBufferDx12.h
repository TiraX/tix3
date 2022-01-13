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
	class FGPUBufferDx12 : public FGPUBuffer
	{
	public:
		FGPUBufferDx12()
			: ResourceState(EGPUResourceState::Common)
		{}

		virtual ~FGPUBufferDx12()
		{}

		virtual void Init(const FGPUBufferDesc& Desc, TStreamPtr Data) override;
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