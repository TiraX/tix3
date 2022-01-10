// Fill out your copyright notice in the Description page of Project Settings.


#include "TiXExportFunctions.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Serialization/BufferArchive.h"
#include "ImageUtils.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstance.h"
#include "Engine/TextureCube.h"
#include "Engine/MapBuildDataRegistry.h"
#include "Factories/TextureFactory.h"
#include "Exporters/Exporter.h"

void FTiXExportFunctions::ExportScene(FTiXScene& Scene)
{
	const FString& ExportPath = FTiXExporterSetting::Setting.ExportPath;

	// Export scene desc
	{
		FString JsonStr;
		FJsonObjectConverter::UStructToJsonObjectString(Scene.SceneDesc, JsonStr);
		SaveJsonToFile(JsonStr, Scene.SceneDesc.name, ExportPath);
	}

	// Export tiles
	{
		FString SceneTilePath = ExportPath + Scene.SceneDesc.name + TEXT("/");
		for (const auto& Iter : Scene.SceneTiles)
		{
			FTiXSceneTile* Tile = Iter.Value;
			FString JsonStr;
			FJsonObjectConverter::UStructToJsonObjectString(Tile->SceneTileDesc, JsonStr);
			FString TileName = FString::Printf(TEXT("t%d_%d"), Tile->SceneTileDesc.position[0], Tile->SceneTileDesc.position[1]);
			SaveJsonToFile(JsonStr, TileName, SceneTilePath);
		}
	}

	// Export skylight cubemap
	if (Scene.SkylightCube != nullptr)
	{
		ExportTexture(Scene.SkylightCube, ExportPath + GetResourcePath(Scene.SkylightCube), true);
	}

	// Export static meshes
	for (const auto& SM : Scene.StaticMeshes)
	{
		ExportStaticMesh(SM);
	}

	// Export materials
	for (const auto& M : Scene.Materials)
	{
		ExportMaterial(M);
	}

	// Export material instances
	for (const auto& MI : Scene.MaterialInstances)
	{
		ExportMaterialInstance(MI);
	}

	// Export textures
	for (const auto& T : Scene.Textures)
	{
		ExportTexture(T, ExportPath + GetResourcePath(T));
	}

	// Export reflection capture cubemaps
	for (const auto& RCA : Scene.RCActors)
	{
		UWorld* RCWorld = RCA->GetWorld();
		UReflectionCaptureComponent* RCComponent = RCA->GetCaptureComponent();

		FReflectionCaptureData ReadbackCaptureData;
		RCWorld->Scene->GetReflectionCaptureData(RCComponent, ReadbackCaptureData);
		if (ReadbackCaptureData.CubemapSize > 0)
		{
			UMapBuildDataRegistry* Registry = RCWorld->GetCurrentLevel()->GetOrCreateMapBuildData();
			FString TextureName = TEXT("TC_") + RCA->GetName();
			UTextureFactory* TextureFactory = NewObject<UTextureFactory>();
			TextureFactory->SuppressImportOverwriteDialog();

			TextureFactory->CompressionSettings = TC_HDR;
			UTextureCube* TextureCube = TextureFactory->CreateTextureCube(Registry, FName(TextureName), RF_Standalone | RF_Public);

			if (TextureCube)
			{
				const int32 NumMips = FMath::CeilLogTwo(ReadbackCaptureData.CubemapSize) + 1;
				TextureCube->Source.Init(
					ReadbackCaptureData.CubemapSize,
					ReadbackCaptureData.CubemapSize,
					6,
					NumMips,
					TSF_RGBA16F,
					ReadbackCaptureData.FullHDRCapturedData.GetData()
				);
				// the loader can suggest a compression setting
				TextureCube->LODGroup = TEXTUREGROUP_World;

				bool bIsCompressed = false;
				TextureCube->CompressionSettings = TC_HDR;
				TextureCube->CompressionNone = !bIsCompressed;
				TextureCube->CompressionQuality = TCQ_Highest;
				TextureCube->Filter = TF_Trilinear;
				TextureCube->SRGB = 0;

				// for now we don't support mip map generation on cubemaps
				TextureCube->MipGenSettings = TMGS_LeaveExistingMips;

				TextureCube->UpdateResource();
				TextureCube->MarkPackageDirty();

				ExportTexture(TextureCube, ExportPath + RCWorld->GetName() + TEXT("/"), true);
			}
		}
	}

	UE_LOG(LogTiXExporter, Log, TEXT("Export tix scene DONE."));
}

