/*
	TiX Engine v2.0 Copyright (C) 2018~2021
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once
#include "TCooker.h"

namespace tix
{
	class TCookerMaterial : public TCooker
	{
	public:
		TCookerMaterial();
		virtual ~TCookerMaterial();

		virtual EChunkLib GetCookerType() const override
		{
			return EChunkLib::Material;
		};
		virtual bool Load(const TJSON& JsonDoc) override;
		virtual void SaveTrunk(TChunkFile& OutChunkFile) override;

		//void SetShaderName(E_SHADER_STAGE Stage, const TString& Name);
		void SetBlendMode(E_BLEND_MODE InBlendMode);
		void SetShaderVsFormat(uint32 InVsFormat);
		void SetShaderInsFormat(uint32 InInsFormat);
		void EnableDepthWrite(bool bEnable);
		void EnableDepthTest(bool bEnable);
		void EnableTwoSides(bool bEnable);

	private:

	private:
		//E_BLEND_MODE BlendMode;
		TVector<TString> ShaderNames;
		TStream ShaderCodes[ESS_COUNT];

		E_BLEND_MODE BlendMode;
		TPipelineDesc PipelineDesc;
	};
}