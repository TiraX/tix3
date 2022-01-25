/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "TNodeSceneTile.h"

namespace tix
{
	TSceneTileLoadingFinishDelegate::TSceneTileLoadingFinishDelegate(const TString& InLevelName, const TString& InTileName)
		: LevelName(InLevelName)
		, TileName(InTileName)
	{}

	TSceneTileLoadingFinishDelegate::~TSceneTileLoadingFinishDelegate()
	{}

	void TSceneTileLoadingFinishDelegate::LoadingFinished(TAssetPtr InAsset)
	{
		TI_ASSERT(IsGameThread());

		// Try to find Parent Level Node
		TNode* NodeLevel = TEngine::Get()->GetScene()->GetRoot()->GetNodeById(LevelName);
		if (NodeLevel != nullptr)
		{
			TI_ASSERT(NodeLevel->GetType() == ENT_Level);
			// Create scene tile node
			TNodeSceneTile * NodeSceneTile = TNodeFactory::CreateNode<TNodeSceneTile>(NodeLevel, TileName);

			// Hold references of Instances
			const TVector<TResourcePtr>& Resources = InAsset->GetResources();
			TI_ASSERT(Resources.size() == 1);
			TResourcePtr FirstResource = Resources[0];
			TI_ASSERT(FirstResource->GetType() == ERES_SCENE_TILE);

			NodeSceneTile->SceneTileResource = static_cast<TSceneTileResource*>(FirstResource.get());
			TI_ASSERT(NodeSceneTile->LoadedStaticMeshAssets.empty() && NodeSceneTile->LoadedSkeletalMeshAssets.empty());
			// Init Loaded mesh asset array.
			NodeSceneTile->LoadedStaticMeshAssets.resize(NodeSceneTile->SceneTileResource->SMInfos.MeshAssets.size());
			NodeSceneTile->LoadedSkeletalMeshAssets.resize(NodeSceneTile->SceneTileResource->SKMInfos.MeshAssets.size());
			// Init Loaded env light array
			NodeSceneTile->LoadedEnvLightInfos.resize(NodeSceneTile->SceneTileResource->EnvLightInfos.size());
		}
		else
		{
			_LOG(ELog::Warning, "Failed to find level node - [%s]\n", LevelName.c_str());
		}
	}

	//////////////////////////////////////////////////////////////////////////

	TNodeLevel::TNodeLevel(TNode* parent)
		: TNode(TNodeLevel::NODE_TYPE, parent)
	{
		TI_TODO("Add a THMap for finding tile node more fast.");
		TI_TODO("Override GetNodeByID method.");
	}

	TNodeLevel::~TNodeLevel()
	{
	}

	//////////////////////////////////////////////////////////////////////////

	TNodeSceneTile::TNodeSceneTile(TNode* parent)
		: TNode(TNodeSceneTile::NODE_TYPE, parent)
	{
	}

	TNodeSceneTile::~TNodeSceneTile()
	{
		TI_TODO("Remove scene tile resources from FScene, like meshes and env lights");
	}

	void TNodeSceneTile::Tick(float Dt)
	{
		// check if Asset is loaded
		if (SceneTileResource != nullptr)
		{
			LoadStaticMeshes();
			LoadSkeletalMeshes();
			LoadEnvCubemaps();
		}

		// Do loading first, then tick children, Loading skeletal meshes may create child SKM node
		TNode::Tick(Dt);

	}

	void TNodeSceneTile::LoadStaticMeshes()
	{
		const uint32 StaticMeshCount = (uint32)SceneTileResource->SMInfos.MeshAssets.size();
		if (StaticMeshCount > 0)
		{
			uint32 LoadedMeshCount = 0;
			for (uint32 m = 0; m < StaticMeshCount; ++m)
			{
				TAssetPtr MeshAsset = SceneTileResource->SMInfos.MeshAssets[m];

				if (MeshAsset != nullptr)
				{
					if (MeshAsset->IsLoaded())
					{
						// Gather loaded mesh resources
						const TVector<TResourcePtr>& MeshResources = MeshAsset->GetResources();
						TI_ASSERT(MeshResources[0]->GetType() == ERES_STATIC_MESH);
						TStaticMeshPtr StaticMesh = static_cast<TStaticMesh*>(MeshResources[0].get());

						// Add primitive to scene
						FInstanceBufferPtr InstanceBuffer = SceneTileResource->SMInstances.InstanceBuffer->InstanceBufferResource;
						FInt2 CountAndOffset = SceneTileResource->SMInstances.InstanceCountAndOffset[m];

						ENQUEUE_RENDER_COMMAND(AddTSceneTileMeshPrimitivesToFScene)(
							[StaticMesh, InstanceBuffer, CountAndOffset]()
							{
								FPrimitivePtr Primitive = ti_new FPrimitive;
								Primitive->InitFromInstancedStaticMesh(
									StaticMesh,
									InstanceBuffer,
									CountAndOffset.X,
									CountAndOffset.Y
								);
								FRenderThread::Get()->GetRenderScene()->AddPrimitive(Primitive);
							});

						// Remove the reference holder
						TI_ASSERT(LoadedStaticMeshAssets[m] == nullptr);
						LoadedStaticMeshAssets[m] = MeshAsset;
						SceneTileResource->SMInfos.MeshAssets[m] = nullptr;

						++LoadedMeshCount;
					}
				}
				else
				{
					++LoadedMeshCount;
				}
			}
			TI_ASSERT(LoadedMeshCount <= StaticMeshCount);
			if (LoadedMeshCount == StaticMeshCount)
			{
				// Tile Loading Done
				SceneTileResource->SMInfos.MeshAssets.clear();
			}
		}
	}

