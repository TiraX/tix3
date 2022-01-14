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

	void FUniformBuffer::CreateGPUBuffer(TStreamPtr Data)
	{
		TI_ASSERT(IsRenderThread());
		TI_ASSERT(Buffer == nullptr);
		FRHI* RHI = FRHI::Get();

		// Readback flag auto used in FUniformBufferReadable, NEVER appear in FUniformBuffer
		TI_ASSERT((UBFlag & (uint32)EGPUResourceFlag::Readback) == 0);

		FGPUBufferDesc Desc;
		Desc.Flag = UBFlag;
		Desc.BufferSize = GetTotalBufferSize();

		// Create GPU resource and copy data
		Buffer = RHI->CreateGPUBuffer();
		Buffer->Init(Desc, Data);
		RHI->SetGPUBufferName(Buffer, GetResourceName());
	}

	/////////////////////////////////////////////////////////
	FUniformBufferReadable::FUniformBufferReadable(uint32 InStructureSizeInBytes, uint32 InElements, uint32 InUBFlag)
		: FUniformBuffer(InStructureSizeInBytes, InElements, InUBFlag)
	{
	}

	FUniformBufferReadable::~FUniformBufferReadable()
	{
	}

	void FUniformBufferReadable::CreateGPUBuffer(TStreamPtr Data)
	{
		FUniformBuffer::CreateGPUBuffer(Data);
		TI_ASSERT(ReadbackBuffer == nullptr);
		FRHI* RHI = FRHI::Get();

		FGPUBufferDesc Desc;
		Desc.Flag = (uint32)EGPUResourceFlag::Readback;
		Desc.BufferSize = GetTotalBufferSize();

		// Create GPU resource and copy data
		ReadbackBuffer = RHI->CreateGPUBuffer();
		ReadbackBuffer->Init(Desc, nullptr);
		RHI->SetGPUBufferName(ReadbackBuffer, GetResourceName() + "-Readback");

		// Set resource state to VertexAndConstantBuffer
		//RHI->SetGPUBufferState(GPUResourceVB, EGPUResourceState::VertexAndConstantBuffer);
	}

	void FUniformBufferReadable::PrepareDataForCPU()
	{
		// Copy data from Buffer to ReadbackBuffer
		FRHI* RHI = FRHI::Get();
		RHI->SetGPUBufferState(Buffer, EGPUResourceState::CopySource);
		RHI->CopyGPUBuffer(ReadbackBuffer, Buffer);
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