/*
	TiX Engine v3.0 Copyright (C) 2022~2025
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
			FFloat3 InitPos;
			FQuat InitRot;
			FFloat3 InitScale;
		};

		int32 TotalBones;
		TVector<ResBoneInfo> InitBones;
	};
}