void FTiXExportFunctions::ExportStaticMesh(UStaticMesh* SM)
{
	// Export LOD0 only for now.
	const int32 CurrentLOD = 0;
	FStaticMeshLODResources& LODResource = SM->GetRenderData()->LODResources[CurrentLOD];

	const FStaticMeshVertexBuffer& StaticMeshVertexBuffer = LODResource.VertexBuffers.StaticMeshVertexBuffer;
	const FPositionVertexBuffer& PositionVertexBuffer = LODResource.VertexBuffers.PositionVertexBuffer;
	const FColorVertexBuffer& ColorVertexBuffer = LODResource.VertexBuffers.ColorVertexBuffer;
	const int32 TotalNumTexCoords = StaticMeshVertexBuffer.GetNumTexCoords();

	check(PositionVertexBuffer.GetNumVertices() == StaticMeshVertexBuffer.GetNumVertices());
	if (ColorVertexBuffer.GetNumVertices() > 0)
	{
		check(PositionVertexBuffer.GetNumVertices() == ColorVertexBuffer.GetNumVertices());
	}

	// Get Vertex format
	uint32 VsFormat = 0;
	int32 StrideInFloat = 0;
	if (PositionVertexBuffer.GetNumVertices() > 0)
	{
		VsFormat |= EVSSEG_POSITION;
		StrideInFloat += 3;
	}
	else
	{
		UE_LOG(LogTiXExporter, Error, TEXT("Static mesh [%s] do not have position stream."), *SM->GetPathName());
		return;
	}
	if (StaticMeshVertexBuffer.GetNumVertices() > 0)
	{
		VsFormat |= EVSSEG_NORMAL;
		VsFormat |= EVSSEG_TANGENT;
		StrideInFloat += 6;
	}
	if (ColorVertexBuffer.GetNumVertices() > 0)
	{
		VsFormat |= EVSSEG_COLOR;
		StrideInFloat += 4;
	}
	if (TotalNumTexCoords > 0)
	{
		VsFormat |= EVSSEG_TEXCOORD0;
		StrideInFloat += 2;
	}
	if (TotalNumTexCoords > 1)
	{
		VsFormat |= EVSSEG_TEXCOORD1;
		StrideInFloat += 2;
	}

	// Copy Index Data
	TArray<uint32> IndexData;
	LODResource.IndexBuffer.GetCopy(IndexData);

	// Copy Vertex Data Interleaved
	TArray<FTiXVertex> VertexData;
	VertexData.AddZeroed(PositionVertexBuffer.GetNumVertices());

	for (uint32 i = 0; i < PositionVertexBuffer.GetNumVertices(); i++)
	{
		FTiXVertex Vertex;
		Vertex.Position = PositionVertexBuffer.VertexPosition(i) * FTiXExporterSetting::Setting.MeshVertexPositionScale;
		if ((VsFormat & EVSSEG_NORMAL) != 0)
		{
			Vertex.Normal = StaticMeshVertexBuffer.VertexTangentZ(i).GetSafeNormal();
		}
		if ((VsFormat & EVSSEG_TANGENT) != 0)
		{
			Vertex.TangentX = StaticMeshVertexBuffer.VertexTangentX(i).GetSafeNormal();
		}
		if ((VsFormat & EVSSEG_TEXCOORD0) != 0)
		{
			Vertex.TexCoords[0] = StaticMeshVertexBuffer.GetVertexUV(i, 0);
		}
		if ((VsFormat & EVSSEG_TEXCOORD1) != 0)
		{
			Vertex.TexCoords[1] = StaticMeshVertexBuffer.GetVertexUV(i, 1);
		}
		if ((VsFormat & EVSSEG_COLOR) != 0)
		{
			FColor C = ColorVertexBuffer.VertexColor(i);
			const float OneOver255 = 1.f / 255.f;
			Vertex.Color.X = C.R * OneOver255;
			Vertex.Color.Y = C.G * OneOver255;
			Vertex.Color.Z = C.B * OneOver255;
			Vertex.Color.W = C.A * OneOver255;
		}
		VertexData[i] = Vertex;
	}

	// Collect Mesh Sections
	TArray<FTiXMeshSection> MeshSections;
	TArray< TSharedPtr<FJsonValue> > JsonSections;
	for (int32 Section = 0; Section < LODResource.Sections.Num(); ++Section)
	{
		FStaticMeshSection& MeshSection = LODResource.Sections[Section];

		// Remember this section
		FTiXMeshSection TiXSection;
		TiXSection.NumTriangles = MeshSection.NumTriangles;
		TiXSection.IndexStart = MeshSection.FirstIndex;
		if (FTiXExporterSetting::Setting.bIgnoreMaterial)
		{
			TiXSection.MaterialSlotName = TEXT("DebugMaterialName");
			TiXSection.MIPathName = TEXT("DebugMaterial");
		}
		else
		{
			TiXSection.MaterialSlotName = SM->GetStaticMaterials()[MeshSection.MaterialIndex].MaterialSlotName.ToString();
			UMaterialInterface* MaterialInterface = SM->GetStaticMaterials()[MeshSection.MaterialIndex].MaterialInterface;
			if (MaterialInterface != nullptr)
				TiXSection.MIPathName = GetResourcePathName(MaterialInterface) + FTiXExporterSetting::Setting.ExtName;
			else
				TiXSection.MIPathName = TEXT("MI_Default.tasset");
		}
		MeshSections.Add(TiXSection);
	}

	// Fill Structure
	FTiXStaticMesh SMDesc;
	SMDesc.name = SM->GetName();
	SMDesc.type = TEXT("static_mesh");
	SMDesc.version = 1;
	SMDesc.desc = TEXT("Static mesh (Render Resource) from TiX exporter.");
	SMDesc.vertex_count_total = VertexData.Num();
	SMDesc.index_count_total = IndexData.Num();
	SMDesc.texcoord_count = TotalNumTexCoords;
	SMDesc.total_lod = 1;	
#define ADD_VS_FORMAT(Format) if ((VsFormat & Format) != 0) SMDesc.data.vs_format.Add(TEXT(#Format))
	ADD_VS_FORMAT(EVSSEG_POSITION);
	ADD_VS_FORMAT(EVSSEG_NORMAL);
	ADD_VS_FORMAT(EVSSEG_COLOR);
	ADD_VS_FORMAT(EVSSEG_TEXCOORD0);
	ADD_VS_FORMAT(EVSSEG_TEXCOORD1);
	ADD_VS_FORMAT(EVSSEG_TANGENT);
	ADD_VS_FORMAT(EVSSEG_BLENDINDEX);
	ADD_VS_FORMAT(EVSSEG_BLENDWEIGHT);
#undef ADD_VS_FORMAT
	SMDesc.data.vertices.Reserve(StrideInFloat * VertexData.Num());
	for (const auto& V : VertexData)
	{
		V.AppendToArray(VsFormat, SMDesc.data.vertices);
	}
	SMDesc.data.indices = IndexData;
	SMDesc.sections.Reserve(MeshSections.Num());
	for (const auto& S : MeshSections)
	{
		FTiXStaticMeshSection SectionDesc;
		SectionDesc.index_start = S.IndexStart;
		SectionDesc.triangles = S.NumTriangles;
		SectionDesc.name = S.MaterialSlotName;
		SectionDesc.material_instance = S.MIPathName;
		SMDesc.sections.Add(SectionDesc);
	}

	// Save to json
	FString JsonStr;
	FJsonObjectConverter::UStructToJsonObjectString(SMDesc, JsonStr);
	SaveJsonToFile(JsonStr, SM->GetName(), FTiXExporterSetting::Setting.ExportPath + GetResourcePath(SM));
}

