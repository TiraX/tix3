/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "FUniformBuffer.h"

namespace tix
{
	FUniformBuffer::FUniformBuffer(uint32 InStructureSizeInBytes, uint32 InElements, uint32 InUBFlag)
		: FRenderResource(ERenderResourceType::UniformBuffer)
		, StructureSizeInBytes(InStructureSizeInBytes)
		, Elements(InElements)
		, UBFlag(InUBFlag)
	{
	}

	FUniformBuffer::~FUniformBuffer()
	{
	}

	void FUniformBuffer::CreateGPUBuffer(FRHICmdList* CmdList, TStreamPtr Data, EGPUResourceState TargetState)
	{
		TI_ASSERT(TThread::AccquireId() == CmdList->WorkingThread);
		TI_ASSERT(Buffer == nullptr);
		FRHI* RHI = FRHI::Get();

		// Readback flag auto used in FUniformBufferReadable, NEVER appear in FUniformBuffer
		TI_ASSERT((UBFlag & (uint32)EGPUResourceFlag::Readback) == 0);

		FGPUBufferDesc Desc;
		Desc.Flag = UBFlag;
		Desc.BufferSize = GetTotalBufferSize();

		// Create GPU resource and copy data
		Buffer = RHI->CreateGPUBuffer();
		Buffer->Init(CmdList, Desc, Data);
		RHI->SetGPUBufferName(Buffer, GetResourceName());

		if (TargetState != EGPUResourceState::Ignore)
			CmdList->SetGPUBufferState(Buffer, TargetState);
	}

	FUniformBufferPtr FUniformBuffer::CreateBuffer(
		FRHICmdList* RHICmdList,
		const TString& InName,
		uint32 InStructureSizeInBytes,
		uint32 InElements,
		uint32 Flags,
		TStreamPtr InInitData,
		EGPUResourceState TargetState)
	{
		FUniformBufferPtr Buffer = ti_new FUniformBuffer(InStructureSizeInBytes, InElements, Flags);
		Buffer->SetResourceName(InName);
		Buffer->CreateGPUBuffer(RHICmdList, InInitData, TargetState);
		return Buffer;
	}

	FUniformBufferPtr FUniformBuffer::CreateUavBuffer(
		FRHICmdList* RHICmdList,
		const TString& InName,
		uint32 InStructureSizeInBytes,
		uint32 InElements,
		TStreamPtr InInitData,
		EGPUResourceState TargetState)
	{
		return CreateBuffer(RHICmdList, InName, InStructureSizeInBytes, InElements, (uint32)EGPUResourceFlag::Uav, InInitData, TargetState);
	}

	FUniformBufferPtr FUniformBuffer::CreateReadableUavBuffer(
		FRHICmdList* RHICmdList,
		const TString& InName,
		uint32 InStructureSizeInBytes,
		uint32 InElements,
		TStreamPtr InInitData)
	{
		FUniformBufferPtr Buffer = ti_new FUniformBufferReadable(InStructureSizeInBytes, InElements, (uint32)EGPUResourceFlag::Uav);
		Buffer->SetResourceName(InName);
		Buffer->CreateGPUBuffer(RHICmdList, InInitData);
		return Buffer;
	}

	/////////////////////////////////////////////////////////////
	FUniformBufferReadable::FUniformBufferReadable(uint32 InStructureSizeInBytes, uint32 InElements, uint32 InUBFlag)
		: FUniformBuffer(InStructureSizeInBytes, InElements, InUBFlag)
	{
	}

	FUniformBufferReadable::~FUniformBufferReadable()
	{
	}

	void FUniformBufferReadable::CreateGPUBuffer(FRHICmdList* CmdList, TStreamPtr Data, EGPUResourceState TargetState)
	{
		FUniformBuffer::CreateGPUBuffer(CmdList, Data, TargetState);
		TI_ASSERT(ReadbackBuffer == nullptr);
		FRHI* RHI = FRHI::Get();

		FGPUBufferDesc Desc;
		Desc.Flag = (uint32)EGPUResourceFlag::Readback;
		Desc.BufferSize = GetTotalBufferSize();

		// Create GPU resource and copy data
		ReadbackBuffer = RHI->CreateGPUBuffer();
		ReadbackBuffer->Init(CmdList, Desc, nullptr);
		RHI->SetGPUBufferName(ReadbackBuffer, GetResourceName() + "-Readback");
	}

	void FUniformBufferReadable::PrepareDataForCPU(FRHICmdList* CmdList)
	{
		// Copy data from Buffer to ReadbackBuffer
		CmdList->SetGPUBufferState(Buffer, EGPUResourceState::CopySource);
		CmdList->CopyGPUBuffer(ReadbackBuffer, Buffer);
	}

	TStreamPtr FUniformBufferReadable::ReadBufferData()
	{
		if (ReadbackBuffer != nullptr)
		{
			FRHI* RHI = FRHI::Get();
			TStreamPtr Buffer = RHI->ReadGPUBufferToCPU(ReadbackBuffer);
			return Buffer;
		}
		return nullptr;
	}
}