/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"

#if COMPILE_WITH_RHI_DX12
#include "FGPUBufferDx12.h"
#include "FRHIDx12.h"
#include "FRHIDx12Conversion.h"

namespace tix
{
	void FGPUBufferDx12::Init(const FGPUBufferDesc& Desc, TStreamPtr Data)
	{
		FRHIDx12* RHIDx12 = static_cast<FRHIDx12*>(FRHI::Get());

		TI_ASSERT(Resource == nullptr);
		CD3DX12_RESOURCE_DESC ResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(Desc.BufferSize);

		if ((Desc.Flag & (uint32)EGPUResourceFlag::Intermediate) != 0)
		{
			// Create resource on UPLOAD heap directly for simple and efficency
			ResourceState = EGPUResourceState::GenericRead;
			D3D12_RESOURCE_STATES InitState = GetDx12ResourceState(ResourceState);
			RHIDx12->CreateD3D12Resource(
				D3D12_HEAP_TYPE_UPLOAD,
				&ResourceDesc,
				InitState,
				nullptr,
				Resource
			);

			if (Data != nullptr)
			{
				// Copy data
				uint8* MappedAddress = nullptr;
				CD3DX12_RANGE ReadRange(0, 0);		// We do not intend to read from this resource on the CPU.
				VALIDATE_HRESULT(Resource->Map(0, &ReadRange, reinterpret_cast<void**>(&MappedAddress)));
				memcpy(MappedAddress, Data->GetBuffer(), Desc.BufferSize);
				Resource->Unmap(0, nullptr);
			}
		}
		else if ((Desc.Flag & (uint32)EGPUResourceFlag::Readback) != 0)
		{
			// Readback buffer, create on READBACK heap
			ResourceState = EGPUResourceState::CopyDest;
			D3D12_RESOURCE_STATES InitState = GetDx12ResourceState(ResourceState);
			RHIDx12->CreateD3D12Resource(
				D3D12_HEAP_TYPE_READBACK,
				&ResourceDesc,
				InitState,
				nullptr,
				Resource
			);
		}
		else
		{
			// Create upload heap resource and copy to default heap
			ResourceState = EGPUResourceState::CopyDest;
			D3D12_RESOURCE_STATES InitState = GetDx12ResourceState(ResourceState);
			RHIDx12->CreateD3D12Resource(
				D3D12_HEAP_TYPE_DEFAULT,
				&ResourceDesc,
				InitState,
				nullptr,
				Resource
			);

			if (Data != nullptr)
			{
				ComPtr<ID3D12Resource> BufferUpload;
				RHIDx12->CreateD3D12Resource(
					D3D12_HEAP_TYPE_UPLOAD,
					&ResourceDesc,
					D3D12_RESOURCE_STATE_GENERIC_READ,
					nullptr,
					BufferUpload
				);

				// Upload the buffer data to the GPU.
				D3D12_SUBRESOURCE_DATA BufferData = {};
				BufferData.pData = reinterpret_cast<const uint8*>(Data->GetBuffer());
				BufferData.RowPitch = Desc.BufferSize;
				BufferData.SlicePitch = Desc.BufferSize;

				RHIDx12->UpdateD3D12Resource(
					Resource.Get(),
					BufferUpload.Get(),
					1,
					&BufferData);
				RHIDx12->HoldResourceReference(BufferUpload);
			}
		}
	}
}
#endif	// COMPILE_WITH_RHI_DX12