void FTiXExportFunctions::ExportMaterial(UMaterial* M)
{
	FTiXMaterial MaterialDesc;
	MaterialDesc.name = M->GetName();
	MaterialDesc.type = TEXT("material");
	MaterialDesc.version = 1;
	MaterialDesc.desc = TEXT("Material from TiX exporter.");

	// Shader Names
	const FString ShaderPrefix = TEXT("S_");
	FString ShaderName = M->GetName();
	if (ShaderName.Left(2) == TEXT("M_"))
		ShaderName = ShaderName.Right(ShaderName.Len() - 2);
	ShaderName = ShaderPrefix + ShaderName;
	MaterialDesc.shaders.Add(ShaderName + TEXT("VS"));
	MaterialDesc.shaders.Add(ShaderName + TEXT("PS"));
	MaterialDesc.shaders.Add(TEXT(""));
	MaterialDesc.shaders.Add(TEXT(""));
	MaterialDesc.shaders.Add(TEXT(""));

	// Fixed vertex format temp.
	MaterialDesc.vs_format.Add(TEXT("EVSSEG_POSITION"));
	MaterialDesc.vs_format.Add(TEXT("EVSSEG_NORMAL"));
	MaterialDesc.vs_format.Add(TEXT("EVSSEG_TEXCOORD0"));
	MaterialDesc.vs_format.Add(TEXT("EVSSEG_TANGENT"));

	// Fixed instance format temp.
	MaterialDesc.ins_format.Add(TEXT("EINSSEG_TRANSITION"));	// Transition
	MaterialDesc.ins_format.Add(TEXT("EINSSEG_ROT_SCALE_MAT0"));	// Rot and Scale Mat Row0
	MaterialDesc.ins_format.Add(TEXT("EINSSEG_ROT_SCALE_MAT1"));	// Rot and Scale Mat Row1
	MaterialDesc.ins_format.Add(TEXT("EINSSEG_ROT_SCALE_MAT2"));	// Rot and Scale Mat Row2

	// Fixed rt & depth format temp.
	MaterialDesc.rt_colors.Add(TEXT("EPF_RGBA16F"));
	MaterialDesc.rt_depth = TEXT("EPF_DEPTH24_STENCIL8");

	switch (M->BlendMode)
	{
	case BLEND_Opaque:
		MaterialDesc.blend_mode = TEXT("BLEND_MODE_OPAQUE");
		break;
	case BLEND_Masked:
		MaterialDesc.blend_mode = TEXT("BLEND_MODE_MASK");
		break;
	case BLEND_Translucent:
		MaterialDesc.blend_mode = TEXT("BLEND_MODE_TRANSLUCENT");
		break;
	case BLEND_Additive:
		MaterialDesc.blend_mode = TEXT("BLEND_MODE_ADDITIVE");
		break;
	case BLEND_Modulate:
	case BLEND_AlphaComposite:
		UE_LOG(LogTiXExporter, Error, TEXT("  Blend Mode Modulate/AlphaComposite NOT supported."));
		MaterialDesc.blend_mode = TEXT("BLEND_MODE_TRANSLUCENTs");
		break;
	}
	MaterialDesc.depth_write = M->BlendMode == BLEND_Opaque || M->BlendMode == BLEND_Masked;
	MaterialDesc.depth_test = true;
	MaterialDesc.two_sides = M->IsTwoSided();

	// Save to json
	FString JsonStr;
	FJsonObjectConverter::UStructToJsonObjectString(MaterialDesc, JsonStr);
	SaveJsonToFile(JsonStr, M->GetName(), FTiXExporterSetting::Setting.ExportPath + GetResourcePath(M));
}

