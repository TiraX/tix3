/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	struct TMeshInfoInTile
	{
		uint32 NumMeshes;
		TVector<TAssetPtr> MeshAssets;

		TMeshInfoInTile()
			: NumMeshes(0)
		{}
	};
	struct TStaticMeshInstanceInfo
	{
		uint32 NumInstances;	// Total static mesh instances
		TVector<FInt2> InstanceCountAndOffset;	// X is Count, Y is Offset
		TInstanceBufferPtr InstanceBuffer;

		TStaticMeshInstanceInfo()
			: NumInstances(0)
		{}
	};
	struct TSkeletalMeshActorInfo
	{
		TAssetPtr MeshAssetRef;
		TAssetPtr SkeletonAsset;
		TAssetPtr AnimAsset;
		FFloat3 Pos;
		FQuat Rot;
		FFloat3 Scale;
	};

	// Hold all resources in a tile, like meshes, instances, etc
	class TSceneTileResource : public TResource
	{
	public:
		TSceneTileResource();
		~TSceneTileResource();

		TInstanceBufferPtr GetInstanceBuffer()
		{
			return SMInstances.InstanceBuffer;
		}

	public:
		virtual void InitRenderThreadResource() override;
		virtual void DestroyRenderThreadResource() override;

	public:
		TString LevelName;
		FInt2 Position;
		FBox BBox;

		uint32 TotalEnvLights;
		TVector<TAssetPtr> EnvLights;
		struct TEnvLightInfo
		{
			FFloat3 Position;
			float Radius;

			TEnvLightInfo()
				: Radius(0.f)
			{}
		};
		TVector<TEnvLightInfo> EnvLightInfos;

		// Static Meshes
		// Static meshes always processed with instances
		TMeshInfoInTile SMInfos;
		TStaticMeshInstanceInfo SMInstances;

		// Skeletal Meshes
		// Skeletal mesh always processed with actors
		TMeshInfoInTile SKMInfos;
		TVector<TSkeletalMeshActorInfo> SKMActorInfos;

		FSceneTileResourcePtr RenderThreadTileResource;
	};
}
