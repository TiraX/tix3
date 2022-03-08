/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include <DirectXColors.h>
#include "FRHICmdListDx12.h"
#include "FFrameResourcesDx12.h"
#include "FGPUBufferDx12.h"
#include "FGPUTextureDx12.h"
#include "FPipelineDx12.h"
#include "FRenderTargetDx12.h"
#include "FShaderDx12.h"
#include "FArgumentBufferDx12.h"
#include "FGPUCommandSignatureDx12.h"
#include "FGPUCommandBufferDx12.h"
#include "FRtxPipelineDx12.h"
#include "FAccelerationStructureDx12.h"
#include "FRHIDx12Conversion.h"
#include "FRHIDx12.h"

#if COMPILE_WITH_RHI_DX12

#if !defined(TIX_SHIPPING)
// Include pix function for dx12 profile
#include <WinPixEventRuntime\pix3.h>
#pragma comment (lib, "WinPixEventRuntime.lib")
#define BEGIN_EVENT(CmdList, formatStr) PIXBeginEvent(CmdList, 0xffffffff, formatStr)
#define END_EVENT(CmdList) PIXEndEvent(CmdList)
#else
#define BEGIN_EVENT(CmdList, formatStr)
#define END_EVENT(CmdList)
#endif

namespace tix
{
	FRHICmdListDx12::FRHICmdListDx12(ERHICmdList InType)
		: FRHICmdList(InType)
		, RHIDx12(nullptr)
		, CurrentFrameIndex(0)
	{
		Barriers.reserve(16);
	}

	FRHICmdListDx12::~FRHICmdListDx12()
	{
		// delete frame resource holders
		for (auto p : ResourceHolders)
		{
			ti_delete p;
		}
	}

