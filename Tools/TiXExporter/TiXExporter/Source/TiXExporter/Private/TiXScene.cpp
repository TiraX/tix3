// Fill out your copyright notice in the Description page of Project Settings.


#include "TiXScene.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Engine/DirectionalLight.h"
#include "Components/LightComponent.h"
#include "Engine/SkyLight.h"
#include "Components/SkyLightComponent.h"
#include "TiXExportFunctions.h"

FTiXSceneTile::FTiXSceneTile()
{
}

FTiXSceneTile::~FTiXSceneTile()
{
}

void FTiXSceneTile::AddStaticMeshActor(AActor* Actor, TArray<UStaticMesh*>& OutSM)
{
	check(Actor->IsA<AStaticMeshActor>());
	AStaticMeshActor* SMActor = Cast<AStaticMeshActor>(Actor);
	UStaticMesh* StaticMesh = SMActor->GetStaticMeshComponent()->GetStaticMesh();

	// Add to SM Instances
	TArray<FTiXSceneTile::FInstance>& Instances = SMInstances.FindOrAdd(StaticMesh);
	FTiXSceneTile::FInstance Instance;
	Instance.Position = SMActor->GetTransform().GetLocation() * FTiXExporterSetting::Setting.MeshVertexPositionScale;
	Instance.Rotation = SMActor->GetTransform().GetRotation();
	Instance.Scale = SMActor->GetTransform().GetScale3D();
	Instances.Add(Instance);
}

void FTiXSceneTile::AddFoliageActor(AActor* Actor)
{

}

void FTiXSceneTile::AddReflectionCaptureActor(AActor* Actor)
{

}

void FTiXSceneTile::UpdateSceneTileDesc()
{
	check(0);
	// Update statistics

	// Update dependencies

	// Update Instances
}

///////////////////////////////////////////////////////////

FTiXScene::FTiXScene(const FString& SceneName)
{
	SceneDesc.name = SceneName;
	SceneDesc.type = TEXT("scene");
	SceneDesc.version = 1;
	SceneDesc.desc = TEXT("Scene and scene tiles information from TiX exporter.");
}

FTiXScene::~FTiXScene()
{
	// Delete scene tiles
	for (auto T : SceneTiles)
	{
		delete T.Value;
	}
	SceneTiles.Empty();
}

void FTiXScene::DoExport()
{
	UpdateSceneDesc();
	FTiXExportFunctions::ExportScene(*this);
}

void FTiXScene::AddStaticMeshActor(AActor* Actor)
{
	// Add actor to scene tile
	TArray<UStaticMesh*> ActorStaticMeshes;
	GetActorTile(Actor)->AddStaticMeshActor(Actor, ActorStaticMeshes);

	// Add static meshes to resources
	for (auto SM : ActorStaticMeshes)
	{
		StaticMeshes.AddUnique(SM);
	}
}

void FTiXScene::AddFoliageActor(AActor* Actor)
{
	GetActorTile(Actor)->AddFoliageActor(Actor);
}

void FTiXScene::AddSkyLightActor(AActor* Actor)
{

}

void FTiXScene::AddCameraActor(AActor* Actor)
{
	check(Actor->IsA<ACameraActor>());
	ACameraActor* CamActor = Cast<ACameraActor>(Actor);
	UCameraComponent* CamComp = CamActor->GetCameraComponent();

	FTiXCamera Camera;
	Camera.location = CamComp->GetComponentToWorld().GetTranslation() * FTiXExporterSetting::Setting.MeshVertexPositionScale;
	Camera.rotator = CamComp->GetComponentToWorld().GetRotation().Rotator();
	FVector CamDir = CamComp->GetComponentToWorld().GetRotation().Vector();
	CamDir.Normalize();
	Camera.target = Camera.location + CamDir * 1.5f;
	Camera.fov = CamComp->FieldOfView;
	Camera.aspect = CamComp->AspectRatio;

	SceneDesc.cameras.Add(Camera);
}

void FTiXScene::AddReflectionCaptureActor(AActor* Actor)
{
	GetActorTile(Actor)->AddReflectionCaptureActor(Actor);
}

void FTiXScene::ApplyDirectionalLight(AActor* Actor)
{
	check(Actor->IsA<ADirectionalLight>());
	ADirectionalLight* DLightActor = Cast<ADirectionalLight>(Actor);
	ULightComponent* DLightComp = DLightActor->GetLightComponent();

	SceneDesc.environment.sun_light.name = DLightActor->GetName();
	SceneDesc.environment.sun_light.direction = DLightComp->GetDirection();
	SceneDesc.environment.sun_light.color = DLightComp->GetLightColor();
	SceneDesc.environment.sun_light.intensity = DLightComp->Intensity;
}

FTiXSceneTile* FTiXScene::GetActorTile(AActor* Actor)
{
	// Calc tile index by actor position
	FVector Location = Actor->GetTransform().GetLocation() * FTiXExporterSetting::Setting.MeshVertexPositionScale;

	if (FMath::IsNaN(Location.X) ||
		FMath::IsNaN(Location.Y) ||
		FMath::IsNaN(Location.Z))
	{
		return nullptr;
	}

	const float TileSize = FTiXExporterSetting::Setting.TileSize;
	float X = (Location.X + TileSize * 0.5f) / TileSize;
	float Y = (Location.Y + TileSize * 0.5f) / TileSize;
	if (X < 0.f)
	{
		X -= 1.f;
	}
	if (Y < 0.f)
	{
		Y -= 1.f;
	}
	FIntPoint TileIndex = FIntPoint(int32(X), int32(Y));
	FTiXSceneTile** Tile = SceneTiles.Find(TileIndex);
	if (Tile == nullptr)
	{
		// Create a new tile
		FTiXSceneTile* NewTile = CreateSceneTile(TileIndex);
		SceneTiles[TileIndex] = NewTile;
		return NewTile;
	}
	else
	{
		// Return exist tile
		return *Tile;
	}
}

FTiXSceneTile* FTiXScene::CreateSceneTile(const FIntPoint& TileIndex)
{
	FTiXSceneTile* Tile = new FTiXSceneTile;
	FString SceneName = SceneDesc.name;
	FString TileName = FString::Printf(TEXT("t%d_%d"), TileIndex.X, TileIndex.Y);
	Tile->SceneTileDesc.name = SceneName + TEXT("_") + TileName;
	Tile->SceneTileDesc.level = SceneName;
	Tile->SceneTileDesc.type = TEXT("scene_tile");
	Tile->SceneTileDesc.version = 1;
	Tile->SceneTileDesc.desc = TEXT("Scene tiles contains mesh instance information from TiX exporter.");
	Tile->SceneTileDesc.position = TileIndex;
	return Tile;
}

void FTiXScene::UpdateSceneDesc()
{
	SceneDesc.tiles.Empty();
	SceneDesc.tiles.Reserve(SceneTiles.Num());

	for (const auto& T : SceneTiles)
	{
		const FIntPoint& TileIndex = T.Key;
		SceneDesc.tiles.Add(TileIndex);
	}
}