/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "TSkeleton.h"

namespace tix
{
	inline void MakeMatrix(FMat4& Mat, const FFloat3& Pos, const FQuat& Rot, const FFloat3& Scale)
	{
		Rot.GetMatrix(Mat);
		Mat.PostScale(Scale);
		Mat.SetTranslation(Pos);
	}

	TSkeleton::TSkeleton()
		: TResource(ERES_SKELETON)
	{
	}

	TSkeleton::TSkeleton(int32 NumBones)
		: TResource(ERES_SKELETON)
	{
		Bones.reserve(NumBones);

		InvBindMatrix.resize(NumBones);
	}

	TSkeleton::~TSkeleton()
	{
	}

	void TSkeleton::AddBone(int32 ParentIndex, const FFloat3& InvTrans, const FQuat& InvRot, const FFloat3& InvScale)
	{
		TBoneInfo Bone;
		Bone.ParentIndex = ParentIndex;
		Bone.Pos = InvTrans;
		Bone.Rot = InvRot;
		Bone.Scale = InvScale;

		AddBone(Bone);
	}

	void TSkeleton::ComputeInvBindMatrices()
	{
		TVector<FMat4> BindMatrix;
		BindMatrix.resize(Bones.size());

		const int32 NumBones = (int32)Bones.size();
		for (int32 b = 0; b < NumBones; b++)
		{
			const TBoneInfo& Bone = Bones[b];
			if (Bone.ParentIndex == -1)
			{
				MakeMatrix(BindMatrix[b], Bone.Pos, Bone.Rot, Bone.Scale);
			}
			else
			{
				FMat4 ParentMat = BindMatrix[Bone.ParentIndex];
				FMat4 Mat;
				MakeMatrix(Mat, Bone.Pos, Bone.Rot, Bone.Scale);
				BindMatrix[b] =  ParentMat * Mat;
			}
		}

		InvBindMatrix.resize(Bones.size());
		for (int32 b = 0; b < NumBones; b++)
		{
			BindMatrix[b].GetInverse(InvBindMatrix[b]);
		}
	}

	void TSkeleton::AddBone(const TBoneInfo& Bone)
	{
		Bones.push_back(Bone);
	}

	void TSkeleton::SetBonePos(int32 BoneIndex, const FFloat3& InPos)
	{
		Bones[BoneIndex].Pos = InPos;
	}

	void TSkeleton::SetBoneRot(int32 BoneIndex, const FQuat& InRot)
	{
		Bones[BoneIndex].Rot = InRot;
	}

	void TSkeleton::SetBoneScale(int32 BoneIndex, const FFloat3& InScale)
	{
		Bones[BoneIndex].Scale = InScale;
	}

	void TSkeleton::InitRenderThreadResource()
	{
		////TI_ASSERT(SkeletonResource == nullptr);
		//// Skeleton Bone info resource always need to re-create
		//SkeletonResource = FRHI::Get()->CreateUniformBuffer(sizeof(float)*12*MaxBones, 1, 0);
		//SkeletonResource->SetResourceName(GetResourceName());

		//FUniformBufferPtr SkeletonDataResource = SkeletonResource;
		//TVector<float> BoneData = BoneMatricsData;
		//ENQUEUE_RENDER_COMMAND(TSkeletonUpdateSkeletonResource)(
		//	[SkeletonDataResource, BoneData]()
		//	{
		//		FRHI::Get()->UpdateHardwareResourceUB(SkeletonDataResource, BoneData.data());
		//	});
	}

	void TSkeleton::DestroyRenderThreadResource()
	{
		//TI_ASSERT(SkeletonResource != nullptr);
		//
		//FUniformBufferPtr SkeletonDataResource = SkeletonResource;
		//ENQUEUE_RENDER_COMMAND(TSkeletonDestroySkeletonResource)(
		//	[SkeletonDataResource]()
		//	{
		//		//SkeletonDataResource = nullptr;
		//	});
		//SkeletonDataResource = nullptr;
		//SkeletonResource = nullptr;
	}

	void TSkeleton::BuildGlobalPoses()
	{
		GlobalPoses.resize(Bones.size());

		// Update Tree
		const int32 NumBones = (int32)Bones.size();
		for (int32 b = 0; b < NumBones; b++)
		{
			const TBoneInfo& Bone = Bones[b];

			if (Bone.ParentIndex < 0)
			{
				// Root
				MakeMatrix(GlobalPoses[b], Bone.Pos, Bone.Rot, Bone.Scale);
			}
			else
			{
				FMat4 Mat;
				MakeMatrix(Mat, Bone.Pos, Bone.Rot, Bone.Scale);

				GlobalPoses[b] = GlobalPoses[Bone.ParentIndex] * Mat;
			}
		}

		// Multi with InvBindPose
		for (int32 b = 0; b < NumBones; b++)
		{
			GlobalPoses[b] = GlobalPoses[b] * InvBindMatrix[b];
		}

	}

	void TSkeleton::GatherBoneData(TVector<float>& BoneData, const TVector<uint32>& BoneMap)
	{
		// Gather data to 4x3 matrices
		BoneData.resize(MaxBones * 4 * 3);
		for (int32 i = 0; i < (int32)BoneMap.size(); ++i)
		{
			const int32 Index = BoneMap[i];
			const FMat4& Mat = GlobalPoses[Index];
			BoneData[i * 4 * 3 + 0] = Mat[0];
			BoneData[i * 4 * 3 + 1] = Mat[4];
			BoneData[i * 4 * 3 + 2] = Mat[8];
			BoneData[i * 4 * 3 + 3] = Mat[12];

			BoneData[i * 4 * 3 + 4] = Mat[1];
			BoneData[i * 4 * 3 + 5] = Mat[5];
			BoneData[i * 4 * 3 + 6] = Mat[9];
			BoneData[i * 4 * 3 + 7] = Mat[13];

			BoneData[i * 4 * 3 + 8] = Mat[2];
			BoneData[i * 4 * 3 + 9] = Mat[6];
			BoneData[i * 4 * 3 + 10] = Mat[10];
			BoneData[i * 4 * 3 + 11] = Mat[14];
		}
	}
}