void FTiXExportFunctions::ExportMaterialInstance(UMaterialInstance* MI)
{
	FTiXMaterialInstance MIDesc;
	MIDesc.name = MI->GetName();
	MIDesc.type = TEXT("material_instance");
	MIDesc.version = 1;
	MIDesc.desc = TEXT("Material instance from TiX exporter.");
	UMaterialInterface * ParentMaterial = MI->Parent;
	while (ParentMaterial->IsA(UMaterialInstance::StaticClass()))
	{
		ParentMaterial = Cast<UMaterialInstance>(ParentMaterial)->Parent;
	}
	check(ParentMaterial != nullptr && ParentMaterial->IsA(UMaterial::StaticClass()));
	MIDesc.linked_material = GetResourcePathName(ParentMaterial) + FTiXExporterSetting::Setting.ExtName;

	// Collect Parameters
	// Scalar parameters collect as float4
	TArray<FVector4> ScalarVectorValues;
	TArray<FString> ScalarVectorNames;
	TArray<FString> ScalarVectorComments;
	for (int32 i = 0; i < MI->ScalarParameterValues.Num(); ++i)
	{
		const FScalarParameterValue& ScalarValue = MI->ScalarParameterValues[i];

		int32 CombinedIndex = i / 4;
		int32 IndexInVector4 = i % 4;
		if (ScalarVectorValues.Num() <= CombinedIndex)
		{
			ScalarVectorValues.Add(FVector4(0, 0, 0, 0));
			FString Name = FString::Printf(TEXT("CombinedScalar%d"), CombinedIndex);
			ScalarVectorNames.Add(Name);
			ScalarVectorComments.Add(FString());
		}
		// Remember value
		ScalarVectorValues[CombinedIndex][IndexInVector4] = ScalarValue.ParameterValue;
		// Remember scalar param name
		FString ScalarParamName = FString::Printf(TEXT("%d = %s; "), IndexInVector4, *ScalarValue.ParameterInfo.Name.ToString());
		ScalarVectorComments[CombinedIndex] += ScalarParamName;
	}

	// Vector parameters.
	for (int32 i = 0; i < MI->VectorParameterValues.Num(); ++i)
	{
		const FVectorParameterValue& VectorValue = MI->VectorParameterValues[i];

		ScalarVectorValues.Add(FVector4(VectorValue.ParameterValue));
		ScalarVectorNames.Add(VectorValue.ParameterInfo.Name.ToString());
		ScalarVectorComments.Add(VectorValue.ParameterInfo.Name.ToString());
	}

	MIDesc.param_vectors.Reserve(ScalarVectorValues.Num());
	for (int32 p = 0; p < ScalarVectorValues.Num(); p++)
	{
		FTiXMIParamVector ParamDesc;
		ParamDesc.name = ScalarVectorNames[p];
		ParamDesc.type = TEXT("float4");
		ParamDesc.desc = ScalarVectorComments[p];
		ParamDesc.value = ToArray(ScalarVectorValues[p]);
		MIDesc.param_vectors.Add(ParamDesc);
	}

	MIDesc.param_textures.Reserve(MI->TextureParameterValues.Num());
	for (int32 p = 0; p < MI->TextureParameterValues.Num(); p++)
	{
		const FTextureParameterValue& TextureValue = MI->TextureParameterValues[p];
		UTexture* Texture = TextureValue.ParameterValue;
		FTiXMIParamTexture ParamDesc;
		ParamDesc.name = TextureValue.ParameterInfo.Name.ToString();
		if (Texture->IsA(UTexture2D::StaticClass()))
		{
			ParamDesc.type = TEXT("texture2d");
		}
		else if (Texture->IsA(UTextureCube::StaticClass()))
		{
			ParamDesc.type = TEXT("texturecube");
		}
		else
		{
			UE_LOG(LogTiXExporter, Error, TEXT("Unsupported texture type in material instance [%s, %s]."), *MI->GetName(), *TextureValue.ParameterInfo.Name.ToString());
		}
		ParamDesc.desc = TEXT("");
		ParamDesc.value = GetResourcePathName(Texture) + FTiXExporterSetting::Setting.ExtName;
		MIDesc.param_textures.Add(ParamDesc);
	}

	// Save to json
	FString JsonStr;
	FJsonObjectConverter::UStructToJsonObjectString(MIDesc, JsonStr);
	SaveJsonToFile(JsonStr, MI->GetName(), FTiXExporterSetting::Setting.ExportPath + GetResourcePath(MI));
}

