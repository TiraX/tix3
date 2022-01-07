// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TiXDefines.h"

class FTiXSceneTile
{
public:
	FTiXSceneTile();
	~FTiXSceneTile();

	void AddStaticMeshActor(AActor* Actor);
	void AddFoliageActor(AActor* Actor);

	void UpdateSceneTileDesc();
private:
	void UpdateDependencies();
	void UpdateInstancesDesc();
	void UpdateStatisticsDesc();

	FTiXSceneTileDesc SceneTileDesc;

	struct FInstance
	{
		FInstance()
		{}
		FTransform Transform;
	};
	TMap<UStaticMesh*, TArray<FInstance>> SMInstances;
	TArray<UMaterial*> Materials;
	TArray<UMaterialInstance*> MaterialInstances;
	TArray<UTexture*> Textures;

	friend class FTiXScene;
	friend class FTiXExportFunctions;
};

class FTiXScene
{
public:
	FTiXScene(const FString& SceneName);
	~FTiXScene();

	void DoExport();

	void AddStaticMeshActor(AActor* Actor);
	void AddFoliageActor(AActor* Actor);
	void AddReflectionCaptureActor(AActor* Actor);
	void AddCameraActor(AActor* Actor);
	void ApplyDirectionalLight(AActor* Actor);
	void ApplySkyLight(AActor* Actor);

private:
	FTiXSceneTile* GetActorTile(AActor* Actor);
	FTiXSceneTile* CreateSceneTile(const FIntPoint& TileIndex);
	void UpdateSceneDesc();

private:
	FTiXSceneDesc SceneDesc;

	// Scene tiles
	TMap<FIntPoint, FTiXSceneTile*> SceneTiles;

	// Resources to export
	TArray<UStaticMesh*> StaticMeshes;
	TArray<UTexture*> Textures;
	TArray<UMaterial*> Materials;
	TArray<UMaterialInstance*> MaterialInstances;

	friend class FTiXExportFunctions;
};