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

	FRHI* FRHI::CreateRHI()
	{
		TI_ASSERT(RHI == nullptr);
#if defined (TI_PLATFORM_WIN32) && (COMPILE_WITH_RHI_DX12)
		RHI = ti_new FRHIDx12();
#elif defined (TI_PLATFORM_IOS) && (COMPILE_WITH_RHI_METAL)
        RHI = ti_new FRHIMetal;
#else
#error("No avaible RHI for this platform.")
#endif
		return RHI;
	}

	void FRHI::ReleaseRHI()
	{
		TI_ASSERT(RHI != nullptr);
		ti_delete RHI;
		RHI = nullptr;
	}

	FRHI::FRHI(ERHIType InRHIType)
		: RHIType(InRHIType)
	{
	}

	FRHI::~FRHI()
	{
	}

	void FRHI::SupportFeature(E_RHI_FEATURE InFeature)
	{
		RHIConfig.SupportedFeatures |= InFeature;
	}
	
    void FRHI::BeginFrame()
    {
		FStats::ResetPerFrame();
    }
}
