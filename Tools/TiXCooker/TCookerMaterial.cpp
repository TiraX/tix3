/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "ResHelper.h"
#include "TCookerMaterial.h"

using namespace rapidjson;

namespace tix
{
	TCookerMaterial::TCookerMaterial()
	{
	}

	TCookerMaterial::~TCookerMaterial()
	{
	}

	bool TCookerMaterial::Load(const TJSON& Doc)
	{
		// shaders
		Doc["shaders"] << ShaderNames;
		TI_ASSERT(ShaderNames.size() == ESS_COUNT);

		// topology
		{
			TString Topo;
			Doc["topology"] << Topo;
			PipelineDesc.PrimitiveType = EPrimitiveType::TriangleList;// GetPrimitiveType(Topo);
		}

		// vertex format
		{
			TJSONNode JVSFormat = Doc["vs_format"];
			TI_ASSERT(JVSFormat.IsArray());
			uint32 VsFormat = 0;
			for (int32 vs = 0; vs < JVSFormat.Size(); ++vs)
			{
				TString Format;
				JVSFormat[vs] << Format;
				VsFormat |= GetVertexSegment(Format);
			}
			SetShaderVsFormat(VsFormat);
		}

		// instance format
		{
			TJSONNode JINSFormat = Doc["ins_format"];
			if (!JINSFormat.IsNull())
			{
				TI_ASSERT(JINSFormat.IsArray());
				uint32 InsFormat = 0;
				for (int32 vs = 0; vs < JINSFormat.Size(); ++vs)
				{
					TString Format;
					JINSFormat[vs] << Format;
					InsFormat |= GetInstanceSegment(Format);
				}
				SetShaderInsFormat(InsFormat);
			}
		}

		// blend mode
		TString BlendMode;
		Doc["blend_mode"] << BlendMode;
		SetBlendMode(GetBlendMode(BlendMode));

		// depth write / depth test / two sides
		bool DepthWrite = true;
		Doc["depth_write"] << DepthWrite;
		EnableDepthWrite(DepthWrite);

		bool DepthTest = true;
		Doc["depth_test"] << DepthTest;
		EnableDepthTest(DepthTest);

		bool TwoSides = false;
		Doc["two_sides"] << TwoSides;
		EnableTwoSides(TwoSides);

		// stencil state
		bool StencilEnable = false;
		Doc["stencil_enable"] << StencilEnable;
		if (StencilEnable)
			PipelineDesc.Enable(EPSO_STENCIL);
		else
			PipelineDesc.Disable(EPSO_STENCIL);

		// Not enable depth test, then set depth compare function to Always
		if (!PipelineDesc.IsEnabled(EPSO_DEPTH_TEST))
		{
			PipelineDesc.DepthStencilDesc.DepthFunc = ECF_ALWAYS;
		}

		int32 StencilReadMask = 0;
		Doc["stencil_read_mask"] << StencilReadMask;
		PipelineDesc.DepthStencilDesc.StencilReadMask = (uint8)StencilReadMask;

		int32 StencilWriteMask = 0;
		Doc["stencil_write_mask"] << StencilWriteMask;
		PipelineDesc.DepthStencilDesc.StencilWriteMask = (uint8)StencilWriteMask;

		TString FrontStencilOp;
		Doc["front_stencil_fail"] << FrontStencilOp;
		PipelineDesc.DepthStencilDesc.FrontFace.StencilFailOp = GetStencilOp(FrontStencilOp);

		TString FrontStencilDepthFail;
		Doc["front_stencil_depth_fail"] << FrontStencilDepthFail;
		PipelineDesc.DepthStencilDesc.FrontFace.StencilDepthFailOp = GetStencilOp(FrontStencilDepthFail);

		TString FrontStencilPass;
		Doc["front_stencil_pass"] << FrontStencilPass;
		PipelineDesc.DepthStencilDesc.FrontFace.StencilPassOp = GetStencilOp(FrontStencilPass);

		TString FrontStencilFunc;
		Doc["front_stencil_func"] << FrontStencilFunc;
		PipelineDesc.DepthStencilDesc.FrontFace.StencilFunc = GetComparisonFunc(FrontStencilFunc);

		TString BackStencilFail;
		Doc["back_stencil_fail"] << BackStencilFail;
		PipelineDesc.DepthStencilDesc.BackFace.StencilFailOp = GetStencilOp(BackStencilFail);

		TString BackStencilDepthFail;
		Doc["back_stencil_depth_fail"] << BackStencilDepthFail;
		PipelineDesc.DepthStencilDesc.BackFace.StencilDepthFailOp = GetStencilOp(BackStencilDepthFail);

		TString BackStencilPass;
		Doc["back_stencil_pass"] << BackStencilPass;
		PipelineDesc.DepthStencilDesc.BackFace.StencilPassOp = GetStencilOp(BackStencilPass);
		
		TString BackStencilFunc;
		Doc["back_stencil_func"] << BackStencilFunc;
		PipelineDesc.DepthStencilDesc.BackFace.StencilFunc = GetComparisonFunc(BackStencilFunc);

		// rt format
		TVector<TString> RTFormats;
		Doc["rt_colors"] << RTFormats;
		TI_ASSERT(RTFormats.size() <= 4);

		PipelineDesc.RTCount = (int32)RTFormats.size();
		for (int32 cb = 0; cb < RTFormats.size(); ++cb)
		{
			PipelineDesc.RTFormats[cb] = GetPixelFormat(RTFormats[cb]);
		}
		TString RTDepth;
		Doc["rt_depth"] << RTDepth;
		PipelineDesc.DepthFormat = GetPixelFormat(RTDepth);

		return true;
	}


