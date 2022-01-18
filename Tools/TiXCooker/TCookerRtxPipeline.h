/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once
#include "TCooker.h"

namespace tix
{
	class TCookerRtxPipeline : public TCooker
	{
	public:
		TCookerRtxPipeline();
		virtual ~TCookerRtxPipeline();

		virtual EChunkLib GetCookerType() const override
		{
			return EChunkLib::RtxPipeline;
		};
		virtual bool Load(const TJSON& JsonDoc) override;
		virtual void SaveTrunk(TChunkFile& OutChunkFile) override;

	private:

	private:
		//E_BLEND_MODE BlendMode;
		TString ShaderLibName;
		TStream ShaderBlob;

		TRtxPipelineDesc RtxDesc;
	};
}