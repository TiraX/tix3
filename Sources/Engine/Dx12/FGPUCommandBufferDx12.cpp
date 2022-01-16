/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "FRHIDx12.h"
#include "FGPUCommandSignatureDx12.h"
#include "FGPUCommandBufferDx12.h"
#include "FGPUBufferDx12.h"

#if COMPILE_WITH_RHI_DX12
namespace tix
{
	FGPUCommandBufferDx12::FGPUCommandBufferDx12(FGPUCommandSignaturePtr Signature, uint32 InCommandsCount, uint32 InBufferFlag)
		: FGPUCommandBuffer(Signature, InCommandsCount, InBufferFlag)
		, CommandsEncoded(0)
	{
		// Alloc data for command buffer data
		FGPUCommandSignatureDx12 * GPUCommandSignatureDx12 = static_cast<FGPUCommandSignatureDx12*>(GetGPUCommandSignature().get());
		TI_ASSERT(GPUCommandSignatureDx12->GetCommandStrideInBytes() != 0);
		if ((InBufferFlag & (uint32)EGPUResourceFlag::Uav) == 0)
		{
			// UAV do not need command data
			CBData = ti_new TStream(GPUCommandSignatureDx12->GetCommandStrideInBytes() * InCommandsCount);
		}
		else
		{
			// UAV use counter for command count, take CommandsEncoded as the MAX count of commands
			CommandsEncoded = InCommandsCount;
		}
	}

	FGPUCommandBufferDx12::~FGPUCommandBufferDx12()
	{
		TI_ASSERT(IsRenderThread());
		CBData = nullptr;
	}

	uint32 FGPUCommandBufferDx12::GetEncodedCommandsCount() const
	{
		return CommandsEncoded;
	}

	void FGPUCommandBufferDx12::EncodeEmptyCommand(uint32 CommandIndex)
	{
		const uint32 CommandStride = GetGPUCommandSignature()->GetCommandStrideInBytes();
		uint32 CommandPos = CommandIndex * CommandStride;
		CBData->Seek(CommandPos);
		CBData->FillWithZero(CommandStride);
		CommandsEncoded = TMath::Max(CommandsEncoded, CommandIndex + 1);
	}

	void FGPUCommandBufferDx12::EncodeSetVertexBuffer(
		uint32 CommandIndex,
		uint32 ArgumentIndex,
		FVertexBufferPtr VertexBuffer)
	{
		FGPUCommandSignatureDx12 * GPUCommandSignatureDx12 = static_cast<FGPUCommandSignatureDx12*>(GetGPUCommandSignature().get());
		TI_ASSERT(GPUCommandSignatureDx12->GetCommandStrideInBytes() != 0);

		// Only support indexed triangle list.
		const TVertexBufferDesc& VBDesc = VertexBuffer->GetDesc();
		TI_ASSERT(VBDesc.PrimitiveType == EPrimitiveType::TriangleList);

		const TVector<E_GPU_COMMAND_TYPE>& CommandStructure = GPUCommandSignatureDx12->GetCommandStructure();
		E_GPU_COMMAND_TYPE CommandType = CommandStructure[ArgumentIndex];
		TI_ASSERT(CommandType == GPU_COMMAND_SET_VERTEX_BUFFER);
		uint32 CommandPos = CommandIndex * GPUCommandSignatureDx12->GetCommandStrideInBytes() + GPUCommandSignatureDx12->GetArgumentStrideOffset(ArgumentIndex);

		// Vertex buffer
		FRHIDx12* RHIDx12 = static_cast<FRHIDx12*>(FRHI::Get());
		D3D12_VERTEX_BUFFER_VIEW VBView = RHIDx12->GetVertexBufferView(VertexBuffer);
		CBData->Seek(CommandPos);
		CBData->Set(&VBView, sizeof(D3D12_VERTEX_BUFFER_VIEW));

		// Remember commands encoded
		CommandsEncoded = TMath::Max(CommandsEncoded, CommandIndex + 1);
	}

