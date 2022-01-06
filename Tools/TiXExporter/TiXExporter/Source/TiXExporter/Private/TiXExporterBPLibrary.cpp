// Copyright Epic Games, Inc. All Rights Reserved.

#include "TiXExporterBPLibrary.h"
#include "TiXExporter.h"
#include "TiXDefines.h"
#include "TiXScene.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/SkeletalMeshActor.h"
#include "InstancedFoliageActor.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/ReflectionCapture.h"
#include "Components/ReflectionCaptureComponent.h"
#include "Camera/CameraActor.h"

DEFINE_LOG_CATEGORY(LogTiXExporter);

FTiXExporterSetting FTiXExporterSetting::Setting;

void UTiXExporterBPLibrary::SetTileSize(float TileSize)
{
	FTiXExporterSetting::Setting.TileSize = TileSize;
}

void UTiXExporterBPLibrary::SetMeshVertexPositionScale(float MeshVertexPositionScale)
{
	FTiXExporterSetting::Setting.MeshVertexPositionScale = MeshVertexPositionScale;
}

void UTiXExporterBPLibrary::SetIgnoreMaterial(bool bIgnore)
{
	FTiXExporterSetting::Setting.bIgnoreMaterial = bIgnore;
}

void UTiXExporterBPLibrary::SetExportPath(const FString& ExportPath)
{
	FTiXExporterSetting::Setting.ExportPath = ExportPath;
}



UTiXExporterBPLibrary::UTiXExporterBPLibrary(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{

}

void UTiXExporterBPLibrary::ExportCurrentScene(AActor* Actor, const FString& ExportPath, const TArray<FString>& SceneComponents, const TArray<FString>& MeshComponents)
{
	UWorld* CurrentWorld = Actor->GetWorld();
	ULevel* CurrentLevel = CurrentWorld->GetCurrentLevel();

	FTiXScene Scene(CurrentWorld->GetName());
	TArray<AActor*> Actors;

	UE_LOG(LogTiXExporter, Log, TEXT("Exporting tix scene ..."));

	// Load all static mesh actors
	UGameplayStatics::GetAllActorsOfClass(Actor, AStaticMeshActor::StaticClass(), Actors);
	for (auto A : Actors)
	{
		if (A->IsHidden())
			continue;

		Scene.AddStaticMeshActor(A);
	}

	// Load all skeletal mesh actors

	// Load all foliages
	Actors.Empty();
	UGameplayStatics::GetAllActorsOfClass(Actor, AInstancedFoliageActor::StaticClass(), Actors);
	for (auto A : Actors)
	{
		if (A->IsHidden())
			continue;

		Scene.AddFoliageActor(A);
	}

	// Load all sky lights
	Actors.Empty();
	UGameplayStatics::GetAllActorsOfClass(Actor, ASkyLight::StaticClass(), Actors);
	for (auto A : Actors)
	{
		if (A->IsHidden())
			continue;

		Scene.AddSkyLightActor(A);
	}

	// Collect Reflection Captures
	Actors.Empty();
	UGameplayStatics::GetAllActorsOfClass(Actor, AReflectionCapture::StaticClass(), Actors);
	for (auto A : Actors)
	{
		if (A->IsHidden())
			continue;

		Scene.AddReflectionCaptureActor(A);
	}

	// Get directional light
	Actors.Empty();
	UGameplayStatics::GetAllActorsOfClass(Actor, ADirectionalLight::StaticClass(), Actors);
	if (Actors.Num() > 0)
	{
		Scene.ApplyDirectionalLight(Actors[0]);
	}

	// Load cameras
	Actors.Empty();
	UGameplayStatics::GetAllActorsOfClass(Actor, ACameraActor::StaticClass(), Actors);
	for (auto A : Actors)
	{
		if (A->IsHidden())
			continue;

		Scene.AddCameraActor(A);
	}

	// Do Export
	Scene.DoExport();
}

