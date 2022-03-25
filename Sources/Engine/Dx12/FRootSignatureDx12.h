/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

#if COMPILE_WITH_RHI_DX12

#include "d3dx12.h"

using namespace Microsoft::WRL;

namespace tix
{
	class FRootSignatureDx12 : public FShaderBinding
	{
	public:
		FRootSignatureDx12(uint32 NumRootParams = 0, uint32 NumStaticSamplers = 0) 
			: FShaderBinding(NumRootParams)
			, Finalized(false)
		{
		}

		virtual ~FRootSignatureDx12()
		{
			Signature = nullptr;
		}

		void Finalize(ID3D12Device* D3dDevice, const D3D12_ROOT_SIGNATURE_DESC& RSDesc);


		ID3D12RootSignature* Get() const
		{
			return Signature.Get(); 
		}
	private:

	private:
		bool Finalized;

		ComPtr<ID3D12RootSignature> Signature;

		friend class FRHIDx12;
	};
}
#endif	// COMPILE_WITH_RHI_DX12