	void FRHICmdListDx12::Init(FRHIDx12* InRHIDx12, const TString& InNamePrefix, int32 BufferCount)
	{
		RHIDx12 = InRHIDx12;
		ID3D12Device* D3dDevice = InRHIDx12->GetD3dDevice();

		auto GetCmdListType = [](ERHICmdList InType)
		{
			switch (InType)
			{
			case ERHICmdList::Direct:
				return D3D12_COMMAND_LIST_TYPE_DIRECT;
			case ERHICmdList::Compute:
				return D3D12_COMMAND_LIST_TYPE_COMPUTE;
			case ERHICmdList::Copy:
				return D3D12_COMMAND_LIST_TYPE_COPY;
			default:
				RuntimeFail();
				return D3D12_COMMAND_LIST_TYPE_DIRECT;
			}
		};
		// Create the command queue. allocator, command list and fence
		// Command Queue
		D3D12_COMMAND_LIST_TYPE CmdListType = GetCmdListType(Type);
		D3D12_COMMAND_QUEUE_DESC QueueDesc = {};
		QueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		QueueDesc.Type = CmdListType;

		VALIDATE_HRESULT(D3dDevice->CreateCommandQueue(&QueueDesc, IID_PPV_ARGS(&CommandQueue)));
		CommandQueue->SetName(FromString(InNamePrefix + "-CmdQueue").c_str());

		// Command Allocators
		CommandAllocators.resize(BufferCount);
		char Name[128];
		for (int32 n = 0; n < BufferCount; n++)
		{
			sprintf_s(Name, 128, "%s-CmdAllocator%d", InNamePrefix.c_str(), n);
			VALIDATE_HRESULT(D3dDevice->CreateCommandAllocator(CmdListType, IID_PPV_ARGS(&CommandAllocators[n])));
			CommandAllocators[n]->SetName(FromString(Name).c_str());
		}

		// Command List
		VALIDATE_HRESULT(D3dDevice->CreateCommandList(
			0,
			CmdListType,
			CommandAllocators[0].Get(),
			nullptr,
			IID_PPV_ARGS(&CommandList)));
		CommandList->SetName(FromString(InNamePrefix + "-CmdList").c_str());
		VALIDATE_HRESULT(CommandList->Close());

		// Fence
		VALIDATE_HRESULT(D3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&CommandFence)));

		FenceValues.resize(BufferCount);
		FenceValues[0]++;
		for (int32 n = 0; n < BufferCount; n++)
		{
			FenceValues[n] = FenceValues[0];
		}

		FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (FenceEvent == nullptr)
		{
			HRESULT Hr = HRESULT_FROM_WIN32(GetLastError());
			RuntimeFail();
		}

		// Create frame resource holders
		ResourceHolders.resize(BufferCount);
		for (int32 i = 0; i < BufferCount; ++i)
		{
			ResourceHolders[i] = ti_new FFrameResourcesDx12;
		}
	}

	void FRHICmdListDx12::BeginFrame(int32 FrameIndex, ID3D12DescriptorHeap* Heap)
	{
		CurrentRenderTarget = nullptr;
		CurrentBoundResource.Reset();

		TI_ASSERT(Barriers.size() == 0);
		CurrentFrameIndex = FrameIndex;

		// Reset command list
		VALIDATE_HRESULT(CommandAllocators[CurrentFrameIndex]->Reset());
		VALIDATE_HRESULT(CommandList->Reset(CommandAllocators[CurrentFrameIndex].Get(), nullptr));

		// Set the descriptor heaps to be used by this frame.
		ID3D12DescriptorHeap* ppHeaps[] = { Heap };
		CommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
	}

	void FRHICmdListDx12::EndFrame()
	{
		ReleaseFrameResources();

		CurrentRenderTarget = nullptr;
		CurrentBoundResource.Reset();
	}

	void FRHICmdListDx12::SetBackbufferTarget(D3D12_CPU_DESCRIPTOR_HANDLE InRTView, D3D12_CPU_DESCRIPTOR_HANDLE InDSView)
	{
		TI_ASSERT(InRTView.ptr != 0 && InDSView.ptr != 0);
		CommandList->ClearRenderTargetView(InRTView, DirectX::Colors::CornflowerBlue, 0, nullptr);
		CommandList->ClearDepthStencilView(InDSView, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

		CommandList->OMSetRenderTargets(1, &InRTView, false, &InDSView);
	}

	void FRHICmdListDx12::Close()
	{
		VALIDATE_HRESULT(CommandList->Close());
	}

	void FRHICmdListDx12::Execute()
	{
		ID3D12CommandList* ppCommandLists[] = { CommandList.Get() };
		CommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	}

	void FRHICmdListDx12::WaitingForGpu()
	{
		// Schedule a Signal command in the queue.
		VALIDATE_HRESULT(CommandQueue->Signal(CommandFence.Get(), FenceValues[CurrentFrameIndex]));

		// Wait until the fence has been crossed.
		if (CommandFence->GetCompletedValue() < FenceValues[CurrentFrameIndex])
		{
			VALIDATE_HRESULT(CommandFence->SetEventOnCompletion(FenceValues[CurrentFrameIndex], FenceEvent));
			WaitForSingleObjectEx(FenceEvent, INFINITE, FALSE);
		}

		// Increment the fence value for the current frame.
		FenceValues[CurrentFrameIndex]++;
	}

	void FRHICmdListDx12::MoveToNextFrame(int32 NextFrameIndex)
	{
		TI_ASSERT(NextFrameIndex == (CurrentFrameIndex + 1) % CommandAllocators.size());
		// Schedule a Signal command in the queue.
		const uint64 CurrentFenceValue = FenceValues[CurrentFrameIndex];
		VALIDATE_HRESULT(CommandQueue->Signal(CommandFence.Get(), CurrentFenceValue));

		// Check to see if the next frame is ready to start.
		if (CommandFence->GetCompletedValue() < FenceValues[NextFrameIndex])
		{
			VALIDATE_HRESULT(CommandFence->SetEventOnCompletion(FenceValues[NextFrameIndex], FenceEvent));
			WaitForSingleObjectEx(FenceEvent, INFINITE, FALSE);
		}

		// Set the fence value for the next frame.
		FenceValues[NextFrameIndex] = CurrentFenceValue + 1;
	}

	void FRHICmdListDx12::FlushBarriers()
	{
		if (!Barriers.empty())
		{
			CommandList->ResourceBarrier((uint32)Barriers.size(), Barriers.data());
			Barriers.clear();
		}
	}

	void FRHICmdListDx12::Transition(
		ID3D12Resource* pResource,
		D3D12_RESOURCE_STATES stateBefore,
		D3D12_RESOURCE_STATES stateAfter
	)
	{
		TI_ASSERT(pResource != nullptr);
		Barriers.push_back(D3D12_RESOURCE_BARRIER());
		D3D12_RESOURCE_BARRIER& Barrier = Barriers.back();
		Barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		Barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		Barrier.Transition.pResource = pResource;
		Barrier.Transition.StateBefore = stateBefore;
		Barrier.Transition.StateAfter = stateAfter;
		Barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	}

	void FRHICmdListDx12::UAVBarrier(ID3D12Resource* pResource)
	{
		TI_ASSERT(pResource != nullptr);
		Barriers.push_back(D3D12_RESOURCE_BARRIER());
		D3D12_RESOURCE_BARRIER& Barrier = Barriers.back();
		Barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		Barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		Barrier.UAV.pResource = pResource;
	}

	void FRHICmdListDx12::UpdateD3D12Resource(
		_In_ ID3D12Resource* pDestinationResource,
		_In_ ID3D12Resource* pIntermediate,
		uint32 NumSubResources,
		const D3D12_SUBRESOURCE_DATA* pData
	)
	{
		UpdateSubresources(
			pDestinationResource,
			pIntermediate,
			0, 0, NumSubResources,
			pData
		);
	}

	void FRHICmdListDx12::HoldResourceReference(FRenderResourcePtr InResource)
	{
		ResourceHolders[CurrentFrameIndex]->HoldReference(InResource);
	}

	void FRHICmdListDx12::HoldResourceReference(ComPtr<ID3D12Resource> InDxResource)
	{
		ResourceHolders[CurrentFrameIndex]->HoldDxReference(InDxResource);
	}

	void FRHICmdListDx12::ReleaseFrameResources()
	{
		// Release resources references for next drawing
		ResourceHolders[CurrentFrameIndex]->RemoveAllReferences();
	}

	//------------------------------------------------------------------------------------------------
	// All arrays must be populated (e.g. by calling GetCopyableFootprints)
	uint64 FRHICmdListDx12::UpdateSubresources(
		_In_ ID3D12Resource* pDestinationResource,
		_In_ ID3D12Resource* pIntermediate,
		_In_range_(0, D3D12_REQ_SUBRESOURCES) uint32 FirstSubresource,
		_In_range_(0, D3D12_REQ_SUBRESOURCES - FirstSubresource) uint32 NumSubresources,
		uint64 RequiredSize,
		_In_reads_(NumSubresources) const D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts,
		_In_reads_(NumSubresources) const uint32* pNumRows,
		_In_reads_(NumSubresources) const uint64* pRowSizesInBytes,
		_In_reads_(NumSubresources) const D3D12_SUBRESOURCE_DATA* pSrcData)
	{
		// Minor validation
		D3D12_RESOURCE_DESC IntermediateDesc = pIntermediate->GetDesc();
		D3D12_RESOURCE_DESC DestinationDesc = pDestinationResource->GetDesc();
		if (IntermediateDesc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER ||
			IntermediateDesc.Width < RequiredSize + pLayouts[0].Offset ||
			RequiredSize >(SIZE_T) - 1 ||
			(DestinationDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER &&
				(FirstSubresource != 0 || NumSubresources != 1)))
		{
			return 0;
		}

		uint8* pData;
		HRESULT hr = pIntermediate->Map(0, nullptr, reinterpret_cast<void**>(&pData));
		if (FAILED(hr))
		{
			return 0;
		}

		for (uint32 i = 0; i < NumSubresources; ++i)
		{
			if (pRowSizesInBytes[i] > (SIZE_T)-1) return 0;
			D3D12_MEMCPY_DEST DestData = { pData + pLayouts[i].Offset, pLayouts[i].Footprint.RowPitch, pLayouts[i].Footprint.RowPitch * pNumRows[i] };
			MemcpySubresource(&DestData, &pSrcData[i], (SIZE_T)pRowSizesInBytes[i], pNumRows[i], pLayouts[i].Footprint.Depth);
		}
		pIntermediate->Unmap(0, nullptr);

		if (DestinationDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
		{
			CD3DX12_BOX SrcBox(uint32(pLayouts[0].Offset), uint32(pLayouts[0].Offset + pLayouts[0].Footprint.Width));
			CommandList->CopyBufferRegion(
				pDestinationResource, 0, pIntermediate, pLayouts[0].Offset, pLayouts[0].Footprint.Width);
		}
		else
		{
			for (uint32 i = 0; i < NumSubresources; ++i)
			{
				CD3DX12_TEXTURE_COPY_LOCATION Dst(pDestinationResource, i + FirstSubresource);
				CD3DX12_TEXTURE_COPY_LOCATION Src(pIntermediate, pLayouts[i]);
				CommandList->CopyTextureRegion(&Dst, 0, 0, 0, &Src, nullptr);
			}
		}
		return RequiredSize;
	}

	//------------------------------------------------------------------------------------------------
	// Heap-allocating UpdateSubresources implementation
	uint64 FRHICmdListDx12::UpdateSubresources(
		_In_ ID3D12Resource* pDestinationResource,
		_In_ ID3D12Resource* pIntermediate,
		uint64 IntermediateOffset,
		_In_range_(0, D3D12_REQ_SUBRESOURCES) uint32 FirstSubresource,
		_In_range_(0, D3D12_REQ_SUBRESOURCES - FirstSubresource) uint32 NumSubresources,
		_In_reads_(NumSubresources) const D3D12_SUBRESOURCE_DATA* pSrcData)
	{
		uint64 RequiredSize = 0;
		uint64 MemToAlloc = static_cast<uint64>(sizeof(D3D12_PLACED_SUBRESOURCE_FOOTPRINT) + sizeof(uint32) + sizeof(uint64)) * NumSubresources;
		if (MemToAlloc > SIZE_MAX)
		{
			return 0;
		}
		void* pMem = HeapAlloc(GetProcessHeap(), 0, static_cast<SIZE_T>(MemToAlloc));
		if (pMem == nullptr)
		{
			return 0;
		}
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts = reinterpret_cast<D3D12_PLACED_SUBRESOURCE_FOOTPRINT*>(pMem);
		uint64* pRowSizesInBytes = reinterpret_cast<uint64*>(pLayouts + NumSubresources);
		uint32* pNumRows = reinterpret_cast<uint32*>(pRowSizesInBytes + NumSubresources);

		D3D12_RESOURCE_DESC Desc = pDestinationResource->GetDesc();
		ID3D12Device* pDevice;
		pDestinationResource->GetDevice(__uuidof(*pDevice), reinterpret_cast<void**>(&pDevice));
		pDevice->GetCopyableFootprints(&Desc, FirstSubresource, NumSubresources, IntermediateOffset, pLayouts, pNumRows, pRowSizesInBytes, &RequiredSize);
		pDevice->Release();

		uint64 Result = UpdateSubresources(pDestinationResource, pIntermediate, FirstSubresource, NumSubresources, RequiredSize, pLayouts, pNumRows, pRowSizesInBytes, pSrcData);
		HeapFree(GetProcessHeap(), 0, pMem);
		return Result;
	}

	/////////////////////////////////////////////////////////////
	void FRHICmdListDx12::BeginEvent(const int8* InEventName)
	{
		BEGIN_EVENT(CommandList.Get(), InEventName);
	}

	void FRHICmdListDx12::BeginEvent(const int8* InEventName, int32 Index)
	{
		int8 TempName[64];
		sprintf(TempName, "%s_%d", InEventName, Index);
		BEGIN_EVENT(CommandList.Get(), TempName);
	}

	void FRHICmdListDx12::EndEvent()
	{
		END_EVENT(CommandList.Get());
	}

	void FRHICmdListDx12::CopyTextureRegion(FGPUBufferPtr DstBuffer, FGPUTexturePtr SrcTexture, uint32 RowPitch)
	{
		FlushBarriers();
		FGPUBufferDx12* GPUBufferDx12 = static_cast<FGPUBufferDx12*>(DstBuffer.get());
		FGPUTextureDx12* GPUTextureDx12 = static_cast<FGPUTextureDx12*>(SrcTexture.get());
		D3D12_RESOURCE_DESC Desc = GPUTextureDx12->Resource->GetDesc();

		D3D12_PLACED_SUBRESOURCE_FOOTPRINT Footprint;
		Footprint.Offset = 0;
		Footprint.Footprint.Format = Desc.Format;
		Footprint.Footprint.Width = (uint32)Desc.Width;
		Footprint.Footprint.Height = (uint32)Desc.Height;
		Footprint.Footprint.Depth = Desc.DepthOrArraySize;
		Footprint.Footprint.RowPitch = RowPitch;
		TI_ASSERT(Footprint.Footprint.RowPitch % D3D12_TEXTURE_DATA_PITCH_ALIGNMENT == 0);
		CD3DX12_TEXTURE_COPY_LOCATION Dst(GPUBufferDx12->Resource.Get(), Footprint);
		CD3DX12_TEXTURE_COPY_LOCATION Src(GPUTextureDx12->Resource.Get(), 0);
		CommandList->CopyTextureRegion(&Dst, 0, 0, 0, &Src, nullptr);

		HoldResourceReference(GPUTextureDx12->Resource);
		HoldResourceReference(GPUBufferDx12->Resource);
	}

	void FRHICmdListDx12::CopyGPUBuffer(FGPUBufferPtr DstBuffer, FGPUBufferPtr SrcBuffer)
	{
		FlushBarriers();
		FGPUBufferDx12* DstDx12 = static_cast<FGPUBufferDx12*>(DstBuffer.get());
		FGPUBufferDx12* SrcDx12 = static_cast<FGPUBufferDx12*>(SrcBuffer.get());

		CommandList->CopyResource(DstDx12->GetResource(), SrcDx12->GetResource());

		HoldResourceReference(DstDx12->Resource);
		HoldResourceReference(SrcDx12->Resource);
	}

	void FRHICmdListDx12::UAVBarrier(FBottomLevelAccelerationStructurePtr BLAS)
	{
		FBottomLevelAccelerationStructureDx12* ASDx12 = static_cast<FBottomLevelAccelerationStructureDx12*>(BLAS.get());
		UAVBarrier(ASDx12->GetASResource());
	}

	void FRHICmdListDx12::SetGraphicsPipeline(FPipelinePtr InPipeline)
	{
		if (CurrentBoundResource.Pipeline != InPipeline)
		{
			FPipelineDx12* PipelineDx12 = static_cast<FPipelineDx12*>(InPipeline.get());
			FShaderPtr Shader = InPipeline->GetShader();
			TI_ASSERT(Shader->GetShaderType() == EST_RENDER);
			FShaderBindingPtr ShaderBinding = Shader->GetShaderBinding();
			TI_ASSERT(ShaderBinding != nullptr);

			if (CurrentBoundResource.ShaderBinding != ShaderBinding)
			{
				FRootSignatureDx12* RSDx12 = static_cast<FRootSignatureDx12*>(ShaderBinding.get());
				CommandList->SetGraphicsRootSignature(RSDx12->Get());
				CurrentBoundResource.ShaderBinding = ShaderBinding;
			}

			CommandList->SetPipelineState(PipelineDx12->PipelineState.Get());

			HoldResourceReference(InPipeline);

			CurrentBoundResource.Pipeline = InPipeline;
		}
	}

	void FRHICmdListDx12::SetVertexBuffer(FVertexBufferPtr InVB, FInstanceBufferPtr InInsB)
	{
		const TVertexBufferDesc& VBDesc = InVB->GetDesc();
		if (CurrentBoundResource.PrimitiveType != VBDesc.PrimitiveType)
		{
			CommandList->IASetPrimitiveTopology(GetDx12Topology(VBDesc.PrimitiveType));
			CurrentBoundResource.PrimitiveType = VBDesc.PrimitiveType;
		}

		D3D12_VERTEX_BUFFER_VIEW VBView = FRHIDx12::GetVertexBufferView(InVB);
		if (InInsB == nullptr)
		{
			CommandList->IASetVertexBuffers(0, 1, &VBView);
		}
		else
		{
			D3D12_VERTEX_BUFFER_VIEW InsView = FRHIDx12::GetInstanceBufferView(InInsB);
			D3D12_VERTEX_BUFFER_VIEW Views[2] =
			{
				VBView,
				InsView
			};
			CommandList->IASetVertexBuffers(0, 2, Views);
			HoldResourceReference(InInsB);
		}

		HoldResourceReference(InVB);
	}

	void FRHICmdListDx12::SetIndexBuffer(FIndexBufferPtr InIB)
	{
		D3D12_INDEX_BUFFER_VIEW IBView = FRHIDx12::GetIndexBufferView(InIB);
		CommandList->IASetIndexBuffer(&IBView);
		HoldResourceReference(InIB);
	}

	void FRHICmdListDx12::SetUniformBuffer(E_SHADER_STAGE, int32 BindIndex, FUniformBufferPtr InUniformBuffer)
	{
		FGPUBufferDx12* BufferDx12 = static_cast<FGPUBufferDx12*>(InUniformBuffer->GetGPUResource().get());

		// Bind the current frame's constant buffer to the pipeline.
		CommandList->SetGraphicsRootConstantBufferView(BindIndex, BufferDx12->GetResource()->GetGPUVirtualAddress());

		HoldResourceReference(InUniformBuffer);
	}

	void FRHICmdListDx12::SetRenderResourceTable(int32 BindIndex, FRenderResourceTablePtr RenderResourceTable)
	{
		D3D12_GPU_DESCRIPTOR_HANDLE Descriptor = RHIDx12->GetGpuDescriptorHandle(RenderResourceTable, RenderResourceTable->GetStartIndex());
		CommandList->SetGraphicsRootDescriptorTable(BindIndex, Descriptor);

		HoldResourceReference(RenderResourceTable);
	}

	void FRHICmdListDx12::SetArgumentBuffer(int32 InBindIndex, FArgumentBufferPtr InArgumentBuffer)
	{
		TI_ASSERT(InBindIndex >= 0);
		FArgumentBufferDx12* ArgDx12 = static_cast<FArgumentBufferDx12*>(InArgumentBuffer.get());
		SetRenderResourceTable(InBindIndex, ArgDx12->ResourceTable);
	}

	void FRHICmdListDx12::SetGPUBufferState(FGPUBufferPtr GPUBuffer, EGPUResourceState NewState)
	{
		FGPUBufferDx12* GPUBufferDx12 = static_cast<FGPUBufferDx12*>(GPUBuffer.get());

		if (GPUBufferDx12->ResourceState == NewState)
			return;

		D3D12_RESOURCE_STATES StateBefore = GetDx12ResourceState(GPUBufferDx12->ResourceState);
		D3D12_RESOURCE_STATES StateAfter = GetDx12ResourceState(NewState);
		Transition(
			GPUBufferDx12->Resource.Get(),
			StateBefore,
			StateAfter);
		GPUBufferDx12->ResourceState = NewState;
	}

	void FRHICmdListDx12::SetGPUTextureState(FGPUTexturePtr GPUTexture, EGPUResourceState NewState)
	{
		FGPUTextureDx12* GPUTextureDx12 = static_cast<FGPUTextureDx12*>(GPUTexture.get());

		if (GPUTextureDx12->ResourceState == NewState)
			return;

		D3D12_RESOURCE_STATES StateBefore = GetDx12ResourceState(GPUTextureDx12->ResourceState);
		D3D12_RESOURCE_STATES StateAfter = GetDx12ResourceState(NewState);
		Transition(
			GPUTextureDx12->Resource.Get(),
			StateBefore,
			StateAfter);
		GPUTextureDx12->ResourceState = NewState;
	}

	void FRHICmdListDx12::SetStencilRef(uint32 InRefValue)
	{
		CommandList->OMSetStencilRef(InRefValue);
	}

	void FRHICmdListDx12::DrawPrimitiveInstanced(uint32 VertexCount, uint32 InstanceCount, uint32 InstanceOffset)
	{
		FlushBarriers();
		CommandList->DrawInstanced(VertexCount, InstanceCount, 0, 0);

		//FStats::Stats.TrianglesRendered += MeshBuffer->GetIndicesCount() / 3 * InstanceCount;
	}

	void FRHICmdListDx12::DrawPrimitiveIndexedInstanced(uint32 IndexCount, uint32 InstanceCount, uint32 InstanceOffset)
	{
		FlushBarriers();
		CommandList->DrawIndexedInstanced(IndexCount, InstanceCount, 0, 0, InstanceOffset);

		FStats::Stats.TrianglesRendered += IndexCount / 3 * InstanceCount;
	}

	void FRHICmdListDx12::DrawPrimitiveIndexedInstanced(uint32 IndexCount, uint32 InstanceCount, uint32 IndexOffset, uint32 InstanceOffset)
	{
		FlushBarriers();
		CommandList->DrawIndexedInstanced(IndexCount, InstanceCount, IndexOffset, 0, InstanceOffset);

		FStats::Stats.TrianglesRendered += IndexCount / 3 * InstanceCount;
	}

	void FRHICmdListDx12::SetTilePipeline(FPipelinePtr InPipeline)
	{
		TI_ASSERT(0);
	}

	void FRHICmdListDx12::SetTileBuffer(int32 BindIndex, FUniformBufferPtr InUniformBuffer)
	{
		TI_ASSERT(0);
	}

	void FRHICmdListDx12::DispatchTile(const FInt3& GroupSize)
	{
		TI_ASSERT(0);
	}

	void FRHICmdListDx12::SetComputePipeline(FPipelinePtr InPipeline)
	{
		FPipelineDx12* PipelineDx12 = static_cast<FPipelineDx12*>(InPipeline.get());
		FShaderPtr Shader = InPipeline->GetShader();
		TI_ASSERT(Shader->GetShaderType() == EST_COMPUTE);
		FShaderBindingPtr ShaderBinding = Shader->GetShaderBinding();
		TI_ASSERT(ShaderBinding != nullptr);

		CommandList->SetPipelineState(PipelineDx12->PipelineState.Get());

		FRootSignatureDx12* RSDx12 = static_cast<FRootSignatureDx12*>(ShaderBinding.get());
		CommandList->SetComputeRootSignature(RSDx12->Get());

		HoldResourceReference(InPipeline);
	}

	void FRHICmdListDx12::SetComputeConstant(int32 BindIndex, const FUInt4& InValue)
	{
		CommandList->SetComputeRoot32BitConstants(BindIndex, 4, &InValue, 0);
	}

	void FRHICmdListDx12::SetComputeConstant(int32 BindIndex, const FFloat4& InValue)
	{
		CommandList->SetComputeRoot32BitConstants(BindIndex, 4, &InValue, 0);
	}

	void FRHICmdListDx12::SetComputeConstantBuffer(int32 BindIndex, FUniformBufferPtr InUniformBuffer, uint32 BufferOffset)
	{
		FGPUBufferDx12* BufferDx12 = static_cast<FGPUBufferDx12*>(InUniformBuffer->GetGPUResource().get());

		// Bind the current frame's constant buffer to the pipeline.
		CommandList->SetComputeRootConstantBufferView(BindIndex, BufferDx12->GetResource()->GetGPUVirtualAddress() + BufferOffset);

		HoldResourceReference(InUniformBuffer);
	}

	void FRHICmdListDx12::SetComputeShaderResource(int32 BindIndex, FUniformBufferPtr InUniformBuffer, uint32 BufferOffset)
	{
		FGPUBufferDx12* BufferDx12 = static_cast<FGPUBufferDx12*>(InUniformBuffer->GetGPUResource().get());

		// Bind the current frame's constant buffer to the pipeline.
		CommandList->SetComputeRootShaderResourceView(BindIndex, BufferDx12->GetResource()->GetGPUVirtualAddress() + BufferOffset);

		HoldResourceReference(InUniformBuffer);
	}

	void FRHICmdListDx12::SetComputeResourceTable(int32 BindIndex, FRenderResourceTablePtr RenderResourceTable)
	{
		D3D12_GPU_DESCRIPTOR_HANDLE Descriptor = RHIDx12->GetGpuDescriptorHandle(RenderResourceTable, RenderResourceTable->GetStartIndex());
		CommandList->SetComputeRootDescriptorTable(BindIndex, Descriptor);

		HoldResourceReference(RenderResourceTable);
	}

	void FRHICmdListDx12::SetComputeArgumentBuffer(int32 InBindIndex, FArgumentBufferPtr InArgumentBuffer)
	{
		FArgumentBufferDx12* ArgDx12 = static_cast<FArgumentBufferDx12*>(InArgumentBuffer.get());
		SetComputeResourceTable(InBindIndex, ArgDx12->ResourceTable);
	}

	void FRHICmdListDx12::SetComputeTexture(int32 BindIndex, FTexturePtr InTexture)
	{
		TI_ASSERT(0);
	}

	void FRHICmdListDx12::DispatchCompute(const FInt3& GroupSize, const FInt3& GroupCount)
	{
		FlushBarriers();
		CommandList->Dispatch(GroupCount.X, GroupCount.Y, GroupCount.Z);
	}

	void FRHICmdListDx12::SetRtxPipeline(FRtxPipelinePtr RtxPipeline)
	{
		TI_ASSERT(0);
		//FRtxPipelineDx12* RtxPipelineDx12 = static_cast<FRtxPipelineDx12*>(RtxPipeline.get());
		//FShaderPtr ShaderLib = RtxPipeline->GetShaderLib();

		//// Set Rtx Pipeline
		//DXR->DXRCommandList->SetPipelineState1(RtxPipelineDx12->StateObject.Get());

		//// Bind Global root signature
		//FRootSignatureDx12* GlobalRSDx12 = static_cast<FRootSignatureDx12*>(ShaderLib->GetShaderBinding().get());
		//DXR->DXRCommandList->SetComputeRootSignature(GlobalRSDx12->Get());
	}

	void FRHICmdListDx12::TraceRays(FRtxPipelinePtr RtxPipeline, const FInt3& Size)
	{
		FlushBarriers();
		FRtxPipelineDx12* RtxPipelineDx12 = static_cast<FRtxPipelineDx12*>(RtxPipeline.get());
		FGPUBufferDx12* ShaderTableBuffer = static_cast<FGPUBufferDx12*>(RtxPipelineDx12->ShaderTable->GetGPUResource().get());
		FShaderPtr ShaderLib = RtxPipeline->GetShaderLib();

		D3D12_DISPATCH_RAYS_DESC RaytraceDesc = {};
		RaytraceDesc.Width = Size.X;
		RaytraceDesc.Height = Size.Y;
		RaytraceDesc.Depth = Size.Z;

		// RayGen is the first entry in the shader-table
		RaytraceDesc.RayGenerationShaderRecord.StartAddress =
			ShaderTableBuffer->GetResource()->GetGPUVirtualAddress() + RtxPipelineDx12->RayGenShaderOffsetAndSize.X;
		RaytraceDesc.RayGenerationShaderRecord.SizeInBytes = RtxPipelineDx12->RayGenShaderOffsetAndSize.Y;

		// Miss is the second entry in the shader-table
		RaytraceDesc.MissShaderTable.StartAddress =
			ShaderTableBuffer->GetResource()->GetGPUVirtualAddress() + RtxPipelineDx12->MissShaderOffsetAndSize.X;
		RaytraceDesc.MissShaderTable.StrideInBytes = RtxPipelineDx12->MissShaderOffsetAndSize.Y;
		RaytraceDesc.MissShaderTable.SizeInBytes = RtxPipelineDx12->MissShaderOffsetAndSize.Y;   // Only a s single miss-entry

		// Hit is the third entry in the shader-table
		RaytraceDesc.HitGroupTable.StartAddress =
			ShaderTableBuffer->GetResource()->GetGPUVirtualAddress() + RtxPipelineDx12->HitGroupOffsetAndSize.X;
		RaytraceDesc.HitGroupTable.StrideInBytes = RtxPipelineDx12->HitGroupOffsetAndSize.Y;
		RaytraceDesc.HitGroupTable.SizeInBytes = RtxPipelineDx12->HitGroupOffsetAndSize.Y;

		// Dispatch
		TI_ASSERT(0);
		//DXR->DXRCommandList->DispatchRays(&RaytraceDesc);
	}

	void FRHICmdListDx12::ExecuteGPUDrawCommands(FGPUCommandBufferPtr GPUCommandBuffer)
	{
		if (GPUCommandBuffer->GetEncodedCommandsCount() > 0)
		{
			FlushBarriers();
			FGPUCommandSignatureDx12* SignatueDx12 = static_cast<FGPUCommandSignatureDx12*>(GPUCommandBuffer->GetGPUCommandSignature().get());

			// Set Primitive Topology
			CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			// Execute indirect draw.
			FGPUBufferDx12* BufferDx12 = static_cast<FGPUBufferDx12*>(GPUCommandBuffer->GetGPUResource().get());
			if ((GPUCommandBuffer->GetCBFlag() & (uint32)EGPUResourceFlag::UavCounter) != 0)
			{
				uint32 CounterOffset = FRHIDx12::GetUavCounterOffset(GPUCommandBuffer->GetTotalBufferSize());
				CommandList->ExecuteIndirect(
					SignatueDx12->CommandSignature.Get(),
					GPUCommandBuffer->GetEncodedCommandsCount(),
					BufferDx12->GetResource(),
					0,
					BufferDx12->GetResource(),
					CounterOffset);
			}
			else
			{
				CommandList->ExecuteIndirect(
					SignatueDx12->CommandSignature.Get(),
					GPUCommandBuffer->GetEncodedCommandsCount(),
					BufferDx12->GetResource(),
					0,
					nullptr,
					0);
			}

			HoldResourceReference(GPUCommandBuffer);
		}
	}

	void FRHICmdListDx12::ExecuteGPUComputeCommands(FGPUCommandBufferPtr GPUCommandBuffer)
	{
		if (GPUCommandBuffer->GetEncodedCommandsCount() > 0)
		{
			FlushBarriers();
			FGPUCommandBufferDx12* GPUCommandBufferDx12 = static_cast<FGPUCommandBufferDx12*>(GPUCommandBuffer.get());
			FGPUCommandSignatureDx12* SignatueDx12 = static_cast<FGPUCommandSignatureDx12*>(GPUCommandBuffer->GetGPUCommandSignature().get());

			// Execute indirect draw.
			FGPUBufferDx12* BufferDx12 = static_cast<FGPUBufferDx12*>(GPUCommandBuffer->GetGPUResource().get());
			if ((GPUCommandBuffer->GetCBFlag() & (uint32)EGPUResourceFlag::UavCounter) != 0)
			{
				uint32 CounterOffset = FRHIDx12::GetUavCounterOffset(GPUCommandBuffer->GetTotalBufferSize());
				CommandList->ExecuteIndirect(
					SignatueDx12->CommandSignature.Get(),
					GPUCommandBuffer->GetEncodedCommandsCount(),
					BufferDx12->GetResource(),
					0,
					BufferDx12->GetResource(),
					CounterOffset);
			}
			else
			{
				CommandList->ExecuteIndirect(
					SignatueDx12->CommandSignature.Get(),
					GPUCommandBuffer->GetEncodedCommandsCount(),
					BufferDx12->GetResource(),
					0,
					nullptr,
					0);
			}

			HoldResourceReference(GPUCommandBuffer);
		}
	}

	void FRHICmdListDx12::SetViewport(const FRecti& VP)
	{
		FRHICmdList::SetViewport(VP);

		D3D12_VIEWPORT ViewportDx = { float(VP.Left), float(VP.Upper), float(VP.GetWidth()), float(VP.GetHeight()), 0.f, 1.f };
		CommandList->RSSetViewports(1, &ViewportDx);
	}
	
	void FRHICmdListDx12::SetScissorRect(const FRecti& InRect)
	{
		FRHICmdList::SetScissorRect(InRect);
		D3D12_RECT DxRect = { InRect.Left, InRect.Upper, InRect.Right, InRect.Lower };
		CommandList->RSSetScissorRects(1, &DxRect);
	}

	void FRHICmdListDx12::BeginRenderToRenderTarget(FRenderTargetPtr RT, const int8* PassName, uint32 MipLevel)
	{
		END_EVENT(CommandList.Get());

		auto SetRenderTarget = [this](FRenderTargetPtr RT, uint32 MipLevel)
		{
			// Transition Color buffer to D3D12_RESOURCE_STATE_RENDER_TARGET
			FRenderTargetDx12* RTDx12 = static_cast<FRenderTargetDx12*>(RT.get());
			const int32 CBCount = RT->GetColorBufferCount();
			uint32 RtMips = 0;
			for (int32 cb = 0; cb < CBCount; ++cb)
			{
				FTexturePtr Texture = RT->GetColorBuffer(cb).Texture;
				RtMips = Texture->GetDesc().Mips;
				TI_ASSERT(Texture != nullptr);	// Color can be NULL ?
				FGPUTexturePtr GPUTexture = static_cast<FGPUTexture*>(Texture->GetGPUResource().get());
				SetGPUTextureState(GPUTexture, EGPUResourceState::RenderTarget);
			}
			TI_ASSERT(CBCount == 0 || MipLevel < RtMips);

			// Transition Depth buffer to D3D12_RESOURCE_STATE_DEPTH_WRITE
			{
				FTexturePtr Texture = RT->GetDepthStencilBuffer().Texture;
				if (Texture != nullptr)
				{
					TI_ASSERT(CBCount == 0 || Texture->GetDesc().Mips == RtMips);
					RtMips = Texture->GetDesc().Mips;
					FGPUTexturePtr GPUTexture = static_cast<FGPUTexture*>(Texture->GetGPUResource().get());
					SetGPUTextureState(GPUTexture, EGPUResourceState::DepthWrite);
				}
			}
			FlushBarriers();

			TVector<D3D12_CPU_DESCRIPTOR_HANDLE> RTVDescriptors;
			const D3D12_CPU_DESCRIPTOR_HANDLE* Rtv = nullptr;
			if (CBCount > 0)
			{
				FRenderResourceTablePtr ColorTable = RTDx12->RTColorTable;
				RTVDescriptors.reserve(CBCount);
				for (int32 cb = 0; cb < CBCount; ++cb)
				{
					D3D12_CPU_DESCRIPTOR_HANDLE Descriptor = RHIDx12->GetCpuDescriptorHandle(ColorTable, cb * RtMips + MipLevel);
					RTVDescriptors.push_back(Descriptor);
				}
				Rtv = RTVDescriptors.data();
			}

			const D3D12_CPU_DESCRIPTOR_HANDLE* Dsv = nullptr;
			D3D12_CPU_DESCRIPTOR_HANDLE DepthDescriptor;
			FRenderResourceTablePtr DepthTable = RTDx12->RTDepthTable;
			if (DepthTable != nullptr && DepthTable->GetTableSize() != 0)
			{
				DepthDescriptor = RHIDx12->GetCpuDescriptorHandle(DepthTable, MipLevel);
				Dsv = &DepthDescriptor;
			}

			// Set render target
			CommandList->OMSetRenderTargets(RT->GetColorBufferCount(), Rtv, false, Dsv);

			// Clear render target
			if (CBCount > 0)
			{
				for (int32 cb = 0; cb < CBCount; ++cb)
				{
					FTexturePtr Texture = RT->GetColorBuffer(cb).Texture;
					CommandList->ClearRenderTargetView(RTVDescriptors[cb], Texture->GetDesc().ClearColor.GetDataPtr(), 0, nullptr);
				}
			}
			if (Dsv != nullptr)
			{
				CommandList->ClearDepthStencilView(*Dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
			}
		};


		FRHICmdList::BeginRenderToRenderTarget(RT, PassName, MipLevel);
		BEGIN_EVENT(CommandList.Get(), PassName);
		SetRenderTarget(RT, MipLevel);
	}
}
#endif	// COMPILE_WITH_RHI_DX12