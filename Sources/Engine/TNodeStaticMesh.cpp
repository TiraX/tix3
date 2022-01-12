/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "TNodeStaticMesh.h"

namespace tix
{
	TNodeStaticMesh::TNodeStaticMesh(TNode* parent)
		: TNode(TNodeStaticMesh::NODE_TYPE, parent)
		, InstanceCount(0)
		, InstanceOffset(0)
	{
		// Duplicated, All static meshes now managed by TNodeSceneTile. 
		// Redesign like TNodeStaticMesh
		TI_ASSERT(0);
	}

	TNodeStaticMesh::~TNodeStaticMesh()
	{
		// Remove Primitive from scene
		//if (LinkedPrimitives.size() > 0)
		{
			//ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(RemoveStaticMeshPrimitiveFromScene,
			//	TVector<FPrimitivePtr>, Primitives, LinkedPrimitives,
			//	{
			//		FRenderThread::Get()->GetRenderScene()->RemoveStaticMeshPrimitives(Primitives);
			//	});
			//LinkedPrimitives.clear();
		}
	}

	void TNodeStaticMesh::UpdateAllTransformation()
	{
		// check if Asset is loaded
		if (MeshAsset != nullptr)
		{
			if (MeshAsset->IsLoaded())
			{
				TI_ASSERT(MeshAsset->GetResources().size() == 1);
				TI_ASSERT(MeshAsset->GetResources()[0]->GetType() == ERES_STATIC_MESH);
				TStaticMeshPtr StaticMesh = static_cast<TStaticMesh*>(MeshAsset->GetResources()[0].get());

				// Create static mesh node
				LinkStaticMesh(StaticMesh, MeshInstance, InstanceCount, InstanceOffset, false, false);

				// Remove the reference holder
				MeshAsset = nullptr;
				MeshInstance = nullptr;
			}
		}

		// No asset wait for loading
		if (MeshAsset == nullptr)
		{
			TNode::UpdateAllTransformation();
		}
	}

	void TNodeStaticMesh::LinkStaticMesh(
		TStaticMeshPtr InStaticMesh,
		TInstanceBufferPtr InInstanceBuffer,
		uint32 InInstanceCount,
		uint32 InInstanceOffset, 
		bool bCastShadow, 
		bool bReceiveShadow)
	{
		TI_ASSERT(0);	// Static mesh node never created here anymore
		// Create Primitives
		LinkedPrimitive = nullptr;

		TNode* SceneTileParent = GetParent(ENT_SceneTile);
		TNodeSceneTile * SceneTileNode = static_cast<TNodeSceneTile *>(SceneTileParent);

		LinkedPrimitive = ti_new FPrimitive;
		LinkedPrimitive->SetInstancedStaticMesh(
			InStaticMesh,
			InInstanceBuffer != nullptr ? InInstanceBuffer->InstanceBufferResource : nullptr,
			InInstanceCount,
			InInstanceOffset
		);

		// Add primitive to scene
		//ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(AddStaticMeshPrimitivesToScene,
		//	TVector<FPrimitivePtr>, Primitives, LinkedPrimitives,
		//	{
		//		FRenderThread::Get()->GetRenderScene()->AddStaticMeshPrimitives(Primitives);
		//	});
	}

	void TNodeStaticMesh::LinkMeshAsset(
		TAssetPtr InMeshAsset, 
		TInstanceBufferPtr InInstanceBuffer, 
		uint32 InInstanceCount, 
		uint32 InInstanceOffset, 
		bool bCastShadow, 
		bool bReceiveShadow)
	{
		// Save mesh asset
		MeshAsset = InMeshAsset;
		MeshInstance = InInstanceBuffer;
		InstanceCount = InInstanceCount;
		InstanceOffset = InInstanceOffset;
	}

	void TNodeStaticMesh::UpdateAbsoluteTransformation()
	{
		TNode::UpdateAbsoluteTransformation();

		if (HasFlag(ENF_ABSOLUTETRANSFORMATION_UPDATED))
		{
			TI_ASSERT(LinkedPrimitive != nullptr);
			TransformedBBox = LinkedPrimitive->GetVertexBuffer()->GetDesc().BBox;
			AbsoluteTransformation.TransformBoxEx(TransformedBBox);

			// Init uniform buffer resource in render thread
			FPrimitivePtr Primitive = LinkedPrimitive;
			FMat4 LocalToWorld = AbsoluteTransformation;
			ENQUEUE_RENDER_COMMAND(UpdatePrimitiveBuffer)(
				[Primitive, LocalToWorld]()
				{
					Primitive->SetLocalToWorld(LocalToWorld);
				});
		}

		// add this to static solid list
		TEngine::Get()->GetScene()->AddToActiveList(ESLT_STATIC_SOLID, this);
	}

	//static void BindLightResource_RenderThread(const TVector<FPrimitivePtr>& Primitives, const TVector<FLightPtr>& BindedLightResources)
	//{
	//	FPrimitiveUniformBufferPtr Binding = ti_new FPrimitiveUniformBuffer;
	//	Binding->UniformBufferData[0].LightsNum.X = (int32)BindedLightResources.size();

	//	TI_ASSERT(BindedLightResources.size() <= 4);
	//	for (int32 l = 0; l < (int32)BindedLightResources.size(); ++l)
	//	{
	//		FLightPtr LightResource = BindedLightResources[l];
	//		Binding->UniformBufferData[0].LightIndices[l] = LightResource->GetLightIndex();
	//	}
	//	Binding->InitUniformBuffer();

	//	for (auto P : Primitives)
	//	{
	//		P->SetPrimitiveUniform(Binding);
	//	}
	//}

	void TNodeStaticMesh::BindLights(TVector<TNode *>& Lights, bool ForceRebind)
	{
		// Dynamic lighting will be re-designed
		TI_ASSERT(0);
		//if (HasFlag(ENF_ABSOLUTETRANSFORMATION_UPDATED) || ForceRebind)
		//{
		//	BindedLights.clear();

		//	// Find out lights affect this mesh
		//	for (auto Light : Lights)
		//	{
		//		TI_ASSERT(Light->GetType() == ENT_Light);
		//		TNodeLight * LightNode = static_cast<TNodeLight*>(Light);
		//		if (TransformedBBox.intersectsWithBox(LightNode->GetAffectBox()))
		//		{
		//			BindedLights.push_back(LightNode);
		//		}
		//	}

		//	// Create lights info uniform buffers
		//	TVector<FLightPtr> BindedLightResources;
		//	if (BindedLights.size() > 0)
		//	{
		//		TI_ASSERT(BindedLights.size() <= 4);
		//		for (int32 l = 0 ; l < (int32)BindedLights.size(); ++ l)
		//		{
		//			TNodeLight * Light = BindedLights[l];
		//			BindedLightResources.push_back(Light->LightResource);
		//		}
		//	}
		//	// Init uniform buffer resource in render thread
		//	ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(PrimitiveInitLightBindingUB,
		//		TVector<FPrimitivePtr>, Primitives, LinkedPrimitives,
		//		TVector<FLightPtr>, BindedLightResources, BindedLightResources,
		//		{
		//			BindLightResource_RenderThread(Primitives, BindedLightResources);
		//		});
		//}
	}
}
