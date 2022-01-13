/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"

namespace tix
{
	FTexture::FTexture(const TTextureDesc& Desc, uint32 InFlag)
		: FRenderResource(ERenderResourceType::Texture)
		, TextureDesc(Desc)
		, TextureFlag(InFlag)
	{
	}

	FTexture::~FTexture()
	{
		TI_ASSERT(IsRenderThread());
	}

	void FTexture::CreateGPUTexture(const TVector<TImagePtr>& Data)
	{
		TI_ASSERT(IsRenderThread());
		TI_ASSERT(GPUTexture == nullptr);
		FRHI* RHI = FRHI::Get();

		FGPUTextureDesc Desc;
		if ((TextureFlag & (uint32)ETextureFlag::ColorBuffer) != 0)
			Desc.Flag |= (uint32)EGPUResourceFlag::ColorBuffer;
		if ((TextureFlag & (uint32)ETextureFlag::DsBuffer) != 0)
			Desc.Flag |= (uint32)EGPUResourceFlag::DsBuffer;
		if ((TextureFlag & (uint32)ETextureFlag::Uav) != 0)
			Desc.Flag |= (uint32)EGPUResourceFlag::Uav;
		Desc.Texture = TextureDesc;

		// Create GPU resource and copy data
		GPUTexture = RHI->CreateGPUTexture();
		GPUTexture->Init(Desc, Data);
		RHI->SetGPUTextureName(GPUTexture, GetResourceName());

		// Set resource state to IndexBuffer
		//RHI->SetGPUBufferState(GPUResourceIB, EGPUResourceState::IndexBuffer);
	}
	///////////////////////////////////////////////////////////
	FTextureReadable::FTextureReadable(const TTextureDesc& Desc)
		: FTexture(Desc)
	{
	}

	FTextureReadable::~FTextureReadable()
	{
		TI_ASSERT(IsRenderThread());
	}

	void FTextureReadable::CreateGPUTexture(const TVector<TImagePtr>& Data)
	{
		FTexture::CreateGPUTexture(Data);
		// Create GPUReadableTexture on Readback heap
		TI_ASSERT(GPUReadbackBuffer == nullptr);
		FRHI* RHI = FRHI::Get();

		FGPUBufferDesc Desc;
		Desc.Flag = (uint32)EGPUResourceFlag::Readback;
		Desc.BufferSize = TImage::GetDataSize(TextureDesc.Format, TextureDesc.Width, TextureDesc.Height);
		TI_ASSERT(TextureDesc.Depth == 1 && TextureDesc.Type == ETT_TEXTURE_2D);
		// Create GPU resource and copy data
		GPUReadbackBuffer = RHI->CreateGPUBuffer();
		GPUReadbackBuffer->Init(Desc, nullptr);
		RHI->SetGPUBufferName(GPUReadbackBuffer, GetResourceName() + "-Readback");
	}

	void FTextureReadable::PrepareDataForCPU()
	{
		// Copy data from GPUTexture to GPUReadbackBuffer
		FRHI* RHI = FRHI::Get();
		uint32 RowPitch = TImage::GetRowPitch(TextureDesc.Format, TextureDesc.Width);
		RHI->CopyTextureRegion(GPUReadbackBuffer, GPUTexture, RowPitch);
	}

	TImagePtr FTextureReadable::ReadTextureData()
	{
		if (GPUReadbackBuffer != nullptr)
		{
			FRHI* RHI = FRHI::Get();
			TImagePtr Image = ti_new TImage(TextureDesc.Format, TextureDesc.Width, TextureDesc.Height);
			RHI->ReadGPUBufferToImage(GPUReadbackBuffer, Image);
			return Image;
		}
		return nullptr;
	}
}