/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "FRHI.h"
#include "FRenderTarget.h"
#include "Dx12/FRHIDx12.h"
#include "Metal/FRHIMetal.h"

namespace tix
{
	FRHI* FRHI::RHI = nullptr;
	uint32 FRHI::NumGPUFrames = 0;
	FRHIConfig FRHI::RHIConfig;

	FRHI* FRHI::Get()
	{
		return RHI;
	}

	void FRHI::CreateRHI()
	{
		TI_ASSERT(RHI == nullptr);
#if defined (TI_PLATFORM_WIN32) && (COMPILE_WITH_RHI_DX12)
		RHI = ti_new FRHIDx12;
#elif defined (TI_PLATFORM_IOS) && (COMPILE_WITH_RHI_METAL)
        RHI = ti_new FRHIMetal;
#else
#error("No avaible RHI for this platform.")
#endif
	}

	void FRHI::ReleaseRHI()
	{
		TI_ASSERT(RHI != nullptr);
		ti_delete RHI;
		RHI = nullptr;
	}

	FRHI::FRHI(E_RHI_TYPE InRHIType)
		: RHIType(InRHIType)
	{
		for (int32 i = 0; i < FRHIConfig::FrameBufferNum; ++i)
		{
			FrameResources[i] = nullptr;
		}
	}

	FRHI::~FRHI()
	{
		CurrentRenderTarget = nullptr;
		CurrentBoundResource.Reset();
	}

	void FRHI::SupportFeature(E_RHI_FEATURE InFeature)
	{
		RHIConfig.SupportedFeatures |= InFeature;
	}

	void FRHI::SetViewport(const FViewport& InViewport)
	{
		Viewport = InViewport;
	}
	
    void FRHI::BeginFrame()
    {
        CurrentRenderTarget = nullptr;
        CurrentBoundResource.Reset();

		FStats::ResetPerFrame();
    }
    
	FRenderResourceTablePtr FRHI::CreateRenderResourceTable(uint32 InSize, EResourceHeapType InHeap)
	{
		FRenderResourceTablePtr Table = ti_new FRenderResourceTable(InSize);
		GetRenderResourceHeap(InHeap).InitResourceTable(Table);
		return Table;
	}

	void FRHI::BeginRenderToRenderTarget(FRenderTargetPtr RT, const int8* PassName, uint32 MipLevel)
	{
		CurrentRenderTarget = RT;
        
        const FInt2& d = RT->GetDemension();
		RtViewport.Left = 0;
		RtViewport.Top = 0;
		RtViewport.Width = d.X >> MipLevel;
		RtViewport.Height = d.Y >> MipLevel;

		SetViewport(RtViewport);
	}
}
