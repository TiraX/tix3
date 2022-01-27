/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "ResHelper.h"
#include "TCookerSkeleton.h"

using namespace rapidjson;

namespace tix
{
	TCookerSkeleton::TCookerSkeleton()
		: TotalBones(0)
	{
	}

	TCookerSkeleton::~TCookerSkeleton()
	{
	}

	bool TCookerSkeleton::Load(const TJSON& Doc)
	{
		// Bones
		Doc["total_bones"] << TotalBones;

		TJSONNode JBones = Doc["bones"];
		TI_ASSERT(JBones.IsArray() && JBones.Size() == TotalBones);

		InitBones.resize(TotalBones);
		for (int32 b = 0; b < TotalBones; b++)
		{
			TJSONNode JBone = JBones[b];
			ResBoneInfo& Info = InitBones[b];
			JBone["parent_index"] << Info.ParentIndex;
			JBone["translation"] << Info.InitPos;
			JBone["rotation"] << Info.InitRot;
			JBone["scale"] << Info.InitScale;
		}

		CalcInvBindTransform();

		return true;
	}

	FMat4 MakeMatrix(const FFloat3& Trans, const FQuat& Rot, const FFloat3& Scale)
	{
		FMat4 Result;
		Rot.GetMatrix(Result);
		Result.PostScale(Scale);
		Result.SetTranslation(Trans);

		return Result;
	}

	void TCookerSkeleton::CalcInvBindTransform()
	{
		// Calc iinv bind matrix in game after loading
		//TVector<FMat4> BindMatrix;
		//BindMatrix.resize(InitBones.size());

		//for (int32 b = 0; b < TotalBones; b++)
		//{
		//	const ResBoneInfo& Bone = InitBones[b];
		//	if (Bone.ParentIndex == -1)
		//	{
		//		BindMatrix[b] = MakeMatrix(Bone.InitPos, Bone.InitRot, Bone.InitScale);
		//	}
		//	else
		//	{
		//		FMat4 ParentMat = BindMatrix[Bone.ParentIndex];
		//		FMat4 Mat = MakeMatrix(Bone.InitPos, Bone.InitRot, Bone.InitScale);
		//		BindMatrix[b] = ParentMat * Mat;
		//	}
		//}

		//FMat4 testmat;
		//testmat.SetTranslation(FFloat3(1, 2, 3));
		//FMat4 invmat;
		//testmat.getInverse(invmat);

		//ConvertedBones.resize(InitBones.size());
		//for (int32 b = 0; b < TotalBones; b++)
		//{
		//	FMat4 InvMat;
		//	BindMatrix[b].getInverse(InvMat);

		//	TBoneInitInfo& Bone = ConvertedBones[b];
		//	Bone.ParentIndex = InitBones[b].ParentIndex;
		//	Bone.InvPos = InvMat.getTranslation();
		//	Bone.InvRot = InvMat;
		//	Bone.InvScale = InvMat.getScale();

		//	FFloat3 NewScale = FFloat3(1.f / Bone.InvScale.X, 1.f / Bone.InvScale.Y, 1.f / Bone.InvScale.Z);
		//	InvMat.PostScale(NewScale);
		//	Bone.InvRot = InvMat;
		//}
	}
	
	void TCookerSkeleton::SaveTrunk(TChunkFile& OutChunkFile)
	{
		TStream& OutStream = OutChunkFile.GetChunk(GetCookerType());
		TVector<TString>& OutStrings = OutChunkFile.Strings;

		TResfileChunkHeader ChunkHeader;
		ChunkHeader.ID = TIRES_ID_CHUNK_SKELETON;
		ChunkHeader.Version = TIRES_VERSION_CHUNK_SKELETON;
		ChunkHeader.ElementCount = 1;

		TStream HeaderStream, DataStream(1024 * 8);
		for (int32 t = 0; t < ChunkHeader.ElementCount; ++t)
		{
			THeaderSkeleton Define;
			Define.NumBones = TotalBones;

			// Save header
			HeaderStream.Put(&Define, sizeof(THeaderSkeleton));

			// Write bones
			DataStream.Put(InitBones.data(), (uint32)(InitBones.size() * sizeof(TBoneInfo)));
		}

		ChunkHeader.ChunkSize = HeaderStream.GetLength() + DataStream.GetLength();

		OutStream.Put(&ChunkHeader, sizeof(TResfileChunkHeader));
		OutStream.FillZero4();
		OutStream.Put(HeaderStream.GetBuffer(), HeaderStream.GetLength());
		OutStream.Put(DataStream.GetBuffer(), DataStream.GetLength());
	}
}
