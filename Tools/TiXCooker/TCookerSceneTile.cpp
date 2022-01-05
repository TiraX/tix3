/*
	TiX Engine v2.0 Copyright (C) 2018
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "ResHelper.h"
#include "TCookerSceneTile.h"

namespace tix
{
	TCookerSceneTile::TCookerSceneTile()
		: StaticMeshesTotal(0)
		, SMSectionsTotal(0)
		, SMInstancesTotal(0)
		, ReflectionCapturesTotal(0)
		, SkeletalMeshTotal(0)
		, SkeletonTotal(0)
		, AnimationTotal(0)
		, SKMActorsTotal(0)
		, EnvLights(0)
	{
	}

	TCookerSceneTile::~TCookerSceneTile()
	{
	}

	bool TCookerSceneTile::Load(const TJSON& Doc)
	{
		// Instances Name
		TString TileName;
		Doc["name"] << TileName;
		Doc["level"] << LevelName;

		Doc["static_mesh_total"] << StaticMeshesTotal;
		Doc["sm_sections_total"] << SMSectionsTotal;
		Doc["sm_instances_total"] << SMInstancesTotal;
		Doc["reflection_captures_total"] << ReflectionCapturesTotal;
		Doc["skeletal_meshes_total"] << SkeletalMeshTotal;
		Doc["skeletons_total"] << SkeletonTotal;
		Doc["anims_total"] << AnimationTotal;
		Doc["skm_actors_total"] << SKMActorsTotal;

		Doc["position"] << Position;
		Doc["bbox"] << BBox;

		// reflection captures
		{
			TJSONNode JReflectionCaptures = Doc["reflection_captures"];
			for (int32 i = 0; i < JReflectionCaptures.Size(); ++i)
			{
				TJSONNode JRC = JReflectionCaptures[i];
				TResEnvLight RC;
				JRC["name"] << RC.Name;
				JRC["linked_cubemap"] << RC.LinkedCubemap;
				JRC["cubemap_size"] << RC.Size;
				JRC["average_brightness"] << RC.AvgBrightness;
				JRC["brightness"] << RC.Brightness;
				JRC["position"] << RC.Position;

				EnvLights.push_back(RC);
			}
		}

		// dependency
		{        
			// Load asset list
			TJSONNode JAssetList = Doc["dependency"];
			JAssetList["textures"] << AssetTextures;
			JAssetList["materials"] << AssetMaterials;
			JAssetList["material_instances"] << AssetMaterialInstances;
			JAssetList["anims"] << AssetAnims;
			JAssetList["skeletons"] << AssetSkeletons;
			JAssetList["static_meshes"] << AssetSMs;
			JAssetList["skeletal_meshes"] << AssetSKMs;
		}

		// Static Mesh Instances
		{
			TJSONNode JSMInstanceObjects = Doc["static_mesh_instances"];
			TI_ASSERT(JSMInstanceObjects.IsArray());
			SMInstances.reserve(SMInstancesTotal);
			SMInstanceCount.resize(JSMInstanceObjects.Size());
			SMSections.resize(JSMInstanceObjects.Size());
			for (int32 obj = 0; obj < JSMInstanceObjects.Size(); ++obj)
			{
				TJSONNode JInstanceObj = JSMInstanceObjects[obj];
				TJSONNode JLinkedMesh = JInstanceObj["linked_mesh"];
				TJSONNode JMeshSections = JInstanceObj["mesh_sections"];
				TJSONNode JInstances = JInstanceObj["instances"];

				// Make sure InstanceObject has the same order with dependency-mesh
				TString LinkedMesh;
				JLinkedMesh << LinkedMesh;
				TI_ASSERT(LinkedMesh == AssetSMs[obj]);
				SMInstanceCount[obj] = JInstances.Size();
				JMeshSections << SMSections[obj];

				for (int32 ins = 0 ; ins < JInstances.Size(); ++ ins)
				{
					TJSONNode JIns = JInstances[ins];

					TResSMInstance Ins;
					JIns["position"] << Ins.Position;
					JIns["rotation"] << Ins.Rotation;
					JIns["scale"] << Ins.Scale;
					SMInstances.push_back(Ins);
				}
			}
		}


		// Skeleton Mesh Actors
		{
			TJSONNode JSKMActorObjects = Doc["skeletal_mesh_actors"];
			TI_ASSERT(JSKMActorObjects.IsArray());
			SKMActors.reserve(SkeletalMeshTotal);
			for (int32 obj = 0; obj < JSKMActorObjects.Size(); ++obj)
			{
				TJSONNode JActorObj = JSKMActorObjects[obj];
				TJSONNode JSKMActors = JActorObj["actors"];

				TString LinkedSkm, LinkedSK;
				JActorObj["linked_skm"] << LinkedSkm;
				JActorObj["linked_sk"] << LinkedSK;

				const int32 SKMIndex = IndexInArray<TString>(LinkedSkm, AssetSKMs);
				const int32 SKIndex = IndexInArray<TString>(LinkedSK, AssetSkeletons);

				for (int32 actor = 0; actor < JSKMActors.Size(); ++actor)
				{
					TJSONNode JActor = JSKMActors[actor];

					TString Anim;
					JActor["linked_anim"] << Anim;

					TResSKMActor Actor;
					Actor.SKMIndex = SKMIndex;
					Actor.SKIndex = SKIndex;
					Actor.AnimIndex = IndexInArray<TString>(Anim, AssetAnims);
					JActor["position"] << Actor.Position;
					JActor["rotation"] << Actor.Rotation;
					JActor["scale"] << Actor.Scale;
					SKMActors.push_back(Actor);
				}
			}
		}

		return true;
	}
	
	void TCookerSceneTile::SaveTrunk(TChunkFile& OutChunkFile)
	{
		TStream& OutStream = OutChunkFile.GetChunk(GetCookerType());
		TVector<TString>& OutStrings = OutChunkFile.Strings;

		TResfileChunkHeader ChunkHeader;
		ChunkHeader.ID = TIRES_ID_CHUNK_SCENETILE;
		ChunkHeader.Version = TIRES_VERSION_CHUNK_SCENETILE;
		ChunkHeader.ElementCount = 1;

		TStream HeaderStream, DataStream(1024 * 8);
		for (int32 t = 0; t < ChunkHeader.ElementCount; ++t)
		{
			// init header
			THeaderSceneTile SceneTileHeader;
			SceneTileHeader.LevelNameIndex = AddStringToList(OutStrings, LevelName);
			SceneTileHeader.Position.X = (int16)Position.X;
			SceneTileHeader.Position.Y = (int16)Position.Y;
			SceneTileHeader.BBox = BBox;

			SceneTileHeader.NumEnvLights = (int32)EnvLights.size();
			SceneTileHeader.NumTextures = (int32)AssetTextures.size();
			SceneTileHeader.NumMaterials = (int32)AssetMaterials.size();
			SceneTileHeader.NumMaterialInstances = (int32)AssetMaterialInstances.size();
			SceneTileHeader.NumSkeletons = (int32)AssetSkeletons.size();
			SceneTileHeader.NumAnims = (int32)AssetAnims.size();
			SceneTileHeader.NumStaticMeshes = (int32)AssetSMs.size();
			SceneTileHeader.NumSMSections = SMSectionsTotal;
			SceneTileHeader.NumSMInstances = (int32)SMInstances.size();
			SceneTileHeader.NumSkeletalMeshes = (int32)AssetSKMs.size();
			SceneTileHeader.NumSKMActors = (int32)SKMActors.size();

			// reflections
			TI_ASSERT(SceneTileHeader.NumEnvLights == EnvLights.size());
			for (const auto& EnvLight : EnvLights)
			{
				THeaderEnvLight EnvLightHeader;
				EnvLightHeader.NameIndex = AddStringToList(OutStrings, EnvLight.Name);
				EnvLightHeader.LinkedCubemapIndex = AddStringToList(OutStrings, EnvLight.LinkedCubemap);
				EnvLightHeader.Position = EnvLight.Position;

				DataStream.Put(&EnvLightHeader, sizeof(THeaderEnvLight));
			}

			// dependencies
			for (const auto& A : AssetTextures)
			{
				int32 Name = AddStringToList(OutStrings, A);
				DataStream.Put(&Name, sizeof(int32));
			}
			for (const auto& A : AssetMaterials)
			{
				int32 Name = AddStringToList(OutStrings, A);
				DataStream.Put(&Name, sizeof(int32));
			}
			for (const auto& A : AssetMaterialInstances)
			{
				int32 Name = AddStringToList(OutStrings, A);
				DataStream.Put(&Name, sizeof(int32));
			}
			for (const auto& A : AssetSkeletons)
			{
				int32 Name = AddStringToList(OutStrings, A);
				DataStream.Put(&Name, sizeof(int32));
			}
			for (const auto& A : AssetAnims)
			{
				int32 Name = AddStringToList(OutStrings, A);
				DataStream.Put(&Name, sizeof(int32));
			}
			for (const auto& A : AssetSMs)
			{
				int32 Name = AddStringToList(OutStrings, A);
				DataStream.Put(&Name, sizeof(int32));
			}
			TI_ASSERT(AssetSMs.size() == SMSections.size());
			for (const auto& A : SMSections)
			{
				int32 Count = A;
				DataStream.Put(&Count, sizeof(int32));
			}
			TI_ASSERT(AssetSMs.size() == SMInstanceCount.size());
			int32 TotalSMInstances = 0;
			for (const auto& A : SMInstanceCount)
			{
				int32 Count = A;
				DataStream.Put(&Count, sizeof(int32));
				TotalSMInstances += Count;
			}
			TI_ASSERT(TotalSMInstances == SceneTileHeader.NumSMInstances);
			for (const auto& A : SMInstances)
			{
				const TResSMInstance& Ins = A;
				DataStream.Put(&Ins, sizeof(TResSMInstance));
			}
			for (const auto& A : AssetSKMs)
			{
				int32 Name = AddStringToList(OutStrings, A);
				DataStream.Put(&Name, sizeof(int32));
			}
			for (const auto& A : SKMActors)
			{
				const TResSKMActor& Actor = A;
				DataStream.Put(&Actor, sizeof(TResSKMActor));
			}

			HeaderStream.Put(&SceneTileHeader, sizeof(THeaderSceneTile));
		}
		ChunkHeader.ChunkSize = HeaderStream.GetLength() + DataStream.GetLength();

		OutStream.Put(&ChunkHeader, sizeof(TResfileChunkHeader));
		OutStream.Put(HeaderStream.GetBuffer(), HeaderStream.GetLength());
		OutStream.Put(DataStream.GetBuffer(), DataStream.GetLength());
	}
}
