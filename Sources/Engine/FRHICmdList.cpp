/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "FRHICmdList.h"

namespace tix
{
	FRHICmdList::FRHICmdList(ERHICmdList InType)
		: Type(InType)
	{}

	FRHICmdList::~FRHICmdList()
	{
		CurrentRenderTarget = nullptr;
		CurrentBoundResource.Reset();
	}

	void FRHICmdList::SetViewport(const FRecti& InViewport)
	{
		Viewport = InViewport;
	}

	void FRHICmdList::SetScissorRect(const FRecti& InRect)
	{
		ScissorRect = InRect;
	}

	void FRHICmdList::BeginRenderToRenderTarget(FRenderTargetPtr RT, const int8* PassName, uint32 MipLevel)
	{
		CurrentRenderTarget = RT;

		const FInt2& Dim = RT->GetDemension();
		FRecti VP(0, 0, Dim.X >> MipLevel, Dim.Y >> MipLevel);
		SetViewport(VP);
		SetScissorRect(VP);
	}
}
