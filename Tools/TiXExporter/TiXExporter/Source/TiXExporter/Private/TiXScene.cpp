// Fill out your copyright notice in the Description page of Project Settings.


#include "TiXScene.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstance.h"
#include "InstancedFoliageActor.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Engine/DirectionalLight.h"
#include "Components/LightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/SkyLight.h"
#include "Components/SkyLightComponent.h"
#include "Engine/ReflectionCapture.h"
#include "Components/ReflectionCaptureComponent.h"
#include "Engine/MapBuildDataRegistry.h"
#include "TiXExportFunctions.h"

FTiXSceneTile::FTiXSceneTile()
{
}

FTiXSceneTile::~FTiXSceneTile()
{
}

void FTiXSceneTile::AddStaticMeshActor(AActor* Actor)
{
	check(Actor->IsA(AStaticMeshActor::StaticClass()));
	AStaticMeshActor* SMActor = Cast<AStaticMeshActor>(Actor);
	UStaticMesh* StaticMesh = SMActor->GetStaticMeshComponent()->GetStaticMesh();

	// Add to SM Instances
	TArray<FTiXSceneTile::FInstance>& Instances = SMInstances.FindOrAdd(StaticMesh);
	FTiXSceneTile::FInstance Instance;
	Instance.Transform = SMActor->GetTransform();
	Instances.Add(Instance);
}

void FTiXSceneTile::AddFoliageActor(AActor* Actor)
{
	check(Actor->IsA(AInstancedFoliageActor::StaticClass()));
	AInstancedFoliageActor* FoliageActor = Cast<AInstancedFoliageActor>(Actor);

	for (const auto& FoliagePair : FoliageActor->GetFoliageInfos())
	{
		const FFoliageInfo& FoliageInfo = *FoliagePair.Value;

		UHierarchicalInstancedStaticMeshComponent* MeshComponent = FoliageInfo.GetComponent();
		TArray<FInstancedStaticMeshInstanceData> MeshDataArray = MeshComponent->PerInstanceSMData;

		UStaticMesh* StaticMesh = MeshComponent->GetStaticMesh();
		TArray<FTiXSceneTile::FInstance>& Instances = SMInstances.FindOrAdd(StaticMesh);

		for (auto& MeshMatrix : MeshDataArray)
		{
			FTiXSceneTile::FInstance Instance;
			Instance.Transform = FTransform(MeshMatrix.Transform);
			Instances.Add(Instance);
		}
	}
}

void FTiXSceneTile::AddReflectionCaptureActor(AActor* Actor)
{
	check(Actor->IsA(AReflectionCapture::StaticClass()));
	AReflectionCapture* RCActor = Cast<AReflectionCapture>(Actor);
	RCActors.AddUnique(RCActor);
}

void FTiXSceneTile::UpdateSceneTileDesc()
{
	// Update dependencies
	UpdateDependencies();

	// Update Instances
	UpdateInstancesDesc();

	// Update statistics
	UpdateStatisticsDesc();
}

void FTiXSceneTile::UpdateDependencies()
{
	Materials.Empty();
	MaterialInstances.Empty();
	Textures.Empty();

	if (!FTiXExporterSetting::Setting.bIgnoreMaterial)
	{
		// Analysis materials from static meshes
		for (const auto& MeshIter : SMInstances)
		{
			UStaticMesh* SM = MeshIter.Key;
			const int32 LOD = 0;
			FStaticMeshLODResources& LODResource = SM->GetRenderData()->LODResources[LOD];
			for (int32 S = 0; S < LODResource.Sections.Num(); ++S)
			{
				FStaticMeshSection& Section = LODResource.Sections[S];
				UMaterialInterface* MI = SM->GetStaticMaterials()[Section.MaterialIndex].MaterialInterface;
				if (MI == nullptr)
					continue;

				if (MI->IsA(UMaterial::StaticClass()))
				{
					Materials.AddUnique(Cast<UMaterial>(MI));
				}
				else
				{
					check(MI->IsA(UMaterialInstance::StaticClass()));
					UMaterialInstance* MInstance = Cast<UMaterialInstance>(MI);
					MaterialInstances.AddUnique(MInstance);

					// Add material instance's parent material
					UMaterialInterface* ParentMaterial = MInstance->Parent;
					while (ParentMaterial->IsA(UMaterialInstance::StaticClass()))
					{
						ParentMaterial = Cast<UMaterialInstance>(ParentMaterial)->Parent;
					}
					check(ParentMaterial != nullptr && ParentMaterial->IsA(UMaterial::StaticClass()));
					Materials.AddUnique(Cast<UMaterial>(ParentMaterial));

					// Analysis used textures from material instances
					for (const auto& TexParam : MInstance->TextureParameterValues)
					{
						UTexture* Texture = TexParam.ParameterValue;
						Textures.AddUnique(Texture);
					}
				}
			}
		}
	}

	// Update dependency desc
	FTiXSceneTileDependency& Dependency = SceneTileDesc.dependency;
	Dependency.textures.Reserve(Textures.Num() + RCActors.Num());
	for (auto T : Textures)
	{
		Dependency.textures.Add(FTiXExportFunctions::GetResourcePathName(T) + FTiXExporterSetting::Setting.ExtName);
	}
	for (auto RCA : RCActors)
	{
		UWorld* RCWorld = RCA->GetWorld();
		FString RCMapPath = RCWorld->GetName() + TEXT("/TC_") + RCA->GetName() + FTiXExporterSetting::Setting.ExtName;
		Dependency.textures.Add(RCMapPath);
	}
	Dependency.material_instances.Reserve(MaterialInstances.Num());
	for (auto MI : MaterialInstances)
	{
		Dependency.material_instances.Add(FTiXExportFunctions::GetResourcePathName(MI) + FTiXExporterSetting::Setting.ExtName);
	}
	Dependency.materials.Reserve(Materials.Num());
	for (auto M : Materials)
	{
		Dependency.materials.Add(FTiXExportFunctions::GetResourcePathName(M) + FTiXExporterSetting::Setting.ExtName);
	}
	Dependency.static_meshes.Reserve(SMInstances.Num());
	for (const auto& MIter : SMInstances)
	{
		Dependency.static_meshes.Add(FTiXExportFunctions::GetResourcePathName(MIter.Key) + FTiXExporterSetting::Setting.ExtName);
	}
}

