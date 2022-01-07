// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TiXDefines.h"
#include "TiXScene.h"

/**
 * 
 */
class FTiXExportFunctions
{
public:
	static void ExportScene(FTiXScene& Scene);

	static FString GetResourcePath(const UObject* Resource);
	static FString GetResourcePathName(const UObject* Resource);
private:
	static void TryCreateDirectory(const FString& InTargetPath);
	static bool VerifyOrCreateDirectory(FString& TargetDir);
	static void SaveJsonToFile(const FString& JsonString, const FString& Name, const FString& Path);
	static void SaveUTextureToHDR(UTexture2D* Texture, const FString& FileName, const FString& Path);
	static FString CombineResourceExportPath(const UObject* Resource, const FString& InExportPath);

	static const int32 MAX_TIX_TEXTURE_COORDS = 2;
	enum E_VERTEX_STREAM_SEGMENT
	{
		EVSSEG_POSITION = 1,
		EVSSEG_NORMAL = EVSSEG_POSITION << 1,
		EVSSEG_COLOR = EVSSEG_NORMAL << 1,
		EVSSEG_TEXCOORD0 = EVSSEG_COLOR << 1,
		EVSSEG_TEXCOORD1 = EVSSEG_TEXCOORD0 << 1,
		EVSSEG_TANGENT = EVSSEG_TEXCOORD1 << 1,
		EVSSEG_BLENDINDEX = EVSSEG_TANGENT << 1,
		EVSSEG_BLENDWEIGHT = EVSSEG_BLENDINDEX << 1,

		EVSSEG_TOTAL = EVSSEG_BLENDWEIGHT,
	};

	struct FTiXVertex
	{
		FVector Position;
		FVector4 Normal;
		FVector4 TangentX;
		FVector2D TexCoords[MAX_TIX_TEXTURE_COORDS];
		FVector4 Color;
		FVector4 BlendIndex;
		FVector4 BlendWeight;

		FTiXVertex()
			: Color(1.f, 1.f, 1.f, 1.f)
		{}

		bool operator == (const FTiXVertex& Other) const
		{
			return FMemory::Memcmp(this, &Other, sizeof(FTiXVertex)) == 0;
		}

		void AppendToArray(uint32 VsFormat, TArray<float>& Array) const
		{
			if ((VsFormat & EVSSEG_POSITION) != 0)
			{
				Array.Add(Position.X);
				Array.Add(Position.Y);
				Array.Add(Position.Z);
			}
			if ((VsFormat & EVSSEG_NORMAL) != 0)
			{
				Array.Add(Normal.X);
				Array.Add(Normal.Y);
				Array.Add(Normal.Z);
			}
			if ((VsFormat & EVSSEG_COLOR) != 0)
			{
				Array.Add(Color.X);
				Array.Add(Color.Y);
				Array.Add(Color.Z);
				Array.Add(Color.W);
			}
			if ((VsFormat & EVSSEG_TEXCOORD0) != 0)
			{
				Array.Add(TexCoords[0].X);
				Array.Add(TexCoords[0].Y);
			}
			if ((VsFormat & EVSSEG_TEXCOORD1) != 0)
			{
				Array.Add(TexCoords[1].X);
				Array.Add(TexCoords[1].Y);
			}
			if ((VsFormat & EVSSEG_TANGENT) != 0)
			{
				Array.Add(TangentX.X);
				Array.Add(TangentX.Y);
				Array.Add(TangentX.Z);
			}
			if ((VsFormat & EVSSEG_BLENDINDEX) != 0)
			{
				check(0);
			}
			if ((VsFormat & EVSSEG_BLENDWEIGHT) != 0)
			{
				check(0);
			}
		}
	};

	struct FTiXMeshSection
	{
		uint32 IndexStart;
		uint32 NumTriangles;
		FString MaterialSlotName;
		FString MIPathName;

		/** Constructor. */
		FTiXMeshSection()
			: IndexStart(0)
			, NumTriangles(0)
		{
		}

		bool operator == (const FTiXMeshSection& Other) const
		{
			return IndexStart == Other.IndexStart && NumTriangles == Other.NumTriangles;
		}
	};

	static void ExportStaticMesh(UStaticMesh* SM);
	static void ExportMaterial(UMaterial* M);
	static void ExportMaterialInstance(UMaterialInstance* MI);
	static void ExportTexture(UTexture* T);
};