void FTiXExportFunctions::ExportTexture(UTexture* T, const FString& ExportPath, bool UsedAsIBL)
{
	if (!T->IsA(UTexture2D::StaticClass()) && !T->IsA(UTextureCube::StaticClass()))
	{
		UE_LOG(LogTiXExporter, Error, TEXT("  Texture other than UTexture2D and UTextureCube are NOT supported yet."));
		return;
	}
	const bool IsTexture2D = T->IsA(UTexture2D::StaticClass());
	UTexture2D* InTexture2D = Cast<UTexture2D>(T);
	UTextureCube* InTextureCube = Cast<UTextureCube>(T);

	// Save texture 2d with tga format and texture cube with hdr format
	FString ImageExtName = (T->Source.GetFormat() == TSF_BGRE8 || T->Source.GetFormat() == TSF_RGBA16F) ? TEXT("hdr") : TEXT("tga");

	FBufferArchive Buffer;
	if (IsTexture2D)
	{
		UExporter::ExportToArchive(InTexture2D, nullptr, Buffer, *ImageExtName, 0);
	}
	else
	{
		UExporter::ExportToArchive(InTextureCube, nullptr, Buffer, *ImageExtName, 0);
	}

	FString ImageSourcePath = ExportPath;
	VerifyOrCreateDirectory(ImageSourcePath);
	FString ExportFullPathName = ImageSourcePath + T->GetName() + TEXT(".") + ImageExtName;
	if (Buffer.Num() == 0 || !FFileHelper::SaveArrayToFile(Buffer, *ExportFullPathName))
	{
		UE_LOG(LogTiXExporter, Error, TEXT("Fail to save texture %s"), *ExportFullPathName);
		return;
	}

	// Export a Json Desc
	FTiXTexture TextureDesc;
	TextureDesc.name = T->GetName();
	TextureDesc.type = TEXT("texture");
	TextureDesc.version = 1;
	TextureDesc.desc = TEXT("Texture from TiX exporter.");
	TextureDesc.source = T->GetName() + TEXT(".") + ImageExtName;
	TextureDesc.texture_type = IsTexture2D ? TEXT("ETT_TEXTURE_2D") : TEXT("ETT_TEXTURE_CUBE");
	TextureDesc.srgb = T->SRGB;
	TextureDesc.is_normalmap = T->LODGroup == TEXTUREGROUP_WorldNormalMap;
	TextureDesc.has_mips = T->MipGenSettings != TMGS_NoMipmaps;
	TextureDesc.ibl = UsedAsIBL;
	TextureDesc.lod_bias = T->LODBias;
	if (IsTexture2D)
	{
		TextureDesc.width = InTexture2D->GetSizeX();
		TextureDesc.height = InTexture2D->GetSizeY();
		TextureDesc.mips = InTexture2D->GetNumMips();
	}
	else
	{
		TextureDesc.width = InTextureCube->GetSizeX();
		TextureDesc.height = InTextureCube->GetSizeY();
		TextureDesc.mips = InTextureCube->GetNumMips();
	}
	if (IsTexture2D)
	{
		switch (InTexture2D->AddressX)
		{
		case TA_Wrap:
			TextureDesc.address_mode = TEXT("ETC_REPEAT");
			break;
		case TA_Clamp:
			TextureDesc.address_mode = TEXT("ETC_CLAMP_TO_EDGE");
			break;
		case TA_Mirror:
			TextureDesc.address_mode = TEXT("ETC_MIRROR");
			break;
		}

		if (!FMath::IsPowerOfTwo(InTexture2D->GetSizeX()) ||
			!FMath::IsPowerOfTwo(InTexture2D->GetSizeY()))
		{
			UE_LOG(LogTiXExporter, Warning, TEXT("%s size is not Power of Two. %d, %d."), *T->GetName(), InTexture2D->GetSizeX(), InTexture2D->GetSizeY());
		}
	}

	// Save to json
	FString JsonStr;
	FJsonObjectConverter::UStructToJsonObjectString(TextureDesc, JsonStr);
	SaveJsonToFile(JsonStr, T->GetName(), ExportPath);
}

