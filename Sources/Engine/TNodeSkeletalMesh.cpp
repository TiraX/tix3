/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "TNodeSkeletalMesh.h"

namespace tix
{
	TNodeSkeletalMesh::TNodeSkeletalMesh(TNode* parent)
		: TNode(TNodeSkeletalMesh::NODE_TYPE, parent)
		, SkeletalMeshFlag(0)
	{
	}

	TNodeSkeletalMesh::~TNodeSkeletalMesh()
	{
	}

	void TNodeSkeletalMesh::Tick(float Dt)
	{
		TNode::Tick(Dt);

		if (Animation != nullptr)
		{
			TickAnimation(TEngine::GameTime());
		}

		BuildSkeletonMatrix();
	}

	void TNodeSkeletalMesh::UpdateAllTransformation()
	{
		TNode::UpdateAllTransformation();

		// TODO: Temp solution, use LocalToWorld matrix, use instance transform in future for GPU Driven
		if (LinkedPrimitive != nullptr)
		{
			// Update primtive uniform buffer
			FPrimitivePtr Primitive = LinkedPrimitive;
			FMat4 LocalToWorldMat = AbsoluteTransformation;
			ENQUEUE_RENDER_COMMAND(TNodeSkeletalMeshUpdatePrimitiveUniform)(
				[Primitive, LocalToWorldMat]()
				{
					Primitive->SetLocalToWorld(LocalToWorldMat);
					Primitive->UpdatePrimitiveBuffer_RenderThread();
				});
		}
	}

	void TNodeSkeletalMesh::SetSceneTileResource(TSceneTileResourcePtr InSceneTileResource)
	{
		SceneTileResourceRef = InSceneTileResource;
	}

	void TNodeSkeletalMesh::LinkMeshAndSkeleton(TStaticMeshPtr InMesh, TSkeletonPtr InSkeleton)
	{
		StaticMesh = InMesh;
		Skeleton = InSkeleton;
		TI_ASSERT(Skeleton->GetBones() <= TSkeleton::MaxBones);
		TI_ASSERT(0);
		TI_TODO("Find a way to add skeleton mesh to render scene");

		//// Generate primitives and send to render thread
		//LinkedPrimitive = ti_new FPrimitive;
		//LinkedPrimitive->SetSkeletalMesh(InMesh);

		//FPrimitivePtr Primitive = LinkedPrimitive;
		//if (SceneTileResourceRef != nullptr)
		//{
		//	// Add primitive to scene tile
		//	FSceneTileResourcePtr RenderThreadSceneTileResource = SceneTileResourceRef->RenderThreadTileResource;
		//	ENQUEUE_RENDER_COMMAND(AddTNodeSkeletalMeshPrimitivesToFSceneTile)(
		//		[RenderThreadSceneTileResource, Primitive]()
		//		{
		//			RenderThreadSceneTileResource->AppendPrimitive(Primitive);
		//		});
		//}
		//else
		//{
		//	// Add primitive to somewhere, like FScene??
		//	TI_ASSERT(0);
		//}

		SkeletalMeshFlag |= SKMatrixDirty;
	}

	void TNodeSkeletalMesh::SetAnimation(TAnimSequencePtr InAnim)
	{
		Animation = InAnim;
	}

