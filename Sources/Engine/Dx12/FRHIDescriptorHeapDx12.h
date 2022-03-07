/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

#if COMPILE_WITH_RHI_DX12

#include "d3dx12.h"
#include "FHeapAllocator.h"

using namespace Microsoft::WRL;

namespace tix
{
	class FRHIDx12;
	class FDescriptorHeapDx12 : public FRHIHeap
	{
	public:
		FDescriptorHeapDx12(uint32 InHeapId, EResourceHeapType InType);
		virtual ~FDescriptorHeapDx12();

		// Overrides from FRHIHeap
		virtual FRenderResourceTablePtr CreateRenderResourceTable(uint32 InSize) override;
		virtual void RecallRenderResourceTable(uint32 InStart, uint32 InSize) override;

		// Dx12 Descriptor Heap methods
		void Create(ID3D12Device* D3dDevice);

		D3D12_CPU_DESCRIPTOR_HANDLE GetCpuDescriptorHandle(uint32 Index);
		D3D12_GPU_DESCRIPTOR_HANDLE GetGpuDescriptorHandle(uint32 Index);
		
		uint32 GetIncSize() const
		{
			return DescriptorIncSize;
		}

		ID3D12DescriptorHeap* GetHeap()
		{
			return DescriptorHeap.Get();
		}
	private:
		FHeapAllocator HeapAllocator;
		ComPtr<ID3D12DescriptorHeap> DescriptorHeap;
		uint32 DescriptorIncSize;
	};
}
#endif	// COMPILE_WITH_RHI_DX12