void FTiXExportFunctions::TryCreateDirectory(const FString& InTargetPath)
{
	FString TargetPath = InTargetPath;
	TArray<FString> Dirs;
	int32 SplitIndex;
	while (TargetPath.FindChar(L'/', SplitIndex))
	{
		FString Dir = TargetPath.Left(SplitIndex);
		Dirs.Add(Dir);
		TargetPath = TargetPath.Right(TargetPath.Len() - SplitIndex - 1);
	}
	if (!TargetPath.IsEmpty())
	{
		Dirs.Add(TargetPath);
	}


	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	FString TargetDir = "";
	for (int32 Dir = 0; Dir < Dirs.Num(); ++Dir)
	{
		TargetDir += Dirs[Dir] + TEXT("/");
		if (!PlatformFile.DirectoryExists(*TargetDir))
		{
			PlatformFile.CreateDirectory(*TargetDir);
		}
	}
}

bool FTiXExportFunctions::VerifyOrCreateDirectory(FString& TargetDir)
{
	TargetDir.ReplaceInline(TEXT("\\"), TEXT("/"));
	if (!TargetDir.EndsWith(TEXT("/")))
	{
		TargetDir += TEXT("/");
	}

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	// Directory Exists? 
	if (!PlatformFile.DirectoryExists(*TargetDir))
	{
		TryCreateDirectory(TargetDir);
	}

	if (!PlatformFile.DirectoryExists(*TargetDir)) {
		return false;
	}
	return true;
}