	void TNodeSkeletalMesh::TickAnimation(float InTime)
	{
		// Calc time and interpolation time
		float AnimTime = InTime * Animation->GetRateScale();
		AnimTime = TMath::FMod(AnimTime, Animation->GetSequenceLength());

		const float FrameLength = Animation->GetFrameLength();

		const int32 Frame0 = (int32)(AnimTime / FrameLength);
		const int32 Frame1 = (Frame0 + 1) >= Animation->GetNumFrames() ? 0 : (Frame0 + 1);
		
		const float t = (AnimTime - FrameLength * Frame0) / FrameLength;

		// Go through tracks
		const TVector<TTrackInfo>& Tracks = Animation->GetTrackInfos();
		const float* FrameData = Animation->GetFrameData().data();

		const int32 NumTracks = (int32)Tracks.size();
		TI_ASSERT(NumTracks == Skeleton->GetBones());

		for (int32 track = 0; track < NumTracks; track++)
		{
			const TTrackInfo Track = Tracks[track];
			TI_ASSERT(Track.RefBoneIndex == track);

			const float* TrackKeys = FrameData + Track.KeyDataOffset;

			const FFloat3* PosKeys = (FFloat3*)TrackKeys;
			const FQuat* RotKeys = (FQuat*)(TrackKeys + Track.NumPosKeys * 3);
			const FFloat3* ScaleKeys = (FFloat3*)(TrackKeys + Track.NumPosKeys * 3 + Track.NumRotKeys * 4);

			FFloat3 Pos;
			FQuat Rot;
			FFloat3 Scale(1, 1, 1);

			if (Track.NumPosKeys > 0)
			{
				if (Track.NumPosKeys == 1)
				{
					Pos = PosKeys[0];
				}
				else
				{
					Pos = TMath::Lerp(PosKeys[Frame0], PosKeys[Frame1], t);
				}
			}

			if (Track.NumRotKeys > 0)
			{
				if (Track.NumRotKeys == 1)
				{
					Rot = RotKeys[0];
				}
				else
				{
					Rot.Slerp(RotKeys[Frame0], RotKeys[Frame1], t);
				}
			}

			if (Track.NumScaleKeys > 0)
			{
				if (Track.NumScaleKeys == 1)
				{
					Scale = ScaleKeys[0];
				}
				else
				{
					Scale = TMath::Lerp(ScaleKeys[Frame0], ScaleKeys[Frame1], t);
				}
			}
			Skeleton->SetBonePos(Track.RefBoneIndex, Pos);
			Skeleton->SetBoneRot(Track.RefBoneIndex, Rot);
			Skeleton->SetBoneScale(Track.RefBoneIndex, Scale);
		}

		// Need to update skeleton matrix
		SkeletalMeshFlag |= SKMatrixDirty;
	}

	void TNodeSkeletalMesh::BuildSkeletonMatrix()
	{
		if ((SkeletalMeshFlag & SKMatrixDirty) != 0)
		{
			// Build matrices from skeleton
			Skeleton->BuildGlobalPoses();

			TVector< TVector<float> > BoneDatas;
			BoneDatas.resize(LinkedPrimitive->GetNumSections());

			for (int32 p = 0; p < LinkedPrimitive->GetNumSections(); p++)
			{
				const TMeshSection& MeshSection = StaticMesh->GetMeshSection(p);

				SkeletonResources[p] = ti_new FUniformBuffer(sizeof(float) * 12 * TSkeleton::MaxBones, 1, 0);
				SkeletonResources[p]->SetResourceName(Skeleton->GetResourceName());

				Skeleton->GatherBoneData(BoneDatas[p], MeshSection.BoneMap);
			}
			TVector<FUniformBufferPtr> SkeletonDataResources = SkeletonResources;
			ENQUEUE_RENDER_COMMAND(TSkeletonUpdateSkeletonResource)(
				[SkeletonDataResources, BoneDatas]()
				{
					TI_ASSERT(SkeletonDataResources.size() == BoneDatas.size());
					for (int32 i = 0; i < (int32)SkeletonDataResources.size(); i++)
					{
						TStreamPtr BoneData = ti_new TStream(BoneDatas[i].data(), (uint32)(BoneDatas[i].size() * sizeof(float)));
						SkeletonDataResources[i]->CreateGPUBuffer(BoneData);
					}
				});

			// Link skeleton resource to primitives
			FPrimitivePtr Primitive = LinkedPrimitive;
			ENQUEUE_RENDER_COMMAND(PrimitiveSetSkeleton)(
				[Primitive, SkeletonDataResources]()
				{
					TI_ASSERT(Primitive->GetNumSections() == SkeletonDataResources.size());
					for (int32 p = 0; p < Primitive->GetNumSections(); ++p)
					{
						Primitive->SetSkeletonResource(p, SkeletonDataResources[p]);
					}
				});

			SkeletalMeshFlag &= ~SKMatrixDirty;
		}
	}
}
