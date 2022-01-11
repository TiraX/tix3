/*
	TiX Engine v2.0 Copyright (C) 2018
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once
#include "TCooker.h"

namespace tix
{
	struct TResEnvLight
	{
		TString Name;
		TString LinkedCubemap;
		int32 Size;
		float AvgBrightness;
		float Brightness;
		FFloat3 Position;
	};
	class TCookerSceneTile : public TCooker
	{
	public:
		TCookerSceneTile();
		virtual ~TCookerSceneTile();

		virtual EChunkLib GetCookerType() const override
		{
			return EChunkLib::SceneTile;
		};
		virtual bool Load(const TJSON& JsonDoc) override;
		virtual void SaveTrunk(TChunkFile& OutChunkFile) override;

	private:
		TString LevelName;
		FInt2 Position;
		FBox BBox;
		
		int32 StaticMeshesTotal;
		int32 SMInstancesTotal;

		int32 ReflectionCapturesTotal;
		
		TVector<TString> AssetTextures;
		TVector<TString> AssetMaterialInstances;
		TVector<TString> AssetMaterials;
		TVector<TString> AssetSMs;
		TVector<int32> SMInstanceCount;
		TVector<TResEnvLight> EnvLights;
		TVector<TResSMInstance> SMInstances;
	};
}