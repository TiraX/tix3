/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "FRHIDescriptorHeapDx12.h"
#include "FRHIDx12.h"
#include "FRHIDx12Conversion.h"

#if COMPILE_WITH_RHI_DX12

namespace tix
{
	static const int32 MaxDescriptorCount[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] =
	{
		MAX_HEAP_SRV_CBV,	//EHT_CBV_SRV_UAV,
		MAX_HEAP_SAMPLERS,	//EHT_SAMPLER,
		MAX_HEAP_RENDERTARGETS,	//EHT_RTV,
		MAX_HEAP_DEPTHSTENCILS,	//EHT_DSV,
	};

	static const D3D12_DESCRIPTOR_HEAP_FLAGS k_HEAP_CREATE_FLAGS[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] =
	{
		D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,	//EHT_CBV_SRV_UAV,
		D3D12_DESCRIPTOR_HEAP_FLAG_NONE,	//EHT_SAMPLER,
		D3D12_DESCRIPTOR_HEAP_FLAG_NONE,	//EHT_RTV,
		D3D12_DESCRIPTOR_HEAP_FLAG_NONE,	//EHT_DSV,
	};

	static const LPCWSTR k_HEAP_NAMES[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] =
	{
		L"CBV_SRV_UAV_HEAP",	//EHT_CBV_SRV_UAV,
		L"SAMPLER_HEAP",	//EHT_SAMPLER,
		L"RTV_HEAP",	//EHT_RTV,
		L"DSV_HEAP",	//EHT_DSV,
	};

	FDescriptorHeapDx12::FDescriptorHeapDx12(uint32 InHeapId, EResourceHeapType InType)
		: FRHIHeap(InHeapId, InType)
		, DescriptorIncSize(0)
	{
	}

	FDescriptorHeapDx12::~FDescriptorHeapDx12()
	{
	}

	FRenderResourceTablePtr FDescriptorHeapDx12::CreateRenderResourceTable(uint32 InSize)
	{
		uint32 StartIndex = HeapAllocator.AllocateTable(InSize);
		FRenderResourceTablePtr RT = ti_new FRenderResourceTable(HeapType, HeapId, StartIndex, InSize);
		return RT;
	}

	void FDescriptorHeapDx12::RecallRenderResourceTable(uint32 InStart, uint32 InSize)
	{
		HeapAllocator.RecallTable(InStart, InSize);
	}

	void FDescriptorHeapDx12::Create(ID3D12Device* D3dDevice)
	{
		int32 HeapIndex = static_cast<int32>(HeapType);
		HeapAllocator.Init(MaxDescriptorCount[HeapIndex]);

		auto GetDx12HeapType = [](EResourceHeapType InHeapType)
		{
			switch (InHeapType)
			{
			case EResourceHeapType::RenderTarget:
				return D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			case EResourceHeapType::DepthStencil:
				return D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
			case EResourceHeapType::Sampler:
				return D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
			case EResourceHeapType::ShaderResource:
				return D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			default:
				RuntimeFail();
				return D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			}
		};

		D3D12_DESCRIPTOR_HEAP_TYPE DxHeapType = GetDx12HeapType(HeapType);
		D3D12_DESCRIPTOR_HEAP_DESC HeapDesc = {};
		HeapDesc.NumDescriptors = MaxDescriptorCount[HeapIndex];
		HeapDesc.Type = DxHeapType;
		HeapDesc.Flags = k_HEAP_CREATE_FLAGS[HeapIndex];
		HeapDesc.NodeMask = 0;

		VALIDATE_HRESULT(D3dDevice->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(&DescriptorHeap)));
		DescriptorHeap->SetName(k_HEAP_NAMES[HeapIndex]);

		DescriptorIncSize = D3dDevice->GetDescriptorHandleIncrementSize(DxHeapType);
	}

	D3D12_CPU_DESCRIPTOR_HANDLE FDescriptorHeapDx12::GetCpuDescriptorHandle(uint32 Index)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE Result = DescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		Result.ptr += Index * DescriptorIncSize;
		return Result;
	}

	D3D12_GPU_DESCRIPTOR_HANDLE FDescriptorHeapDx12::GetGpuDescriptorHandle(uint32 Index)
	{
		D3D12_GPU_DESCRIPTOR_HANDLE Result = DescriptorHeap->GetGPUDescriptorHandleForHeapStart();
		Result.ptr += Index * DescriptorIncSize;
		return Result;
	}
}
#endif	// COMPILE_WITH_RHI_DX12