	//void TCookerMaterial::SetShaderName(E_SHADER_STAGE Stage, const TString& Name)
	//{
	//	ShaderNames[Stage] = Name;
	//}

	void TCookerMaterial::SetBlendMode(E_BLEND_MODE InBlendMode)
	{
		BlendMode = InBlendMode;
		switch (InBlendMode)
		{
		case BLEND_MODE_OPAQUE:
		case BLEND_MODE_MASK:
			PipelineDesc.Disable(EPSO_BLEND);
			break;
		case BLEND_MODE_TRANSLUCENT:
			PipelineDesc.Enable(EPSO_BLEND);
			PipelineDesc.BlendState.SrcBlend = EBF_SRC_ALPHA;
			PipelineDesc.BlendState.DestBlend = EBF_ONE_MINUS_SRC_ALPHA;
			PipelineDesc.BlendState.BlendOp = EBE_FUNC_ADD;
			PipelineDesc.BlendState.SrcBlendAlpha = EBF_ONE;
			PipelineDesc.BlendState.DestBlendAlpha = EBF_ZERO;
			PipelineDesc.BlendState.BlendOpAlpha = EBE_FUNC_ADD;
			break;
		case BLEND_MODE_ADDITIVE:
			PipelineDesc.Enable(EPSO_BLEND);
			PipelineDesc.BlendState.SrcBlend = EBF_SRC_ALPHA;
			PipelineDesc.BlendState.DestBlend = EBF_ONE;
			PipelineDesc.BlendState.BlendOp = EBE_FUNC_ADD;
			PipelineDesc.BlendState.SrcBlendAlpha = EBF_ONE;
			PipelineDesc.BlendState.DestBlendAlpha = EBF_ZERO;
			PipelineDesc.BlendState.BlendOpAlpha = EBE_FUNC_ADD;
			break;
        default:
            TI_ASSERT(0);
            break;
		}
	}

	void TCookerMaterial::SetShaderVsFormat(uint32 InVsFormat)
	{
		PipelineDesc.VsFormat = InVsFormat;
	}

	void TCookerMaterial::SetShaderInsFormat(uint32 InInsFormat)
	{
		PipelineDesc.InsFormat = InInsFormat;
	}

	void TCookerMaterial::EnableDepthWrite(bool bEnable)
	{
		if (bEnable)
			PipelineDesc.Enable(EPSO_DEPTH);
		else
			PipelineDesc.Disable(EPSO_DEPTH);
	}

	void TCookerMaterial::EnableDepthTest(bool bEnable)
	{
		if (bEnable)
			PipelineDesc.Enable(EPSO_DEPTH_TEST);
		else
			PipelineDesc.Disable(EPSO_DEPTH_TEST);
	}

	void TCookerMaterial::EnableTwoSides(bool bEnable)
	{
		if (bEnable)
			PipelineDesc.RasterizerDesc.CullMode = static_cast<uint8>(ECullMode::None);
		else
			PipelineDesc.RasterizerDesc.CullMode = static_cast<uint8>(ECullMode::Back);
	}
	
	void TCookerMaterial::SaveTrunk(TChunkFile& OutChunkFile)
	{
		TStream& OutStream = OutChunkFile.GetChunk(GetCookerType());
		TVector<TString>& OutStrings = OutChunkFile.Strings;

		TResfileChunkHeader ChunkHeader;
		ChunkHeader.ID = TIRES_ID_CHUNK_MATERIAL;
		ChunkHeader.Version = TIRES_VERSION_CHUNK_MATERIAL;
		ChunkHeader.ElementCount = 1;

		TStream HeaderStream, DataStream(1024 * 8);
		for (int32 t = 0; t < ChunkHeader.ElementCount; ++t)
		{
			THeaderMaterial Define;
			for (int32 s = 0; s < ESS_COUNT; ++s)
			{
				Define.ShaderNames[s] = AddStringToList(OutStrings, ShaderNames[s]);
				Define.ShaderCodeLength[s] = ShaderCodes[s].GetLength();
			}

			Define.Flags = PipelineDesc.Flags;
			Define.BlendMode = BlendMode;
			Define.BlendState = PipelineDesc.BlendState;
			Define.RasterizerDesc = PipelineDesc.RasterizerDesc;
			Define.DepthStencilDesc = PipelineDesc.DepthStencilDesc;
			Define.VsFormat = PipelineDesc.VsFormat;
			Define.InsFormat = PipelineDesc.InsFormat;
			Define.PrmitiveType = static_cast<uint32>(PipelineDesc.PrimitiveType);

			int32 cb = 0;
			for (; cb < ERTC_COUNT; ++cb)
			{
				Define.ColorBuffers[cb] = PipelineDesc.RTFormats[cb];
			}
			Define.DepthBuffer = PipelineDesc.DepthFormat;

			// Save header
			HeaderStream.Put(&Define, sizeof(THeaderMaterial));

			// Write codes
			for (int32 s = 0; s < ESS_COUNT; ++s)
			{
				if (ShaderCodes[s].GetLength() > 0)
				{
					DataStream.Put(ShaderCodes[s].GetBuffer(), ShaderCodes[s].GetLength());
					DataStream.FillZero4();
				}
			}
		}

		ChunkHeader.ChunkSize = HeaderStream.GetLength() + DataStream.GetLength();

		OutStream.Put(&ChunkHeader, sizeof(TResfileChunkHeader));
		OutStream.FillZero4();
		OutStream.Put(HeaderStream.GetBuffer(), HeaderStream.GetLength());
		OutStream.Put(DataStream.GetBuffer(), DataStream.GetLength());
	}
}