void FTiXExportFunctions::SaveJsonToFile(const FString& JsonString, const FString& Name, const FString& Path)
{
	FString ExportPathStr = Path;
	if (VerifyOrCreateDirectory(ExportPathStr))
	{
		FString PathName = ExportPathStr + Name + TEXT(".tjs");
		FFileHelper::SaveStringToFile(JsonString, *PathName);
	}
	else
	{
		UE_LOG(LogTiXExporter, Error, TEXT("Failed to create directory : %s."), *ExportPathStr);
	}
}

void FTiXExportFunctions::SaveUTextureToHDR(UTexture2D* Texture, const FString& FileName, const FString& Path)
{
	FString ExportPathStr = Path;
	FString ExportName;
	if (!VerifyOrCreateDirectory(ExportPathStr))
	{
		UE_LOG(LogTiXExporter, Error, TEXT("Failed to create directory : %s."), *ExportPathStr);
		return;
	}

	FString TotalFileName = FPaths::Combine(*ExportPathStr, *FileName);
	FText PathError;
	FPaths::ValidatePath(TotalFileName, &PathError);

	if (Texture && !FileName.IsEmpty() && PathError.IsEmpty())
	{
		FArchive* Ar = IFileManager::Get().CreateFileWriter(*TotalFileName);

		if (Ar)
		{
			FBufferArchive Buffer;
			bool bSuccess = FImageUtils::ExportTexture2DAsHDR(Texture, Buffer);

			if (bSuccess)
			{
				Ar->Serialize(const_cast<uint8*>(Buffer.GetData()), Buffer.Num());
			}

			delete Ar;
		}
		else
		{
			UE_LOG(LogTiXExporter, Error, TEXT("SaveUTextureToPNG: FileWrite failed to create."));
		}
	}
	else if (!Texture)
	{
		UE_LOG(LogTiXExporter, Error, TEXT("SaveUTextureToPNG: TextureRenderTarget must be non-null."));
	}
	if (!PathError.IsEmpty())
	{
		UE_LOG(LogTiXExporter, Error, TEXT("SaveUTextureToPNG: Invalid file path provided: '%s'"), *PathError.ToString());
	}
	if (FileName.IsEmpty())
	{
		UE_LOG(LogTiXExporter, Error, TEXT("SaveUTextureToPNG: FileName must be non-empty."));
	}
}
FString FTiXExportFunctions::GetResourcePath(const UObject* Resource)
{
	FString SM_GamePath = Resource->GetPathName();
	SM_GamePath = SM_GamePath.Replace(TEXT("/Game/"), TEXT(""));
	int32 DotIndex;
	bool LastDot = SM_GamePath.FindLastChar('.', DotIndex);
	if (LastDot)
	{
		SM_GamePath = SM_GamePath.Mid(0, DotIndex);
	}
	int32 SlashIndex;
	bool LastSlash = SM_GamePath.FindLastChar('/', SlashIndex);
	FString Path;
	if (LastSlash)
	{
		Path = SM_GamePath.Mid(0, SlashIndex + 1);
	}
	return Path;
}

FString FTiXExportFunctions::GetResourcePathName(const UObject* Resource)
{
	return GetResourcePath(Resource) + Resource->GetName();
}

FString FTiXExportFunctions::CombineResourceExportPath(const UObject* Resource, const FString& InExportPath)
{
	FString Path = GetResourcePath(Resource);
	FString ExportPath = InExportPath;
	ExportPath.ReplaceInline(TEXT("\\"), TEXT("/"));
	if (ExportPath[ExportPath.Len() - 1] != '/')
		ExportPath.AppendChar('/');
	FString ExportFullPath = ExportPath + Path;

	FString FullPathName = Path + Resource->GetName();
	return FullPathName;
}