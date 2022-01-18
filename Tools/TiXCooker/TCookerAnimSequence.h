/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once
#include "TCooker.h"

namespace tix
{
	class TCookerAnimSequence : public TCooker
	{
	public:
		TCookerAnimSequence();
		virtual ~TCookerAnimSequence();

		virtual EChunkLib GetCookerType() const override
		{
			return EChunkLib::Animation;
		};
		virtual bool Load(const TJSON& JsonDoc) override;
		virtual void SaveTrunk(TChunkFile& OutChunkFile) override;

	private:

	private:
		int32 TotalFrames;
		float SequenceLength;
		float RateScale;
		int32 TotalTracks;
		TString RefSkeleton;

		struct FTrackInfo
		{
			int32 RefBoneIndex;
			TVector<float> PosKeys;
			TVector<float> RotKeys;
			TVector<float> ScaleKeys;
		};
		TVector<FTrackInfo> Tracks;
	};
}