/*
TiX Engine v3.0 Copyright (C) 2022~2025
By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "TScene.h"

namespace tix
{
	TNodeSceneRoot::TNodeSceneRoot(TNode * Parent)
		: TNode(TNodeSceneRoot::NODE_TYPE, nullptr)
	{
	}
	TNodeSceneRoot::~TNodeSceneRoot()
	{
	}
	////////////////////////////////////////////////////////////////////////////
	TScene::TScene()
		: Flag(0)
	{
		// Create root element
		NodeRoot = TNodeFactory::CreateNode<TNodeSceneRoot>(nullptr);

		// Create default camera, this camera can only deleted by render stage.
		DefaultCamera = TNodeFactory::CreateNode<TNodeCameraNav>(NodeRoot);
		SetActiveCamera(DefaultCamera);

		// Create default environment
		DefaultEnvironment = TNodeFactory::CreateNode<TNodeEnvironment>(NodeRoot);
	}

	TScene::~TScene()
	{
		DefaultCamera->Remove();
		ti_delete DefaultCamera;

		DefaultEnvironment->Remove();
		ti_delete DefaultEnvironment;

		// remove root last
		ti_delete	NodeRoot;
	}

	void TScene::TickAllNodes(float Dt, TNode* Root)
	{
		if (Root == nullptr)
			Root = NodeRoot;

		Root->Tick(Dt);

		UpdateAllNodesTransforms();
	}

	void TScene::UpdateAllNodesTransforms(TNode* Root)
	{
		if (Root == nullptr)
			Root = NodeRoot;

		TI_TODO("Use parallel transform update.");
		Root->UpdateAllTransformation();
	}

	void TScene::SetActiveCamera(TNodeCamera* camera)
	{
		if (camera)
			ActiveCamera = camera;
		else
			ActiveCamera = DefaultCamera;
	}

	TNodeCamera* TScene::GetActiveCamera()
	{
		return ActiveCamera;
	}

	TNodeEnvironment* TScene::GetEnvironment()
	{
		return DefaultEnvironment;
	}

	TNodeStaticMesh* TScene::AddStaticMesh(TStaticMeshPtr InStaticMesh, TMaterialInstancePtr InMInstance, bool bCastShadow, bool bReceiveShadow)
	{
		// Create a static mesh node to hold mesh resource
		TNodeStaticMesh* StaticMesh = TNodeFactory::CreateNode<TNodeStaticMesh>(NodeRoot);

		// Link primitive to node
		StaticMesh->LinkStaticMesh(InStaticMesh, nullptr, 0, 0, bCastShadow, bReceiveShadow);
		
		return StaticMesh;
	}

	void TScene::AddStaticMeshNode(TNodeStaticMesh * MeshNode)
	{
		TI_ASSERT(IsGameThread());
		NodeRoot->AddChild(MeshNode);
	}

	TNodeLight* TScene::AddLight(const FFloat3& Position, float Intensity, const SColor& Color)
	{
		TNodeLight* Light = TNodeFactory::CreateNode<TNodeLight>(NodeRoot);
		Light->SetPosition(Position);
		Light->SetIntensity(Intensity);
		Light->SetColor(Color);
		Light->CreateFLight();
		return Light;
	}

	void TScene::ResetActiveLists()
	{
		for (int32 l = 0; l < ESLT_COUNT; ++l)
		{
			ActiveNodeList[l].clear();
		}
	}

	void TScene::AddToActiveList(E_SCENE_LIST_TYPE List, TNode * ActiveNode)
	{
		ActiveNodeList[List].push_back(ActiveNode);
	}

	void TScene::BindLights()
	{
		// Dynamic lighting will be re-design
		TI_ASSERT(0);
	}
}
