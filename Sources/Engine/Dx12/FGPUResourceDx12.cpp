/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"

#if COMPILE_WITH_RHI_DX12
#include "FGPUResourceDx12.h"
#include "FRHIDx12.h"

namespace tix
{
	FGPUResourceDx12::FGPUResourceDx12()
		: UsageState(D3D12_RESOURCE_STATE_COMMON)
	{}

	FGPUResourceDx12::FGPUResourceDx12(D3D12_RESOURCE_STATES InitState)
		: UsageState(InitState)
	{}

	FGPUResourceDx12::~FGPUResourceDx12()
	{
		Resource = nullptr;
	}

	void FGPUResourceDx12::CreateResource(
		ID3D12Device* Device,
		const D3D12_HEAP_PROPERTIES* pHeapProperties,
		D3D12_HEAP_FLAGS HeapFlags,
		const D3D12_RESOURCE_DESC* pDesc,
		D3D12_RESOURCE_STATES InitialResourceState,
		const D3D12_CLEAR_VALUE* pOptimizedClearValue)
	{
		Resource = nullptr;
		UsageState = InitialResourceState;
		VALIDATE_HRESULT(Device->CreateCommittedResource(
			pHeapProperties,
			HeapFlags,
			pDesc,
			InitialResourceState,
			pOptimizedClearValue,
			IID_PPV_ARGS(&Resource)));
	}

	/////////////////////////////////////////////////////////////

	void FGPUResourceBufferDx12::Init(const FGPUResourceDesc& Desc, TStreamPtr Data)
	{
		FRHIDx12* RHIDx12 = static_cast<FRHIDx12*>(FRHI::Get());
		ID3D12Device* Device = RHIDx12->GetD3dDevice();
		ID3D12GraphicsCommandList* CommandList = RHIDx12->GetCommandList();

		TI_ASSERT(Resource == nullptr);
		CD3DX12_RESOURCE_DESC ResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(Desc.BufferSize);

		if ((Desc.Flag & UB_FLAG_INTERMEDIATE) != 0)
		{
			// Create resource on UPLOAD heap directly for simple and efficency
			CD3DX12_HEAP_PROPERTIES UploadHeapProperties(D3D12_HEAP_TYPE_UPLOAD);
			VALIDATE_HRESULT(Device->CreateCommittedResource(
				&UploadHeapProperties,
				D3D12_HEAP_FLAG_NONE,
				&ResourceDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&Resource)));

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
		else
		{
			// Create upload heap resource and copy to default heap
			CD3DX12_HEAP_PROPERTIES DefaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);
			VALIDATE_HRESULT(Device->CreateCommittedResource(
				&DefaultHeapProperties,
				D3D12_HEAP_FLAG_NONE,
				&ResourceDesc,
				D3D12_RESOURCE_STATE_COPY_DEST,
				nullptr,
				IID_PPV_ARGS(&Resource)));

			if (Data != nullptr)
			{
				ComPtr<ID3D12Resource> BufferUpload;
				CD3DX12_HEAP_PROPERTIES UploadHeapProperties(D3D12_HEAP_TYPE_UPLOAD);
				VALIDATE_HRESULT(Device->CreateCommittedResource(
					&UploadHeapProperties,
					D3D12_HEAP_FLAG_NONE,
					&ResourceDesc,
					D3D12_RESOURCE_STATE_GENERIC_READ,
					nullptr,
					IID_PPV_ARGS(&BufferUpload)));

				// Upload the buffer data to the GPU.
				D3D12_SUBRESOURCE_DATA BufferData = {};
				BufferData.pData = reinterpret_cast<const uint8*>(Data->GetBuffer());
				BufferData.RowPitch = Desc.BufferSize;
				BufferData.SlicePitch = Desc.BufferSize;

				UpdateSubresources(CommandList, Resource.Get(), BufferUpload.Get(), 0, 0, 1, &BufferData);
				RHIDx12->HoldResourceReference(BufferUpload);
			}

			TI_ASSERT(0);// TODO: State change, and barriers
		}
	}
}
#endif	// COMPILE_WITH_RHI_DX12