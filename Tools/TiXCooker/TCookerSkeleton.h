/*
	TiX Engine v2.0 Copyright (C) 2018~2021
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once
#include "TCooker.h"

namespace tix
{
	class TCookerSkeleton : public TCooker
	{
	public:
		TCookerSkeleton();
		virtual ~TCookerSkeleton();

		virtual EChunkLib GetCookerType() const override
		{
			return EChunkLib::Skeleton;
		};
		virtual bool Load(const TJSON& JsonDoc) override;
		virtual void SaveTrunk(TChunkFile& OutChunkFile) override;

	private:
		void CalcInvBindTransform();

	private:
		struct ResBoneInfo
		{
			int32 ParentIndex;
			vector3df InitPos;
			quaternion InitRot;
			vector3df InitScale;
		};

		int32 TotalBones;
		TVector<ResBoneInfo> InitBones;
	};
}