void FTiXSceneTile::UpdateInstancesDesc()
{
	// Update reflection actor
	SceneTileDesc.reflection_captures.Reserve(RCActors.Num());
	for (const auto& RCA : RCActors)
	{
		FTiXReflectionCapture RCDesc;
		RCDesc.name = RCA->GetName();
		UWorld* RCWorld = RCA->GetWorld();
		RCDesc.linked_cubemap = RCWorld->GetName() + TEXT("/TC_") + RCA->GetName() + FTiXExporterSetting::Setting.ExtName;

		UReflectionCaptureComponent* RCComponent = RCA->GetCaptureComponent();
		FReflectionCaptureData ReadbackCaptureData;
		RCWorld->Scene->GetReflectionCaptureData(RCComponent, ReadbackCaptureData);
		RCDesc.cubemap_size = ReadbackCaptureData.CubemapSize;
		RCDesc.average_brightness = ReadbackCaptureData.AverageBrightness;
		RCDesc.brightness = ReadbackCaptureData.Brightness;
		RCDesc.position = ToArray(RCA->GetTransform().GetLocation() * FTiXExporterSetting::Setting.MeshVertexPositionScale);
		SceneTileDesc.reflection_captures.Add(RCDesc);
	}

	// Update static mesh instances
	TArray<FTiXSceneTileSMInstance>& SMInstancesDesc = SceneTileDesc.static_mesh_instances;
	SMInstancesDesc.Empty();
	SMInstancesDesc.Reserve(SMInstances.Num());
	for (const auto& Iter : SMInstances)
	{
		FTiXSceneTileSMInstance SMInstanceDesc;
		SMInstanceDesc.linked_mesh = FTiXExportFunctions::GetResourcePathName(Iter.Key) + FTiXExporterSetting::Setting.ExtName;
		SMInstanceDesc.instances.Reserve(Iter.Value.Num());
		for (const auto& SMIns : Iter.Value)
		{
			FTiXSMInstance InsDesc;
			InsDesc.position = ToArray(SMIns.Transform.GetTranslation() * FTiXExporterSetting::Setting.MeshVertexPositionScale);
			InsDesc.rotation = ToArray(SMIns.Transform.GetRotation());
			InsDesc.scale = ToArray(SMIns.Transform.GetScale3D());
			SMInstanceDesc.instances.Add(InsDesc);
		}
		SMInstancesDesc.Add(SMInstanceDesc);
	}
}

void FTiXSceneTile::UpdateStatisticsDesc()
{
	// Update resources count
	SceneTileDesc.static_meshes_total = SMInstances.Num();
	SceneTileDesc.sm_instances_total = 0;
	for (const auto& Iter : SMInstances)
	{
		SceneTileDesc.sm_instances_total += Iter.Value.Num();
	}
	SceneTileDesc.textures_total = Textures.Num();
	SceneTileDesc.reflection_captures_total = RCActors.Num();

	// Update BBox
	FBox bbox;
	bbox.Init();
	for (const auto& Iter : SMInstances)
	{
		UStaticMesh* SM = Iter.Key;
		FBox SMBox = SM->GetBoundingBox();
		SMBox.Min *= FTiXExporterSetting::Setting.MeshVertexPositionScale;
		SMBox.Max *= FTiXExporterSetting::Setting.MeshVertexPositionScale;

		for (const auto& SMIns : Iter.Value)
		{
			FBox TranslatedBox = SMBox.TransformBy(SMIns.Transform);
			if (bbox.IsValid == 0)
				bbox = TranslatedBox;
			else
				bbox += TranslatedBox;
		}
	}
	SceneTileDesc.bbox = ToArray(bbox);
}

