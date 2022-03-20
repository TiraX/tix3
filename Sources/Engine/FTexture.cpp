/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"

namespace tix
{
	FTexturePtr FTexture::CreateTexture(const TTextureDesc& Desc, uint32 InFlag)
	{
		return ti_new FTexture(Desc, InFlag);
	}
	FTexturePtr FTexture::CreateReadableTexture(const TTextureDesc& Desc, uint32 InFlag)
	{
		return ti_new FTextureReadable(Desc);
	}

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

	void FTexture::CreateGPUTexture(FRHICmdList* CmdList, const TVector<TImagePtr>& Data, EGPUResourceState TargetState)
	{
		TI_ASSERT(IsRenderThread());
		TI_ASSERT(GPUTexture == nullptr);
		FRHI* RHI = FRHI::Get();

		// Readback flag auto used in FTextureReadable, NEVER appear in FTexture
		TI_ASSERT((TextureFlag & (uint32)EGPUResourceFlag::Readback) == 0);

		FGPUTextureDesc Desc;
		Desc.Flag = TextureFlag;
		Desc.Texture = TextureDesc;

		// Create GPU resource and copy data
		GPUTexture = RHI->CreateGPUTexture();
		GPUTexture->Init(CmdList, Desc, Data);
		RHI->SetGPUTextureName(GPUTexture, GetResourceName());
	}
	/////////////////////////////////////////////////////////////
	FTextureReadable::FTextureReadable(const TTextureDesc& Desc)
		: FTexture(Desc, 0)
	{
	}

	FTextureReadable::~FTextureReadable()
	{
		TI_ASSERT(IsRenderThread());
	}

	void FTextureReadable::CreateGPUTexture(FRHICmdList* CmdList, const TVector<TImagePtr>& Data, EGPUResourceState TargetState)
	{
		FTexture::CreateGPUTexture(CmdList, Data);
		// Create GPUReadableTexture on Readback heap
		TI_ASSERT(GPUReadbackBuffer == nullptr);
		FRHI* RHI = FRHI::Get();

		FGPUBufferDesc Desc;
		Desc.Flag = (uint32)EGPUResourceFlag::Readback;
		Desc.BufferSize = TImage::GetDataSize(TextureDesc.Format, TextureDesc.Width, TextureDesc.Height);
		TI_ASSERT(TextureDesc.Depth == 1 && TextureDesc.Type == ETT_TEXTURE_2D);
		// Create GPU resource and copy data
		GPUReadbackBuffer = RHI->CreateGPUBuffer();
		GPUReadbackBuffer->Init(CmdList, Desc, nullptr);
		RHI->SetGPUBufferName(GPUReadbackBuffer, GetResourceName() + "-Readback");
	}

	void FTextureReadable::PrepareDataForCPU(FRHICmdList* CmdList)
	{
		// Copy data from GPUTexture to GPUReadbackBuffer
		FRHI* RHI = FRHI::Get();
		uint32 RowPitch = TImage::GetRowPitch(TextureDesc.Format, TextureDesc.Width);
		CmdList->SetGPUTextureState(GPUTexture, EGPUResourceState::CopySource);
		CmdList->CopyTextureRegion(GPUReadbackBuffer, GPUTexture, RowPitch);
	}

	TImagePtr FTextureReadable::ReadTextureData()
	{
		if (GPUReadbackBuffer != nullptr)
		{
			FRHI* RHI = FRHI::Get();
			TStreamPtr Buffer = RHI->ReadGPUBufferToCPU(GPUReadbackBuffer);
			TI_ASSERT(Buffer->GetLength() == TImage::GetDataSize(TextureDesc.Format, TextureDesc.Width, TextureDesc.Height));

			TImagePtr Image = ti_new TImage(TextureDesc.Format, TextureDesc.Width, TextureDesc.Height);
			uint8* ImageData = Image->Lock();
			memcpy(ImageData, Buffer->GetBuffer(), Buffer->GetLength());
			Image->Unlock();

			return Image;
		}
		return nullptr;
	}
}