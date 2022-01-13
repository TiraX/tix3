/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "FRHIDXR.h"

#if COMPILE_WITH_RHI_DX12
#include "FRHIDx12Conversion.h"

namespace tix
{
	FRHIDXR::FRHIDXR()
	{
	}

	bool FRHIDXR::Init(ID3D12Device* D3DDevice, ID3D12GraphicsCommandList* D3DCommandList)
	{
		return SUCCEEDED(D3DDevice->QueryInterface(IID_PPV_ARGS(&DXRDevice))) &&
			SUCCEEDED(D3DCommandList->QueryInterface(IID_PPV_ARGS(&DXRCommandList)));
	}
}
#endif	// COMPILE_WITH_RHI_DX12