/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	struct TBoneInfo
	{
		int32 ParentIndex;
		FFloat3 Pos;
		FQuat Rot;
		FFloat3 Scale;
	};

	// TSkeleton, hold skeleton info for skeletal mesh
	class TI_API TSkeleton : public TResource
	{
	public:
		static const int32 MaxBones = 128;

		TSkeleton();
		TSkeleton(int32 NumBones);
		~TSkeleton();

		void AddBone(int32 ParentIndex, const FFloat3& InvTrans, const FQuat& InvRot, const FFloat3& InvScale);
		void AddBone(const TBoneInfo& Bone);
		void ComputeInvBindMatrices();

		void SetBonePos(int32 BoneIndex, const FFloat3& InPos);
		void SetBoneRot(int32 BoneIndex, const FQuat& InRot);
		void SetBoneScale(int32 BoneIndex, const FFloat3& InScale);

		void BuildGlobalPoses();
		void GatherBoneData(TVector<float>& BoneData, const TVector<uint32>& BoneMap);

		virtual void InitRenderThreadResource() override;
		virtual void DestroyRenderThreadResource() override;

		int32 GetBones() const
		{
			return (int32)Bones.size();
		}
	protected:

	public:
		//FUniformBufferPtr SkeletonResource;

	protected:
		TVector<TBoneInfo> Bones;
		TVector<matrix4> InvBindMatrix;
		TVector<matrix4> GlobalPoses;
	};
}
