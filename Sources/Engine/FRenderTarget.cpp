/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "FRenderTarget.h"

namespace tix
{
	FRenderTargetPtr FRenderTarget::Create(int32 W, int32 H)
	{
		FRHI* RHI = FRHI::Get();
		return RHI->CreateRenderTarget(W, H);
	}

	FRenderTarget::FRenderTarget(int32 W, int32 H)
		: FRenderResource(ERenderResourceType::RenderTarget)
		, Demension(W, H)
		, ColorBuffers(0)
	{
	}

	FRenderTarget::~FRenderTarget()
	{
	}

	void FRenderTarget::AddColorBuffer(E_PIXEL_FORMAT Format, uint32 Mips, E_RT_COLOR_BUFFER ColorBufferIndex, ERenderTargetLoadAction LoadAction, ERenderTargetStoreAction StoreAction)
	{
		TI_ASSERT(Mips > 0);
		TTextureDesc Desc;
		Desc.Format = Format;
		Desc.Width = Demension.X;
		Desc.Height = Demension.Y;
		Desc.AddressMode = ETC_CLAMP_TO_EDGE;
		Desc.Mips = Mips;

		FRHI * RHI = FRHI::Get();
		FTexturePtr Texture = FTexture::CreateTexture(Desc, (uint32)EGPUResourceFlag::ColorBuffer);
		AddColorBuffer(Texture, ColorBufferIndex, LoadAction, StoreAction);
	}

	void FRenderTarget::AddColorBuffer(FTexturePtr Texture, E_RT_COLOR_BUFFER ColorBufferIndex, ERenderTargetLoadAction LoadAction, ERenderTargetStoreAction StoreAction)
	{
		if (RTColorBuffers[ColorBufferIndex].BufferIndex == ERTC_INVALID)
		{
			++ColorBuffers;
		}

		Texture->SetTextureFlag(EGPUResourceFlag::ColorBuffer, true);
		RTBuffer Buffer;
		Buffer.Texture = Texture;
		Buffer.BufferIndex = ColorBufferIndex;
		Buffer.LoadAction = LoadAction;
		Buffer.StoreAction = StoreAction;

#if defined (TIX_DEBUG)
		{
			int8 NameBuf[64];
			sprintf(NameBuf, "-CB%d", ColorBufferIndex);
			Texture->SetResourceName(GetResourceName() + NameBuf);
		}
#endif

		RTColorBuffers[ColorBufferIndex] = Buffer;
	}

	void FRenderTarget::AddDepthStencilBuffer(E_PIXEL_FORMAT Format, uint32 Mips, ERenderTargetLoadAction LoadAction, ERenderTargetStoreAction StoreAction)
	{
		TI_ASSERT(Mips > 0);
		TTextureDesc Desc;
		Desc.Format = Format;
		Desc.Width = Demension.X;
		Desc.Height = Demension.Y;
		Desc.AddressMode = ETC_CLAMP_TO_EDGE;
		Desc.Mips = Mips;

		FRHI * RHI = FRHI::Get();
		FTexturePtr Texture = FTexture::CreateTexture(Desc, (uint32)EGPUResourceFlag::DsBuffer);
#if defined (TIX_DEBUG)
		Texture->SetResourceName(GetResourceName() + "-DS");
#endif

		AddDepthStencilBuffer(Texture, LoadAction, StoreAction);
	}

	void FRenderTarget::AddDepthStencilBuffer(FTexturePtr Texture, ERenderTargetLoadAction LoadAction, ERenderTargetStoreAction StoreAction)
	{
		Texture->SetTextureFlag(EGPUResourceFlag::DsBuffer, true);
		RTDepthStencilBuffer.Texture = Texture;
		RTDepthStencilBuffer.LoadAction = LoadAction;
		RTDepthStencilBuffer.StoreAction = StoreAction;
	}

	void FRenderTarget::Compile()
	{
		TI_ASSERT(IsRenderThread());
		FRHI * RHI = FRHI::Get();

		for (int32 i = 0; i < ERTC_COUNT; ++i)
		{
			const RTBuffer& ColorBuffer = RTColorBuffers[i];
			// Color buffer must be continuous
			if (ColorBuffer.BufferIndex == ERTC_INVALID)
				break;
			
			TI_ASSERT(ColorBuffer.Texture != nullptr);
			ColorBuffer.Texture->CreateGPUTexture(nullptr);
		}

		if (RTDepthStencilBuffer.Texture != nullptr)
		{
			RTDepthStencilBuffer.Texture->CreateGPUTexture(nullptr);
		}

		RHI->UpdateHardwareResourceRT(this);
	}
}
