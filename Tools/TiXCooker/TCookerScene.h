/*
	TiX Engine v2.0 Copyright (C) 2018~2021
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	struct TSceneMeshInstance
	{
		vector3df Position;
		quaternion Rotation;
		vector3df Scale;

		TSceneMeshInstance()
			: Scale(1.f, 1.f, 1.f)
		{}
	};
	struct TSceneMeshInstances
	{
		TString MeshName;
		TVector<TSceneMeshInstance> Instances;
	};

	struct TSceneEnvSunLight
	{
		vector3df Direction;
		SColorf Color;
		float Intensity;

		TSceneEnvSunLight()
			: Direction(0, 0, -1.f)
			, Color(1.f, 1.f, 1.f, 1.f)
			, Intensity(3.14f)
		{}
	};

	struct TSkyIrradianceSH3
	{
		float SH3_Raw[FSHVectorRGB3::NumTotalFloats];
		TSkyIrradianceSH3()
		{
			memset(SH3_Raw, 0, sizeof(SH3_Raw));
		}
	};

	struct TSceneEnvironment
	{
		TSceneEnvSunLight SunLight;
		TSkyIrradianceSH3 SkyLight;
	};

	class TResSceneHelper
	{
	public:
		TResSceneHelper();
		~TResSceneHelper();

		static void LoadScene(TJSON& Doc, TStream& OutStream, TVector<TString>& OutStrings);
		void OutputScene(TStream& OutStream, TVector<TString>& OutStrings);

	private:

	private:
		TString MapName;
		TSceneEnvironment Environment;
		TVector<THeaderCameraInfo> Cameras;

		int32 VTSize;
		int32 PageSize;

		TVector<vector2di> AssetSceneTiles;
		THMap<TString, TVTRegionInfo> VTRegionInfo;
	};
}