// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TiXDefines.h"

class FTiXSceneTile
{
public:
	FTiXSceneTile();
	~FTiXSceneTile();

	void AddStaticMeshActor(AActor* Actor, TArray<UStaticMesh*>& OutSM);
	void AddFoliageActor(AActor* Actor);
	void AddReflectionCaptureActor(AActor* Actor);

	void UpdateSceneTileDesc();
private:
	FTiXSceneTileDesc SceneTileDesc;

	struct FInstance
	{
		FInstance()
			: Scale(1, 1, 1)
		{}

		FVector Position;
		FQuat Rotation;
		FVector Scale;
	};
	TMap<UStaticMesh*, TArray<FInstance>> SMInstances;

	friend class FTiXScene;
};

class FTiXScene
{
public:
	FTiXScene(const FString& SceneName);
	~FTiXScene();

	void DoExport();

	void AddStaticMeshActor(AActor* Actor);
	void AddFoliageActor(AActor* Actor);
	void AddSkyLightActor(AActor* Actor);
	void AddReflectionCaptureActor(AActor* Actor);
	void AddCameraActor(AActor* Actor);
	void ApplyDirectionalLight(AActor* Actor);

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

	friend class FTiXExportFunctions;
};