	void TNodeSceneTile::LoadSkeletalMeshes()
	{
		const uint32 SkeletalMeshCount = (uint32)SceneTileResource->SKMInfos.MeshAssets.size();
		if (SkeletalMeshCount > 0)
		{
			uint32 LoadedMeshCount = 0;
			for (uint32 m = 0; m < SkeletalMeshCount; ++m)
			{
				TAssetPtr MeshAsset = SceneTileResource->SKMInfos.MeshAssets[m];

				if (MeshAsset != nullptr)
				{
					if (MeshAsset->IsLoaded())
					{
						const TVector<TResourcePtr>& MeshResources = MeshAsset->GetResources();
						TI_ASSERT(MeshResources[0]->GetType() == ERES_STATIC_MESH);
						TStaticMeshPtr StaticMesh = static_cast<TStaticMesh*>(MeshResources[0].get());

						// Go through all actors used this mesh
						for (uint32 a = 0; a < SceneTileResource->SKMActorInfos.size(); a++)
						{
							const TSkeletalMeshActorInfo& SKMActorInfo = SceneTileResource->SKMActorInfos[a];
							if (SKMActorInfo.MeshAssetRef == MeshAsset)
							{
								// Skeleton asset
								TI_ASSERT(SKMActorInfo.SkeletonAsset != nullptr && SKMActorInfo.SkeletonAsset->IsLoaded());
								const TVector<TResourcePtr>& SkeletonResources = SKMActorInfo.SkeletonAsset->GetResources();
								TI_ASSERT(SkeletonResources[0]->GetType() == ERES_SKELETON);
								TSkeletonPtr Skeleton = static_cast<TSkeleton*>(SkeletonResources[0].get());

								// Animation asset
								TAnimSequencePtr Anim = nullptr;
								if (SKMActorInfo.AnimAsset != nullptr)
								{
									const TVector<TResourcePtr>& AnimResources = SKMActorInfo.AnimAsset->GetResources();
									TI_ASSERT(AnimResources[0]->GetType() == ERES_ANIM_SEQUENCE);
									Anim = static_cast<TAnimSequence*>(AnimResources[0].get());
								}

								// Create skeletal node
								TNodeSkeletalMesh* NodeSkeletalMesh = TNodeFactory::CreateNode<TNodeSkeletalMesh>(this, MeshAsset->GetName());
								NodeSkeletalMesh->SetSceneTileResource(SceneTileResource);
								NodeSkeletalMesh->LinkMeshAndSkeleton(StaticMesh, Skeleton);
								NodeSkeletalMesh->SetAnimation(Anim);
								NodeSkeletalMesh->SetPosition(SKMActorInfo.Pos);
								NodeSkeletalMesh->SetRotate(SKMActorInfo.Rot);
								NodeSkeletalMesh->SetScale(SKMActorInfo.Scale);
							}
						}

						// Remove the reference holder
						TI_ASSERT(LoadedSkeletalMeshAssets[m] == nullptr);
						LoadedSkeletalMeshAssets[m] = MeshAsset;
						SceneTileResource->SKMInfos.MeshAssets[m] = nullptr;

						++LoadedMeshCount;
					}
				}
				else
				{
					++LoadedMeshCount;
				}
			}
			TI_ASSERT(LoadedMeshCount <= SkeletalMeshCount);
			if (LoadedMeshCount == SkeletalMeshCount)
			{
				SceneTileResource->SMInfos.MeshAssets.clear();
			}
		}
	}

	void TNodeSceneTile::LoadEnvCubemaps()
	{
		const uint32 EnvCubemapCount = (uint32)SceneTileResource->EnvLights.size();
		if (EnvCubemapCount > 0)
		{
			uint32 LoadedCubemapCount = 0;
			for (uint32 c = 0; c < EnvCubemapCount; ++c)
			{
				TAssetPtr EnvLightAsset = SceneTileResource->EnvLights[c];
				const TSceneTileResource::TEnvLightInfo& EnvLightInfo = SceneTileResource->EnvLightInfos[c];
				if (EnvLightAsset != nullptr)
				{
					if (EnvLightAsset->IsLoaded())
					{
						const TVector<TResourcePtr>& CubeResources = EnvLightAsset->GetResources();
						TI_ASSERT(CubeResources[0]->GetType() == ERES_TEXTURE);
						TTexturePtr CubeTexture = static_cast<TTexture*>(CubeResources[0].get());

						// Add Env Light to FScene
						FTexturePtr CubeTextureResource = CubeTexture->TextureResource;
						FFloat3 Position = EnvLightInfo.Position;
						ENQUEUE_RENDER_COMMAND(AddEnvLightToFScene)(
							[CubeTextureResource, Position]()
							{
								FRenderThread::Get()->GetRenderScene()->SetEnvLight(CubeTextureResource, Position);
							});

						// Remove the reference holder
						TI_ASSERT(LoadedEnvLightInfos[c].Radius == 0.f);
						LoadedEnvLightInfos[c] = EnvLightInfo;
						SceneTileResource->EnvLights[c] = nullptr;

						++LoadedCubemapCount;
					}
				}
				else
				{
					++LoadedCubemapCount;
				}
			}
			TI_ASSERT(LoadedCubemapCount <= EnvCubemapCount);
			if (LoadedCubemapCount == EnvCubemapCount)
			{
				SceneTileResource->EnvLights.clear();
			}
		}
	}
}
