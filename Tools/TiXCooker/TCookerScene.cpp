/*
	TiX Engine v2.0 Copyright (C) 2018~2021
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "ResHelper.h"
#include "TCookerScene.h"

using namespace rapidjson;

namespace tix
{
	TCookerScene::TCookerScene()
		: VTSize(0)
		, PageSize(0)
	{
	}

	TCookerScene::~TCookerScene()
	{
	}

	bool TCookerScene::Load(const TJSON& Doc)
	{
		// Map Name
		Doc["name"] << MapName;

		// Load environments. include sun light, fog, etc
		{
			TJSONNode JEnv = Doc["environment"];

			// Sun light
			TJSONNode JSunLight = JEnv["sun_light"];
			JSunLight["direction"] << Environment.SunLight.Direction;
			JSunLight["color"] << Environment.SunLight.Color;
			JSunLight["intensity"] << Environment.SunLight.Intensity;

			// Sky light
			TJSONNode JSkyLight = JEnv["sky_light"];
			TVector<float> FloatArray;
			JSkyLight["irradiance_sh3"] << FloatArray;
			TI_ASSERT(FloatArray.size() == FSHVectorRGB3::NumTotalFloats);
			for (int32 i = 0; i < FSHVectorRGB3::NumTotalFloats; i++)
			{
				Environment.SkyLight.SH3_Raw[i] = FloatArray[i];
			}
		}

		// Load cameras. 
		{
			TJSONNode JCameras = Doc["cameras"];
			Cameras.clear();
			Cameras.reserve(JCameras.Size());
			for (int32 c = 0; c < JCameras.Size(); ++c)
			{
				TJSONNode JCam = JCameras[c];
				THeaderCameraInfo Cam;
				JCam["location"] << Cam.Location;
				JCam["target"] << Cam.Target;
				JCam["rotator"] << Cam.Rotate;
				JCam["fov"] << Cam.FOV;
				JCam["aspect"] << Cam.Aspect;
				Cameras.push_back(Cam);
			}
		}

		// Load tiles
		{
			TJSONNode JTileList = Doc["tiles"];

			AssetSceneTiles.reserve(JTileList.Size());
			for (int32 i = 0; i < JTileList.Size(); ++i)
			{
				TJSONNode JTile = JTileList[i];
				vector2di TilePos(-99999, -99999);
				JTile["index"] << TilePos;
				AssetSceneTiles.push_back(TilePos);
				TI_ASSERT(TMath::Abs(TilePos.X) <= 32760 && TMath::Abs(TilePos.Y) <= 32760);
			}
		}

		return true;
	}
	
	void TCookerScene::SaveTrunk(TChunkFile& OutChunkFile)
	{
		TStream& OutStream = OutChunkFile.GetChunk(GetCookerType());
		TVector<TString>& OutStrings = OutChunkFile.Strings;

		TResfileChunkHeader ChunkHeader;
		ChunkHeader.ID = TIRES_ID_CHUNK_SCENE;
		ChunkHeader.Version = TIRES_VERSION_CHUNK_SCENE;
		ChunkHeader.ElementCount = 1;

		TStream HeaderStream, DataStream(1024 * 8);
		for (int32 t = 0; t < ChunkHeader.ElementCount; ++t)
		{
			THeaderScene Define;
			Define.NameIndex = AddStringToList(OutStrings, MapName);

			// Environment Info
			Define.MainLightDirection = Environment.SunLight.Direction;
			Define.MainLightColor = Environment.SunLight.Color;
			Define.MainLightIntensity = Environment.SunLight.Intensity;
			memcpy(Define.SkyLight_SH3, Environment.SkyLight.SH3_Raw, sizeof(float) * FSHVectorRGB3::NumTotalFloats);

			// Cameras
			Define.NumCameras = (int32)Cameras.size();
			for (const auto& C : Cameras)
			{
				DataStream.Put(&C, sizeof(THeaderCameraInfo));
			}

			// Tile Info
			Define.NumTiles = (int32)AssetSceneTiles.size();

			// Fill Tile Positions
			for (const auto& A : AssetSceneTiles)
			{
				vector2di16 Pos;
				Pos.X = (int16)(A.X);
				Pos.Y = (int16)(A.Y);
				DataStream.Put(&Pos, sizeof(vector2di16));
			}
			
			// Save header
			HeaderStream.Put(&Define, sizeof(THeaderScene));
		}

		ChunkHeader.ChunkSize = HeaderStream.GetLength() + DataStream.GetLength();

		OutStream.Put(&ChunkHeader, sizeof(TResfileChunkHeader));
		FillZero4(OutStream);
		OutStream.Put(HeaderStream.GetBuffer(), HeaderStream.GetLength());
		OutStream.Put(DataStream.GetBuffer(), DataStream.GetLength());
	}
}
