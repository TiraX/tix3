/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"

namespace tix
{
	FVertexBuffer::FVertexBuffer()
		: FRenderResource(ERenderResourceType::VertexBuffer)
	{
	}

	FVertexBuffer::FVertexBuffer(const TVertexBufferDesc& InDesc)
		: FRenderResource(ERenderResourceType::VertexBuffer)
		, Desc(InDesc)
	{
		Desc.Stride = TVertexBuffer::GetStrideFromFormat(InDesc.VsFormat);
	}

	FVertexBuffer::~FVertexBuffer()
	{
	}

	void FVertexBuffer::CreateGPUBuffer(FRHICmdList* CmdList, TStreamPtr Data, EGPUResourceState TargetState)
	{
		TI_ASSERT(TThread::AccquireId() == CmdList->WorkingThread);
		TI_ASSERT(GPUResourceVB == nullptr);

		FGPUBufferDesc VBDesc;
		VBDesc.Flag = 0;
		VBDesc.BufferSize = Desc.VertexCount * Desc.Stride;

		// Create GPU resource and copy data
		FRHI* RHI = FRHI::Get();
		GPUResourceVB = RHI->CreateGPUBuffer();
		GPUResourceVB->Init(CmdList, VBDesc, Data);
		RHI->SetGPUBufferName(GPUResourceVB, GetResourceName());

		// Set resource state to VertexAndConstantBuffer
		if (TargetState != EGPUResourceState::Ignore)
			CmdList->SetGPUBufferState(GPUResourceVB, TargetState);
	}

	/////////////////////////////////////////////////////////////
	FIndexBuffer::FIndexBuffer()
		: FRenderResource(ERenderResourceType::IndexBuffer)
	{
	}

	FIndexBuffer::FIndexBuffer(const TIndexBufferDesc& InDesc)
		: FRenderResource(ERenderResourceType::IndexBuffer)
		, Desc(InDesc)
	{
	}

	FIndexBuffer::~FIndexBuffer()
	{
	}

	void FIndexBuffer::CreateGPUBuffer(FRHICmdList* CmdList, TStreamPtr Data, EGPUResourceState TargetState)
	{
		TI_ASSERT(TThread::AccquireId() == CmdList->WorkingThread);
		TI_ASSERT(GPUResourceIB == nullptr);

		FGPUBufferDesc IBDesc;
		IBDesc.Flag = 0;
		IBDesc.BufferSize = Desc.IndexCount * (Desc.IndexType == EIT_16BIT ? sizeof(uint16) : sizeof(uint32));

		// Create GPU resource and copy data
		FRHI* RHI = FRHI::Get();
		GPUResourceIB = RHI->CreateGPUBuffer();
		GPUResourceIB->Init(CmdList, IBDesc, Data);
		RHI->SetGPUBufferName(GPUResourceIB, GetResourceName());

		// Set resource state to IndexBuffer
		if (TargetState != EGPUResourceState::Ignore)
			CmdList->SetGPUBufferState(GPUResourceIB, TargetState);
	}

	/////////////////////////////////////////////////////////////
	FInstanceBuffer::FInstanceBuffer()
		: FRenderResource(ERenderResourceType::InstanceBuffer)
	{
	}

	FInstanceBuffer::FInstanceBuffer(const TInstanceBufferDesc& InDesc)
		: FRenderResource(ERenderResourceType::InstanceBuffer)
		, Desc(InDesc)
	{
	}

	FInstanceBuffer::~FInstanceBuffer()
	{
	}

	void FInstanceBuffer::CreateGPUBuffer(FRHICmdList* CmdList, TStreamPtr Data, EGPUResourceState TargetState)
	{
		TI_ASSERT(TThread::AccquireId() == CmdList->WorkingThread);
		TI_ASSERT(GPUResourceInsB == nullptr);

		FGPUBufferDesc InsBDesc;
		InsBDesc.Flag = 0;
		InsBDesc.BufferSize = Desc.InstanceCount * Desc.Stride;

		// Create GPU resource and copy data
		FRHI* RHI = FRHI::Get();
		GPUResourceInsB = RHI->CreateGPUBuffer();
		GPUResourceInsB->Init(CmdList, InsBDesc, Data);
		RHI->SetGPUBufferName(GPUResourceInsB, GetResourceName());

		// Set resource state to VertexAndConstantBuffer
		if (TargetState != EGPUResourceState::Ignore)
			CmdList->SetGPUBufferState(GPUResourceInsB, TargetState);
	}
}