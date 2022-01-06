// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TiXDefines.generated.h"

struct FTiXExporterSetting
{
	float TileSize;
	float MeshVertexPositionScale;
	bool bIgnoreMaterial;
	FString ExportPath;

	static FTiXExporterSetting Setting;

	FTiXExporterSetting()
		: TileSize(16.f)
		, MeshVertexPositionScale(0.01f)
		, bIgnoreMaterial(false)
	{}
};

// Directional Light
USTRUCT()
struct FTiXDirectionalLight
{
	GENERATED_BODY()

	UPROPERTY()
	FString name;

	UPROPERTY()
	FVector direction;

	UPROPERTY()
	FLinearColor color;

	UPROPERTY()
	float intensity;
};

// Sky Light
USTRUCT()
struct FTiXSkyLight
{
	GENERATED_BODY()

	UPROPERTY()
	FString name;

	UPROPERTY()
	TArray<float> irradiance_sh3;
};

// Camera Asset
USTRUCT()
struct FTiXCamera
{
	GENERATED_BODY()

	UPROPERTY()
	FVector location;

	UPROPERTY()
	FVector target;

	UPROPERTY()
	FRotator rotator;

	UPROPERTY()
	float fov;

	UPROPERTY()
	float aspect;
};

// Environment
USTRUCT()
struct FTiXEnvironment
{
	GENERATED_BODY()

	UPROPERTY()
	FTiXDirectionalLight sun_light;

	UPROPERTY()
	FTiXSkyLight sky_light;
};

// Scene
USTRUCT()
struct FTiXSceneDesc
{
	GENERATED_BODY()
		
	UPROPERTY()
	FString name;

	UPROPERTY()
	FString type;

	UPROPERTY()
	int32 version;

	UPROPERTY()
	FString desc;

	UPROPERTY()
	TArray<FTiXCamera> cameras;

	UPROPERTY()
	FTiXEnvironment environment;

	UPROPERTY()
	TArray<FIntPoint> tiles;
};

// Scene Tile Dependency
USTRUCT()
struct FTiXSceneTileDependency
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FString> textures;

	UPROPERTY()
	TArray<FString> materials;

	UPROPERTY()
	TArray<FString> material_instances;
	
	UPROPERTY()
	TArray<FString> static_meshes;
};

// Static Mesh Instance
USTRUCT()
struct FTiXSMInstance
{
	GENERATED_BODY()

	UPROPERTY()
	FVector position;

	UPROPERTY()
	FQuat rotation;

	UPROPERTY()
	FVector scale;
};

// Scene Tile Static Mesh Instance
USTRUCT()
struct FTiXSceneTileSMInstance
{
	GENERATED_BODY()

	UPROPERTY()
	FString linked_mesh;

	UPROPERTY()
	TArray<FTiXSMInstance> instances;
};

// Scene Tile
USTRUCT()
struct FTiXSceneTileDesc
{
	GENERATED_BODY()

	UPROPERTY()
	FString name;

	UPROPERTY()
	FString level;

	UPROPERTY()
	FString type;

	UPROPERTY()
	int32 version;

	UPROPERTY()
	FString desc;

	UPROPERTY()
	FIntPoint position;

	UPROPERTY()
	FBox bbox;

	UPROPERTY()
	int32 static_meshes_total;

	UPROPERTY()
	int32 sm_instances_total;
	
	UPROPERTY()
	int32 textures_total;
	
	UPROPERTY()
	int32 reflection_captures_total;

	UPROPERTY()
	FTiXSceneTileDependency dependency;

	UPROPERTY()
	TArray<FTiXSceneTileSMInstance> static_mesh_instances;
};