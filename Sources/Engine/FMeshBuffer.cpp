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

	void FVertexBuffer::CreateGPUResource(TStreamPtr Data)
	{
		TI_ASSERT(IsRenderThread());
		TI_ASSERT(GPUResourceVB == nullptr);
		FRHI* RHI = FRHI::Get();

		FGPUResourceDesc VBDesc;
		VBDesc.Flag = 0;
		VBDesc.BufferSize = Desc.VertexCount * Desc.Stride;

		// Create GPU resource and copy data
		GPUResourceVB = RHI->CreateGPUResourceBuffer();
		GPUResourceVB->Init(VBDesc, Data);
		RHI->SetGPUResourceBufferName(GPUResourceVB, GetResourceName());

		// Set resource state to VertexAndConstantBuffer
		RHI->SetGPUResourceBufferState(GPUResourceVB, EGPUResourceState::VertexAndConstantBuffer);
	}

	///////////////////////////////////////////////////////////
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

	void FIndexBuffer::CreateGPUResource(TStreamPtr Data)
	{
		TI_ASSERT(IsRenderThread());
		TI_ASSERT(GPUResourceIB == nullptr);
		FRHI* RHI = FRHI::Get();

		FGPUResourceDesc IBDesc;
		IBDesc.Flag = 0;
		IBDesc.BufferSize = Desc.IndexCount * (Desc.IndexType == EIT_16BIT ? sizeof(uint16) : sizeof(uint32));

		// Create GPU resource and copy data
		GPUResourceIB = RHI->CreateGPUResourceBuffer();
		GPUResourceIB->Init(IBDesc, Data);
		RHI->SetGPUResourceBufferName(GPUResourceIB, GetResourceName());

		// Set resource state to IndexBuffer
		RHI->SetGPUResourceBufferState(GPUResourceIB, EGPUResourceState::IndexBuffer);
	}

	///////////////////////////////////////////////////////////
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

	void FInstanceBuffer::CreateGPUResource(TStreamPtr Data)
	{
		TI_ASSERT(IsRenderThread());
		TI_ASSERT(GPUResourceInsB == nullptr);
		FRHI* RHI = FRHI::Get();

		FGPUResourceDesc InsBDesc;
		InsBDesc.Flag = 0;
		InsBDesc.BufferSize = Desc.InstanceCount * Desc.Stride;

		// Create GPU resource and copy data
		GPUResourceInsB = RHI->CreateGPUResourceBuffer();
		GPUResourceInsB->Init(InsBDesc, Data);
		RHI->SetGPUResourceBufferName(GPUResourceInsB, GetResourceName());

		// Set resource state to VertexAndConstantBuffer
		RHI->SetGPUResourceBufferState(GPUResourceInsB, EGPUResourceState::VertexAndConstantBuffer);
	}
}