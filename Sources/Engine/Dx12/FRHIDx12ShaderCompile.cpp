/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "FRHIDx12.h"

#if COMPILE_WITH_RHI_DX12
#include "d3dx12.h"
#include "FRHIDx12Conversion.h"
#include <DirectXColors.h>
#include <d3d12shader.h>
#include <d3dcompiler.h>

namespace tix
{
	class TShaderInclude : public ID3DInclude
	{
	public:
		TString ShaderPath;

		TShaderInclude(const TString& InShaderPath)
			: ShaderPath(InShaderPath)
		{}


		HRESULT __stdcall Open(
			D3D_INCLUDE_TYPE IncludeType,
			LPCSTR pFileName,
			LPCVOID pParentData,
			LPCVOID* ppData,
			UINT* pBytes
		)
		{
			TString FullPath = ShaderPath + pFileName;

			TFile f;
			if (f.Open(FullPath, EFA_READ))
			{
				uint8* Data = ti_new uint8[f.GetSize()];
				f.Read(Data, f.GetSize(), f.GetSize());
				f.Close();

				*ppData = Data;
				*pBytes = f.GetSize();

				return S_OK;
			}
			else
			{
				return E_FAIL;
			}
		}

		HRESULT __stdcall Close(LPCVOID pData)
		{
			uint8* Ptr = (uint8*)pData;
			ti_delete[] Ptr;
			return S_OK;
		}
	};

	TStreamPtr FRHIDx12::CompileShader(const TString& InCode, const TString& Entry, const TString& Target, const TString& IncludePath, bool bDebug)
	{
		TShaderInclude ShaderInclude(IncludePath);
		uint32 CompileFlags = 0;
		if (bDebug)
		{
			CompileFlags |= D3DCOMPILE_DEBUG;
			CompileFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
		}
		ID3DBlob* ByteCode = nullptr;
		ID3DBlob* CompileErrors = nullptr;

		D3DCompile(
			InCode.c_str(),
			InCode.length(),
			nullptr,
			nullptr,
			&ShaderInclude,
			Entry.c_str(),
			Target.c_str(),
			CompileFlags,
			0,
			&ByteCode,
			&CompileErrors
		);

		if (CompileErrors != nullptr)
		{
			_LOG(ELog::Error, "%s", (char*)CompileErrors->GetBufferPointer());

			TVector<TString> Lines;
			Lines.reserve(100);
			TStringSplit(InCode, '\n', Lines);
			for (int32 l = 0; l < (int32)Lines.size(); l++)
			{
				const TString& L = Lines[l];
				_LOG(ELog::Log, "%03d %s\n", l, L.c_str());
			}
			CompileErrors->Release();
			return nullptr;
		}
		else
		{
			TStreamPtr Blob = ti_new TStream((uint32)ByteCode->GetBufferSize());
			Blob->Put(ByteCode->GetBufferPointer(), (uint32)ByteCode->GetBufferSize());
			ByteCode->Release();
			return Blob;
		}
	}
}
#endif	// COMPILE_WITH_RHI_DX12