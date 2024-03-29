/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"

#if COMPILE_WITH_RHI_DX12
#include "FGPUBufferDx12.h"
#include "FRHIDx12.h"
#include "FRHICmdListDx12.h"
#include "FRHIDx12Conversion.h"

namespace tix
{
	void FGPUBufferDx12::Init(FRHICmdList* RHICmdList, const FGPUBufferDesc& Desc, TStreamPtr Data)
	{
		FRHIDx12* RHIDx12 = static_cast<FRHIDx12*>(FRHI::Get());

		TI_ASSERT(Resource == nullptr);
		CD3DX12_RESOURCE_DESC ResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(Desc.BufferSize);
		if ((Desc.Flag & (uint32)EGPUResourceFlag::Uav) != 0)
		{
			ResourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

			// if need add counter for UAV
			if ((Desc.Flag & (uint32)EGPUResourceFlag::UavCounter) != 0)
			{
				ResourceDesc.Width = FRHIDx12::GetUavSizeWithCounter(Desc.BufferSize);
			}
		}

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
				const int32 CopySize = TMath::Min(Desc.BufferSize, Data->GetLength());
				memcpy(MappedAddress, Data->GetBuffer(), CopySize);
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
				// Use a new RESOURCE_DESC to avoid UAV flags.
				CD3DX12_RESOURCE_DESC UploadDesc = CD3DX12_RESOURCE_DESC::Buffer(Desc.BufferSize);
				RHIDx12->CreateD3D12Resource(
					D3D12_HEAP_TYPE_UPLOAD,
					&UploadDesc,
					D3D12_RESOURCE_STATE_GENERIC_READ,
					nullptr,
					BufferUpload
				);
				BufferUpload->SetName(L"TccBufferUpload");

				// Upload the buffer data to the GPU.
				const int32 CopySize = TMath::Min(Desc.BufferSize, Data->GetLength());
				D3D12_SUBRESOURCE_DATA BufferData = {};
				BufferData.pData = reinterpret_cast<const uint8*>(Data->GetBuffer());
				BufferData.RowPitch = CopySize;
				BufferData.SlicePitch = CopySize;

				FRHICmdListDx12* CmdListDx12 = static_cast<FRHICmdListDx12*>(RHICmdList);
				CmdListDx12->UpdateD3D12Resource(
					Resource.Get(),
					BufferUpload.Get(),
					1,
					&BufferData);
				CmdListDx12->HoldResourceReference(BufferUpload);
			}
		}
	}

	uint8* FGPUBufferDx12::Lock()
	{
		uint8* MappedAddress = nullptr;
		CD3DX12_RANGE ReadRange(0, 0);		// We do not intend to read from this resource on the CPU.
		VALIDATE_HRESULT(Resource->Map(0, &ReadRange, reinterpret_cast<void**>(&MappedAddress)));
		return MappedAddress;
	}

	void FGPUBufferDx12::Unlock()
	{
		Resource->Unmap(0, nullptr);
	}
}
#endif	// COMPILE_WITH_RHI_DX12