	void FGPUCommandBufferDx12::EncodeSetInstanceBuffer(
		uint32 CommandIndex,
		uint32 ArgumentIndex,
		FInstanceBufferPtr InstanceBuffer)
	{
		FGPUCommandSignatureDx12 * GPUCommandSignatureDx12 = static_cast<FGPUCommandSignatureDx12*>(GetGPUCommandSignature().get());
		TI_ASSERT(GPUCommandSignatureDx12->GetCommandStrideInBytes() != 0);

		const TVector<E_GPU_COMMAND_TYPE>& CommandStructure = GPUCommandSignatureDx12->GetCommandStructure();
		E_GPU_COMMAND_TYPE CommandType = CommandStructure[ArgumentIndex];
		TI_ASSERT(CommandType == GPU_COMMAND_SET_INSTANCE_BUFFER);
		uint32 CommandPos = CommandIndex * GPUCommandSignatureDx12->GetCommandStrideInBytes() + GPUCommandSignatureDx12->GetArgumentStrideOffset(ArgumentIndex);

		// Instance buffer
		FRHIDx12* RHIDx12 = static_cast<FRHIDx12*>(FRHI::Get());
		D3D12_VERTEX_BUFFER_VIEW InsBView = RHIDx12->GetInstanceBufferView(InstanceBuffer);
		CBData->Seek(CommandPos);
		CBData->Set(&InsBView, sizeof(D3D12_VERTEX_BUFFER_VIEW));
		
		// Remember commands encoded
		CommandsEncoded = TMath::Max(CommandsEncoded, CommandIndex + 1);
	}

	void FGPUCommandBufferDx12::EncodeSetIndexBuffer(
		uint32 CommandIndex,
		uint32 ArgumentIndex,
		FIndexBufferPtr IndexBuffer)
	{
		FGPUCommandSignatureDx12 * GPUCommandSignatureDx12 = static_cast<FGPUCommandSignatureDx12*>(GetGPUCommandSignature().get());
		TI_ASSERT(GPUCommandSignatureDx12->GetCommandStrideInBytes() != 0);

		const TVector<E_GPU_COMMAND_TYPE>& CommandStructure = GPUCommandSignatureDx12->GetCommandStructure();
		E_GPU_COMMAND_TYPE CommandType = CommandStructure[ArgumentIndex];
		TI_ASSERT(CommandType == GPU_COMMAND_SET_INDEX_BUFFER);
		uint32 CommandPos = CommandIndex * GPUCommandSignatureDx12->GetCommandStrideInBytes() + GPUCommandSignatureDx12->GetArgumentStrideOffset(ArgumentIndex);

		// Index buffer
		FRHIDx12* RHIDx12 = static_cast<FRHIDx12*>(FRHI::Get());
		D3D12_INDEX_BUFFER_VIEW IBView = RHIDx12->GetIndexBufferView(IndexBuffer);
		CBData->Seek(CommandPos);
		CBData->Set(&IBView, sizeof(D3D12_INDEX_BUFFER_VIEW));
		
		// Remember commands encoded
		CommandsEncoded = TMath::Max(CommandsEncoded, CommandIndex + 1);
	}

	void FGPUCommandBufferDx12::EncodeSetDrawIndexed(
		uint32 CommandIndex,
		uint32 ArgumentIndex,
		uint32 IndexCountPerInstance,
		uint32 InstanceCount,
		uint32 StartIndexLocation,
		uint32 BaseVertexLocation,
		uint32 StartInstanceLocation)
	{
		FGPUCommandSignatureDx12 * GPUCommandSignatureDx12 = static_cast<FGPUCommandSignatureDx12*>(GetGPUCommandSignature().get());
		TI_ASSERT(GPUCommandSignatureDx12->GetCommandStrideInBytes() != 0);

		// Draw index
		const TVector<E_GPU_COMMAND_TYPE>& CommandStructure = GPUCommandSignatureDx12->GetCommandStructure();
		E_GPU_COMMAND_TYPE CommandType = CommandStructure[ArgumentIndex];
		TI_ASSERT(CommandType == GPU_COMMAND_DRAW_INDEXED);
		uint32 CommandPos = CommandIndex * GPUCommandSignatureDx12->GetCommandStrideInBytes() + GPUCommandSignatureDx12->GetArgumentStrideOffset(ArgumentIndex);

		D3D12_DRAW_INDEXED_ARGUMENTS DrawIndexed;
		DrawIndexed.IndexCountPerInstance = IndexCountPerInstance;
		DrawIndexed.InstanceCount = InstanceCount;
		DrawIndexed.StartIndexLocation = StartIndexLocation;
		DrawIndexed.BaseVertexLocation = BaseVertexLocation;
		DrawIndexed.StartInstanceLocation = StartInstanceLocation;

		CBData->Seek(CommandPos);
		CBData->Set(&DrawIndexed, sizeof(D3D12_DRAW_INDEXED_ARGUMENTS));
		// Remember commands encoded
		CommandsEncoded = TMath::Max(CommandsEncoded, CommandIndex + 1);
	}

