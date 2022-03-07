/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "FRHIAsyncDx12.h"

#if COMPILE_WITH_RHI_DX12

//namespace tix
//{
//	FRHIAsyncDx12::FRHIAsyncDx12(const TString& InRHIName)
//		: FRHIDx12(InRHIName)
//	{
//	}
//
//	FRHIAsyncDx12::~FRHIAsyncDx12()
//	{
//	}
//
//	void FRHIAsyncDx12::InitAsyncRHI(ComPtr<ID3D12Device> InD3dDevice)
//	{
//		D3dDevice = InD3dDevice;
//
//		// Create the command queue. allocator, command list and fence
//		// Command Queue
//		D3D12_COMMAND_LIST_TYPE CmdListType = D3D12_COMMAND_LIST_TYPE_COMPUTE;
//		D3D12_COMMAND_QUEUE_DESC QueueDesc = {};
//		QueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
//		QueueDesc.Type = CmdListType;
//
//		VALIDATE_HRESULT(D3dDevice->CreateCommandQueue(&QueueDesc, IID_PPV_ARGS(&CommandQueue)));
//		CommandQueue->SetName(FromString(RHIName + "-CmdQueue").c_str());
//
//		// Command Allocators
//		char Name[128];
//		for (uint32 n = 0; n < FRHIConfig::FrameBufferNum; n++)
//		{
//			sprintf_s(Name, 128, "%s-CmdAllocator%d", RHIName.c_str(), n);
//			VALIDATE_HRESULT(D3dDevice->CreateCommandAllocator(CmdListType, IID_PPV_ARGS(&CommandAllocators[n])));
//			CommandAllocators[n]->SetName(FromString(Name).c_str());
//		}
//
//		// Command List
//		VALIDATE_HRESULT(D3dDevice->CreateCommandList(
//			0,
//			CmdListType,
//			CommandAllocators[0].Get(),
//			nullptr,
//			IID_PPV_ARGS(&CommandList)));
//		CommandList->SetName(FromString(RHIName + "-CmdList").c_str());
//		VALIDATE_HRESULT(CommandList->Close());
//
//		// Fence
//		VALIDATE_HRESULT(D3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&CommandFence)));
//
//		// Create descriptor heaps for render target views and depth stencil views.
//		TI_ASSERT(0);	// What about descriptor heaps
//
//		_LOG(ELog::Log, "  AsyncRHI [%s] created.\n", RHIName.c_str());
//	}
//}
#endif	// COMPILE_WITH_RHI_DX12