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
		vector3df Position;
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

		TString LevelName;
		vector2di Position;
		aabbox3df BBox;
		
		uint32 StaticMeshesTotal;
		uint32 SMSectionsTotal;
		uint32 SMInstancesTotal;

		uint32 ReflectionCapturesTotal;

		uint32 SkeletalMeshTotal;
		uint32 SkeletonTotal;
		uint32 AnimationTotal;
		uint32 SKMActorsTotal;

		
		TVector<TString> AssetTextures;
		TVector<TString> AssetMaterialInstances;
		TVector<TString> AssetMaterials;
		TVector<TString> AssetAnims;
		TVector<TString> AssetSkeletons;
		TVector<TString> AssetSMs;
		TVector<TString> AssetSKMs;
		TVector<int32> SMInstanceCount;
		TVector<int32> SMSections;
		TVector<TResEnvLight> EnvLights;
		TVector<TResSMInstance> SMInstances;
		TVector<TResSKMActor> SKMActors;
	};
}