	void FGPUCommandBufferDx12::EncodeSetDispatch(
		uint32 CommandIndex,
		uint32 ArgumentIndex,
		uint32 ThreadGroupCountX,
		uint32 ThreadGroupCountY,
		uint32 ThreadGroupCountZ)
	{
		FGPUCommandSignatureDx12 * GPUCommandSignatureDx12 = static_cast<FGPUCommandSignatureDx12*>(GetGPUCommandSignature().get());
		TI_ASSERT(GPUCommandSignatureDx12->GetCommandStrideInBytes() != 0);

		// Draw index
		const TVector<E_GPU_COMMAND_TYPE>& CommandStructure = GPUCommandSignatureDx12->GetCommandStructure();
		E_GPU_COMMAND_TYPE CommandType = CommandStructure[ArgumentIndex];
		TI_ASSERT(CommandType == GPU_COMMAND_DISPATCH);
		uint32 CommandPos = CommandIndex * GPUCommandSignatureDx12->GetCommandStrideInBytes() + GPUCommandSignatureDx12->GetArgumentStrideOffset(ArgumentIndex);

		D3D12_DISPATCH_ARGUMENTS DispatchArg;
		DispatchArg.ThreadGroupCountX = ThreadGroupCountX;
		DispatchArg.ThreadGroupCountY = ThreadGroupCountY;
		DispatchArg.ThreadGroupCountZ = ThreadGroupCountZ;

		CBData->Seek(CommandPos);
		CBData->Set(&DispatchArg, sizeof(D3D12_DISPATCH_ARGUMENTS));
		// Remember commands encoded
		CommandsEncoded = TMath::Max(CommandsEncoded, CommandIndex + 1);
	}

	void FGPUCommandBufferDx12::EncodeSetShaderResrouce(
		uint32 CommandIndex,
		uint32 ArgumentIndex
	)
	{
		TI_ASSERT(0);
	}

	const void * FGPUCommandBufferDx12::GetCommandData(uint32 CommandIndex) const
	{
		TI_ASSERT(CommandIndex < CommandsEncoded);
		TI_ASSERT(GPUCommandSignature->GetCommandStrideInBytes() != 0);

		uint32 CommandPos = CommandIndex * GPUCommandSignature->GetCommandStrideInBytes();
		return CBData->GetBuffer() + CommandPos;
	}

	void FGPUCommandBufferDx12::SetCommandData(uint32 CommandIndex, const void* InData, uint32 InDataSize)
	{
		FGPUCommandSignatureDx12 * GPUCommandSignatureDx12 = static_cast<FGPUCommandSignatureDx12*>(GetGPUCommandSignature().get());
		TI_ASSERT(GPUCommandSignatureDx12->GetCommandStrideInBytes() != 0 && GPUCommandSignatureDx12->GetCommandStrideInBytes() == InDataSize);

		uint32 CommandPos = CommandIndex * GPUCommandSignatureDx12->GetCommandStrideInBytes();
		CBData->Seek(CommandPos);
		CBData->Set(InData, InDataSize);

		CommandsEncoded = TMath::Max(CommandsEncoded, CommandIndex + 1);
	}
}
#endif