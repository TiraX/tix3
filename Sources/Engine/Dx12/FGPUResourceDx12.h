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
	class FRHIDx12;

	class FGPUResourceDx12
	{
	public:
		FGPUResourceDx12();
		FGPUResourceDx12(D3D12_RESOURCE_STATES InitState);
		~FGPUResourceDx12();

		bool IsInited() const
		{
			return Resource != nullptr;
		}

		ComPtr<ID3D12Resource> GetResource()
		{
			return Resource;
		}

		D3D12_RESOURCE_STATES GetCurrentState()
		{
			return UsageState;
		}

		void CreateResource(
			ID3D12Device* Device,
			const D3D12_HEAP_PROPERTIES *pHeapProperties,
			D3D12_HEAP_FLAGS HeapFlags,
			const D3D12_RESOURCE_DESC *pDesc,
			D3D12_RESOURCE_STATES InitialResourceState,
			const D3D12_CLEAR_VALUE *pOptimizedClearValue = nullptr);

	private:
		D3D12_RESOURCE_STATES UsageState;
		ComPtr<ID3D12Resource> Resource;

		friend class FRHIDx12;
	};

	/////////////////////////////////////////////////////////////

	class FGPUResourceBufferDx12 : public FGPUResourceBuffer
	{
	public:
		FGPUResourceBufferDx12()
			: ResourceState(EGPUResourceState::Common)
		{}

		virtual ~FGPUResourceBufferDx12()
		{}

		virtual void Init(const FGPUResourceDesc& Desc, TStreamPtr Data) override;

	private:
		ComPtr<ID3D12Resource> Resource;
		EGPUResourceState ResourceState;

		friend class FRHIDx12;
	};

	/////////////////////////////////////////////////////////////

	class FGPUResourceTextureDx12 : public FGPUResourceTexture
	{
	public:
		FGPUResourceTextureDx12()
			: ResourceState(EGPUResourceState::Common)
		{}

		virtual ~FGPUResourceTextureDx12()
		{}

		virtual void Init(const FGPUResourceDesc& Desc, TStreamPtr Data) override {};

	private:
		ComPtr<ID3D12Resource> Resource;
		EGPUResourceState ResourceState;

		friend class FRHIDx12;
	};
}
#endif	// COMPILE_WITH_RHI_DX12