///////////////////////////////////////////////////////////

FTiXScene::FTiXScene(UWorld* InWorld)
	: SceneWorld(InWorld)
	, SkylightCube(nullptr)
{
	SceneDesc.name = InWorld->GetName();
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
	GetActorTile(Actor)->AddStaticMeshActor(Actor);
}

void FTiXScene::AddFoliageActor(AActor* Actor)
{
	GetActorTile(Actor)->AddFoliageActor(Actor);
}

void FTiXScene::AddCameraActor(AActor* Actor)
{
	check(Actor->IsA(ACameraActor::StaticClass()));
	ACameraActor* CamActor = Cast<ACameraActor>(Actor);
	UCameraComponent* CamComp = CamActor->GetCameraComponent();

	FTiXCamera Camera;
	FVector CamPos = CamComp->GetComponentToWorld().GetTranslation() * FTiXExporterSetting::Setting.MeshVertexPositionScale;
	Camera.location = ToArray(CamPos);
	Camera.rotator = ToArray(CamComp->GetComponentToWorld().GetRotation().Rotator());
	FVector CamDir = CamComp->GetComponentToWorld().GetRotation().Vector();
	CamDir.Normalize();
	Camera.target = ToArray(CamPos + CamDir * 1.5f);
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
	check(Actor->IsA(ADirectionalLight::StaticClass()));
	ADirectionalLight* DLightActor = Cast<ADirectionalLight>(Actor);
	ULightComponent* DLightComp = DLightActor->GetLightComponent();

	SceneDesc.environment.sun_light.name = DLightActor->GetName();
	SceneDesc.environment.sun_light.direction = ToArray(DLightComp->GetDirection());
	SceneDesc.environment.sun_light.color = ToArray(DLightComp->GetLightColor());
	SceneDesc.environment.sun_light.intensity = DLightComp->Intensity;
}

void FTiXScene::ApplySkyLight(AActor* Actor)
{
	check(Actor->IsA(ASkyLight::StaticClass()));
	USkyLightComponent::UpdateSkyCaptureContents(Actor->GetWorld());
	ASkyLight* SkyLightActor = Cast<ASkyLight>(Actor);
	USkyLightComponent* SkyLightComp = SkyLightActor->GetLightComponent();

	SceneDesc.environment.sky_light.name = SkyLightActor->GetName();
	SceneDesc.environment.sky_light.irradiance_sh3 = ToArray(SkyLightComp->GetIrradianceEnvironmentMap());
	if (SkyLightComp->SourceType == SLS_SpecifiedCubemap)
	{
		SceneDesc.environment.sky_light.cube_source = FTiXExportFunctions::GetResourcePathName(SkyLightComp->Cubemap) + FTiXExporterSetting::Setting.ExtName;
		SceneDesc.environment.sky_light.cube_angle = SkyLightComp->SourceCubemapAngle;
		SkylightCube = SkyLightComp->Cubemap;
	}
	else
	{
		SceneDesc.environment.sky_light.cube_angle = 0;
	}
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
		SceneTiles.Add(TileIndex, NewTile);
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
	Tile->SceneTileDesc.position = ToArray(TileIndex);
	return Tile;
}

void FTiXScene::UpdateSceneDesc()
{
	// Update reflection capture at very first
	FString UpdateReason = TEXT("Update for TiX reflection capture export");
	UReflectionCaptureComponent::UpdateReflectionCaptureContents(SceneWorld, *UpdateReason, true);

	// Update scene tile descs first
	for (const auto& TileIter : SceneTiles)
	{
		FTiXSceneTile* Tile = TileIter.Value;
		Tile->UpdateSceneTileDesc();
	}

	// Update scene desc
	SceneDesc.tiles.Empty();
	SceneDesc.tiles.Reserve(SceneTiles.Num());

	for (const auto& T : SceneTiles)
	{
		const FIntPoint& TileIndex = T.Key;
		SceneDesc.tiles.Add(TileIndex);
	}

	// Collect UStaticMesh, UTexture, UMaterial, UMaterialInstances, RCActors from tiles
	StaticMeshes.Empty(1024);
	Textures.Empty(1024);
	Materials.Empty(1024);
	MaterialInstances.Empty(1024);
	RCActors.Empty(1024);

	for (const auto& T : SceneTiles)
	{
		FTiXSceneTile* Tile = T.Value;
		for (const auto& SM : Tile->SMInstances)
		{
			StaticMeshes.AddUnique(SM.Key);
		}
		for (const auto& Tex : Tile->Textures)
		{
			Textures.AddUnique(Tex);
		}
		for (const auto& M : Tile->Materials)
		{
			Materials.AddUnique(M);
		}
		for (const auto& MI : Tile->MaterialInstances)
		{
			MaterialInstances.AddUnique(MI);
		}
		for (const auto& RCA : Tile->RCActors)
		{
			RCActors.AddUnique(RCA);
		}
	}
}