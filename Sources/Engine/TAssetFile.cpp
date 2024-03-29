/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "TAssetFile.h"

namespace tix
{
	// These lines are coded in Stanford~
	// Refactor all loading processes, pre-create all game thread resources with empty ones.
	// Analysis the dependences with different resources and loading them by order.

	TAssetFile::TAssetFile()
		: Filebuffer(nullptr)
		, Header(nullptr)
		, StringOffsets(nullptr)
	{
		memset(ChunkHeader, 0, sizeof(ChunkHeader));
	}

	TAssetFile::~TAssetFile()
	{
		Destroy();
	}

	void TAssetFile::Destroy()
	{
		Filename = "";
		SAFE_DELETE_ARRAY(Filebuffer);
	}

	TFile* TAssetFile::OpenResFile(const TString& Filename)
	{
		TFile* file = ti_new TFile;
		if (!file->Open(Filename, EFA_READ))
		{
			ti_delete file;
			return nullptr;
		}
		return file;
	}

	bool TAssetFile::Load(const TString& InFilename)
	{
		if (!ReadFile(InFilename))
		{
			_LOG(ELog::Error, "Failed to read file %s.\n", InFilename.c_str());
			return false;
		}
		return ParseFile();
	}

	bool TAssetFile::ReadFile(const TString& InFilename)
	{
		TFile* File = OpenResFile(InFilename);
		if (File == nullptr)
		{
			TString Path = TPath::GetAbsolutePath(InFilename);
			File = OpenResFile(Path);
		}
		if (File == nullptr)
		{
			_LOG(ELog::Error, "Failed to LoadAsset %s.\n", InFilename.c_str());
			return false;
		}
		Filename = File->GetFileName();
		TI_ASSERT(Filebuffer == nullptr);
		Filebuffer = ti_new int8[File->GetSize()];
		File->Read(Filebuffer, File->GetSize(), File->GetSize());
		File->Close();
		ti_delete File;
		return true;
	}

	bool TAssetFile::ParseFile()
	{
		int32 pos = 0;
		Header = (TResfileHeader*)(Filebuffer + pos);
		if (Header->Version != TIRES_VERSION_MAINFILE)
		{
			RuntimeFail();
			_LOG(ELog::Error, "Wrong file version. [%s]\n", Filename.c_str());
			return false;
		}
		pos += TMath::Align4((int32)sizeof(TResfileHeader));

		LoadStringList();

		bool result = LoadChunks(Filebuffer + pos);
		if (!result)
			Destroy();
		return result;
	}

	bool TAssetFile::LoadStringList()
	{
		StringOffsets = (int32*)(Filebuffer + Header->StringOffset);
		return true;
	}

	const int8* TAssetFile::GetString(int32 str_index)
	{
		TI_ASSERT(str_index >= 0 && str_index < Header->StringCount);
		return (char*)(StringOffsets + Header->StringCount) + (str_index > 0 ? StringOffsets[str_index - 1] : 0);
	}

	bool TAssetFile::LoadChunks(const char* chunk_start)
	{
		const TResfileChunkHeader* chunkHeader;
		for (int32 c = 0 ; c < Header->ChunkCount ; ++ c)
		{
			chunkHeader = (const TResfileChunkHeader*)chunk_start;

			switch (chunkHeader->ID)
			{
			case TIRES_ID_CHUNK_MESH:
				ChunkHeader[EChunkLib::Mesh] = chunkHeader;
				TI_ASSERT(chunkHeader->Version == TIRES_VERSION_CHUNK_MESH);
				break;
			case TIRES_ID_CHUNK_TEXTURE:
				ChunkHeader[EChunkLib::Texture] = chunkHeader;
				TI_ASSERT(chunkHeader->Version == TIRES_VERSION_CHUNK_TEXTURE);
				break;
			case TIRES_ID_CHUNK_MATERIAL:
				ChunkHeader[EChunkLib::Material] = chunkHeader;
				TI_ASSERT(chunkHeader->Version == TIRES_VERSION_CHUNK_MATERIAL);
				break;
			case TIRES_ID_CHUNK_MINSTANCE:
				ChunkHeader[EChunkLib::MaterialInstance] = chunkHeader;
				TI_ASSERT(chunkHeader->Version == TIRES_VERSION_CHUNK_MINSTANCE);
				break;
			case TIRES_ID_CHUNK_SCENE:
				ChunkHeader[EChunkLib::Scene] = chunkHeader;
				TI_ASSERT(chunkHeader->Version == TIRES_VERSION_CHUNK_SCENE);
				break;
			case TIRES_ID_CHUNK_SCENETILE:
				ChunkHeader[EChunkLib::SceneTile] = chunkHeader;
				TI_ASSERT(chunkHeader->Version == TIRES_VERSION_CHUNK_SCENETILE);
				break;
			case TIRES_ID_CHUNK_SKELETON:
				ChunkHeader[EChunkLib::Skeleton] = chunkHeader;
				TI_ASSERT(chunkHeader->Version == TIRES_VERSION_CHUNK_SKELETON);
				break;
			case TIRES_ID_CHUNK_ANIMATION:
				ChunkHeader[EChunkLib::Animation] = chunkHeader;
				TI_ASSERT(chunkHeader->Version == TIRES_VERSION_CHUNK_ANIM);
				break;
			case TIRES_ID_CHUNK_RTX_PIPELINE:
				ChunkHeader[EChunkLib::RtxPipeline] = chunkHeader;
				TI_ASSERT(chunkHeader->Version == TIRES_VERSION_CHUNK_RTX_PIPELINE);
				break;
			default:
				RuntimeFail();
				break;
			}
			chunk_start += chunkHeader->ChunkSize;
		}
		return true;
	}

	void TAssetFile::CreateResource(TVector<TResourcePtr>& OutResources)
	{
		if (ChunkHeader[EChunkLib::Mesh] != nullptr)
			CreateStaticMesh(OutResources);

		if (ChunkHeader[EChunkLib::Texture] != nullptr)
			CreateTexture(OutResources);

		if (ChunkHeader[EChunkLib::Material] != nullptr)
			CreateMaterial(OutResources);

		if (ChunkHeader[EChunkLib::MaterialInstance] != nullptr)
			CreateMaterialInstance(OutResources);

		if (ChunkHeader[EChunkLib::Scene] != nullptr)
			CreateScene();

		if (ChunkHeader[EChunkLib::SceneTile] != nullptr)
			CreateSceneTile(OutResources);

		if (ChunkHeader[EChunkLib::Animation] != nullptr)
			CreateAnimSequence(OutResources);

		if (ChunkHeader[EChunkLib::Skeleton] != nullptr)
			CreateSkeleton(OutResources);

		if (ChunkHeader[EChunkLib::RtxPipeline] != nullptr)
			CreateRtxPipeline(OutResources);

		for (auto& Res : OutResources)
		{
			Res->SetResourceName(Filename);
		}
	}

	void TAssetFile::CreateStaticMesh(TVector<TResourcePtr>& OutResources)
	{
		if (ChunkHeader[EChunkLib::Mesh] == nullptr)
			return;

		const int8* ChunkStart = (const int8*)ChunkHeader[EChunkLib::Mesh];
		const int32 MeshCount = ChunkHeader[EChunkLib::Mesh]->ElementCount;
		if (MeshCount == 0)
		{
			return;
		}
		
		TI_TODO("Maybe we dont need TVector<TResourcePtr> to hold multi res. ONLY return 1 resource");
		// New mesh format should have 1 mesh buffer ONLY, and multiple mesh sections
		TI_ASSERT(MeshCount == 1);
		// Mesh sections and 1 collision
		OutResources.reserve(MeshCount + 1);

		// Load meshes
		const int8* MeshDataStart = (const int8*)(ChunkStart + TMath::Align4((int32)sizeof(TResfileChunkHeader)));
		const int8* SectionDataStart = (const int8*)(MeshDataStart + TMath::Align4((int32)sizeof(THeaderMesh)));

		const THeaderMesh* Header = (const THeaderMesh*)(MeshDataStart);
		const THeaderMeshSection * HeaderSections = (const THeaderMeshSection*)(SectionDataStart);

		TI_ASSERT(Header->Sections > 0);
		// Calc Active Bone Info
		int32 TotalActiveBones = 0;
		for (int32 s = 0; s < Header->Sections; s++)
		{
			TotalActiveBones += HeaderSections[s].ActiveBones;
		}

		const uint32* ActiveBoneData = (uint32*)(SectionDataStart + Header->Sections * sizeof(THeaderMeshSection));

		const int8* VertexDataStart = MeshDataStart + 
			TMath::Align4((int32)sizeof(THeaderMesh)) * MeshCount + 
			sizeof(THeaderMeshSection) * Header->Sections + 
			TotalActiveBones * sizeof(uint32) + 
			sizeof(THeaderCollisionSet);

		// Create mesh buffer resource
		TVertexBufferPtr VB = ti_new TVertexBuffer();
		VB->SetResourceName(Filename + "-VB");
		TIndexBufferPtr IB = ti_new TIndexBuffer();
		IB->SetResourceName(Filename + "-IB");
		TStaticMeshPtr StaticMesh = ti_new TStaticMesh(VB, IB, Header->Flags);

		// Load vertex data and index data
		const int32 IndexStride = (Header->IndexType == EIT_16BIT) ? sizeof(uint16) : sizeof(uint32);
		const int32 VertexStride = TVertexBuffer::GetStrideFromFormat(Header->VertexFormat);
		const int8* VertexData = VertexDataStart;
		const int8* IndexData = VertexDataStart + TMath::Align4(Header->VertexCount * VertexStride);
		VB->SetVertexData(Header->VertexFormat, VertexData, Header->VertexCount, Header->BBox);
		IB->SetIndexData((E_INDEX_TYPE)Header->IndexType, IndexData, Header->PrimitiveCount * 3);
		TI_ASSERT(Header->ClusterSize == 0 || (Header->PrimitiveCount == Header->Clusters * Header->ClusterSize));

		FStats::Stats.VertexDataInBytes += Header->VertexCount * VertexStride;
		FStats::Stats.IndexDataInBytes += TMath::Align4((int32)(IndexStride * Header->PrimitiveCount * 3));

		// Load sections
		int32 BoneMapOffset = 0;
		for (int32 s = 0 ; s < Header->Sections ; ++ s)
		{
			const THeaderMeshSection& HeaderSection = HeaderSections[s];

			TMeshSection MeshSection;
			MeshSection.IndexStart = HeaderSection.IndexStart;
			MeshSection.Triangles = HeaderSection.Triangles;
			MeshSection.BoneMap.resize(HeaderSection.ActiveBones);
			if (HeaderSection.ActiveBones > 0)
			{
				memcpy(MeshSection.BoneMap.data(), ActiveBoneData + BoneMapOffset, HeaderSection.ActiveBones * sizeof(uint32));
				BoneMapOffset += HeaderSection.ActiveBones;
			}

			// Load material
			TString MaterialResName = GetString(HeaderSection.StrMaterialInstance);
			TAssetPtr MIRes = TAssetLibrary::Get()->LoadAsset(MaterialResName);
			if (MIRes->GetResources().size() == 0)
			{
				_LOG(ELog::Error, "Failed to load default material instance [%s] for mesh [%s].\n", MaterialResName.c_str(), Filename.c_str());
			}
			else
			{
				TMaterialInstancePtr MaterialInstance = static_cast<TMaterialInstance*>(MIRes->GetResourcePtr());
				MeshSection.DefaultMaterial = MaterialInstance;
			}

			StaticMesh->AddMeshSection(MeshSection);
		}

		// Load collisions
		{
			const int8* CollisionHeaderStart = MeshDataStart + 
				sizeof(THeaderMesh) * MeshCount + 
				sizeof(THeaderMeshSection) * Header->Sections +
				TotalActiveBones * sizeof(uint32);
			const int8* CollisionDataStart = VertexDataStart + 
				TMath::Align4(Header->VertexCount * VertexStride)	// Vertex data size
				+ TMath::Align4((int32)(IndexStride * Header->PrimitiveCount * 3))		// Index data size
				+ TMath::Align4((int32)(Header->Clusters * sizeof(TMeshClusterDef)));	// Cluster data size
			THeaderCollisionSet * HeaderCollision = (THeaderCollisionSet*)CollisionHeaderStart;

			const int8* CollisionSphereData = CollisionDataStart;
			const int8* CollisionBoxData = CollisionSphereData + HeaderCollision->SpheresSizeInBytes;
			const int8* CollisionCapsuleData = CollisionBoxData + HeaderCollision->BoxesSizeInBytes;
			const int8* CollisionConvexCount = CollisionCapsuleData + HeaderCollision->CapsulesSizeInBytes;
			const int8* CollisionConvexData = CollisionConvexCount + sizeof(FUInt2) * HeaderCollision->NumConvexes;

			TI_ASSERT(sizeof(TCollisionSet::TSphere) * HeaderCollision->NumSpheres == HeaderCollision->SpheresSizeInBytes);
			TI_ASSERT(sizeof(TCollisionSet::TBox) * HeaderCollision->NumBoxes == HeaderCollision->BoxesSizeInBytes);
			TI_ASSERT(sizeof(TCollisionSet::TCapsule) * HeaderCollision->NumCapsules == HeaderCollision->CapsulesSizeInBytes);
			TI_ASSERT(sizeof(FUInt2) * HeaderCollision->NumConvexes == HeaderCollision->ConvexesSizeInBytes);

			TCollisionSetPtr CollisionSet = ti_new TCollisionSet;
			CollisionSet->Spheres.resize(HeaderCollision->NumSpheres);
			CollisionSet->Boxes.resize(HeaderCollision->NumBoxes);
			CollisionSet->Capsules.resize(HeaderCollision->NumCapsules);
			CollisionSet->Convexes.resize(HeaderCollision->NumConvexes);

			const TCollisionSet::TSphere * SphereData = (const TCollisionSet::TSphere*)CollisionSphereData;
			for (uint32 i = 0; i < HeaderCollision->NumSpheres; ++i)
			{
				CollisionSet->Spheres[i] = SphereData[i];
			}
			const TCollisionSet::TBox * BoxData = (const TCollisionSet::TBox*)CollisionBoxData;
			for (uint32 i = 0; i < HeaderCollision->NumBoxes; ++i)
			{
				CollisionSet->Boxes[i] = BoxData[i];
			}
			const TCollisionSet::TCapsule * CapsuleData = (const TCollisionSet::TCapsule*)CollisionCapsuleData;
			for (uint32 i = 0; i < HeaderCollision->NumCapsules; ++i)
			{
				CollisionSet->Capsules[i] = CapsuleData[i];
			}
			const FUInt2* ConvexDataCount = (const FUInt2*)CollisionConvexCount;
			uint32 ConvexDataOffset = 0;
			for (uint32 i = 0 ; i < HeaderCollision->NumConvexes ; ++i)
			{
				const uint32 VertexCount = ConvexDataCount[i].X;
				const uint32 IndexCount = ConvexDataCount[i].Y;

				CollisionSet->Convexes[i].VertexData.resize(VertexCount);
				CollisionSet->Convexes[i].IndexData.resize(IndexCount);

				memcpy(CollisionSet->Convexes[i].VertexData.data(), CollisionConvexData + ConvexDataOffset, VertexCount * sizeof(FFloat3));
				ConvexDataOffset += VertexCount * sizeof(FFloat3);
				memcpy(CollisionSet->Convexes[i].IndexData.data(), CollisionConvexData + ConvexDataOffset, IndexCount * sizeof(uint16));
				ConvexDataOffset += TMath::Align4(uint32(IndexCount * sizeof(uint16)));
			}
			StaticMesh->SetCollision(CollisionSet);
		}

		// Create occlude mesh from collision
		StaticMesh->CreateOccludeMeshFromCollision();

		OutResources.push_back(StaticMesh);
	}

	void TAssetFile::CreateTexture(TVector<TResourcePtr>& OutResources)
	{
		if (ChunkHeader[EChunkLib::Texture] == nullptr)
			return;

		const uint8* ChunkStart = (const uint8*)ChunkHeader[EChunkLib::Texture];
		const int32 TextureCount = ChunkHeader[EChunkLib::Texture]->ElementCount;
		if (TextureCount == 0)
		{
			return;
		}

		const uint8* HeaderStart = (const uint8*)(ChunkStart + TMath::Align4((int32)sizeof(TResfileChunkHeader)));
		const uint8* TextureDataStart = HeaderStart + TMath::Align4((int32)sizeof(THeaderTexture)) * TextureCount;

		// each ResFile should have only 1 resource
		OutResources.reserve(TextureCount);
		for (int32 i = 0; i < TextureCount; ++i)
		{
			const THeaderTexture* Header = (const THeaderTexture*)(HeaderStart + TMath::Align4((int32)sizeof(THeaderTexture)) * i);

			TTextureDesc Desc;
			Desc.Type = (E_TEXTURE_TYPE)Header->Type;
			Desc.Format = (E_PIXEL_FORMAT)Header->Format;
			Desc.Width = Header->Width;
			Desc.Height = Header->Height;
			Desc.AddressMode = (E_TEXTURE_ADDRESS_MODE)Header->AddressMode;
			Desc.SRGB = Header->SRGB;
			Desc.Mips = Header->Mips;
			Desc.SH = Header->SH;

			if (Desc.SRGB != 0)
			{
				Desc.Format = GetSRGBFormat(Desc.Format);
			}

			TTexturePtr Texture = ti_new TTexture(Desc);

			int32 ArraySize = 1;
			if (Desc.Type == ETT_TEXTURE_CUBE)
			{
				ArraySize = 6;
			}
			TI_ASSERT(Header->Surfaces == ArraySize * Desc.Mips);

			int32 DataOffset = 0;
			for (int32 ArrayIndex = 0; ArrayIndex < ArraySize; ++ArrayIndex)
			{
				for (uint32 MipLevel = 0; MipLevel < Texture->GetDesc().Mips; ++MipLevel)
				{
					const uint8* Data = TextureDataStart + DataOffset;
					int32 Width = *(const int32*)(Data + sizeof(int32) * 0);
					int32 Height = *(const int32*)(Data + sizeof(int32) * 1);
					int32 RowPitch = *(const int32*)(Data + sizeof(int32) * 2);
					int32 Size = *(const int32*)(Data + sizeof(int32) * 3);
					Texture->AddSurface(
						ArrayIndex, 
						MipLevel, 
						Width, 
						Height, 
						Data + sizeof(uint32) * 4, 
						RowPitch, 
						Size);
					DataOffset += Size + sizeof(uint32) * 4;
					DataOffset = TMath::Align4(DataOffset);
				}
			}

			OutResources.push_back(Texture);
		}
	}

	void TAssetFile::CreateMaterial(TVector<TResourcePtr>& OutResources)
	{
		if (ChunkHeader[EChunkLib::Material] == nullptr)
			return;

		const uint8* ChunkStart = (const uint8*)ChunkHeader[EChunkLib::Material];
		const int32 MaterialCount = ChunkHeader[EChunkLib::Material]->ElementCount;
		if (MaterialCount == 0)
		{
			return;
		}

		const uint8* HeaderStart = (const uint8*)(ChunkStart + TMath::Align4((int32)sizeof(TResfileChunkHeader)));
		//const uint8* CodeDataStart = HeaderStart + TMath::Align4((int32)sizeof(THeaderMaterial)) * MaterialCount;
		
		OutResources.reserve(MaterialCount);
		for (int32 i = 0; i < MaterialCount; ++i)
		{
			const THeaderMaterial* Header = (const THeaderMaterial*)(HeaderStart + TMath::Align4((int32)sizeof(THeaderMaterial)) * i);
			TMaterialPtr Material = ti_new TMaterial;

			// Load material
			TShaderNames ShaderNames;
			for (int32 s = 0; s < ESS_COUNT; ++s)
			{
				ShaderNames.ShaderNames[s] = GetString(Header->ShaderNames[s]);
			}

			Material->EnableState(EPSO_BLEND, (Header->Flags & EPSO_BLEND) != 0);
			Material->EnableState(EPSO_DEPTH, (Header->Flags & EPSO_DEPTH) != 0);
			Material->EnableState(EPSO_DEPTH_TEST, (Header->Flags & EPSO_DEPTH_TEST) != 0);
			Material->EnableState(EPSO_STENCIL, (Header->Flags & EPSO_STENCIL) != 0);

			Material->SetShaderVsFormat(Header->VsFormat);
			Material->SetShaderInsFormat(Header->InsFormat);
			Material->SetPrimitiveType((EPrimitiveType)Header->PrmitiveType);
			Material->SetBlendState((E_BLEND_MODE)Header->BlendMode, Header->BlendState);
			Material->SetRasterizerState(Header->RasterizerDesc);
			Material->SetDepthStencilState(Header->DepthStencilDesc);
			
			// RT info
			int RTNum = 0;
			for (int32 cb = 0; cb < ERTC_COUNT; ++cb)
			{
				if (Header->ColorBuffers[cb] == EPF_UNKNOWN)
					break;
				else
				{
					Material->SetRTColor((E_PIXEL_FORMAT)Header->ColorBuffers[cb], (E_RT_COLOR_BUFFER)cb);
					++RTNum;
				}
			}
			if (Header->DepthBuffer != EPF_UNKNOWN)
			{
				Material->SetRTDepth((E_PIXEL_FORMAT)Header->DepthBuffer);
			}
			Material->SetRTColorBufferCount(RTNum);

			// Load Shader code
			//int32 CodeOffset = 0;
			TShaderPtr Shader = ti_new TShader(ShaderNames);
			if (Header->ShaderCodeLength[0] > 0)
			{
				// Load from res file
				//Material->SetShaderCode((E_SHADER_STAGE)s, CodeDataStart + CodeOffset, Header->ShaderCodeLength[s]);
				RuntimeFail();
			}
			else
			{
				Shader->LoadShaderCode();
			}
			Material->SetShader(Shader);
			OutResources.push_back(Material);
		}
	}

	void TAssetFile::CreateMaterialInstance(TVector<TResourcePtr>& OutResources)
	{
		if (ChunkHeader[EChunkLib::MaterialInstance] == nullptr)
			return;

		const uint8* ChunkStart = (const uint8*)ChunkHeader[EChunkLib::MaterialInstance];
		const int32 MICount = ChunkHeader[EChunkLib::MaterialInstance]->ElementCount;
		if (MICount == 0)
		{
			return;
		}

		const uint8* HeaderStart = (const uint8*)(ChunkStart + TMath::Align4((int32)sizeof(TResfileChunkHeader)));
		const uint8* MIDataStart = HeaderStart + TMath::Align4((int32)sizeof(THeaderMaterialInstance)) * MICount;

		OutResources.reserve(MICount);
		// each ResFile should have only 1 resource
		for (int32 i = 0; i < MICount; ++i)
		{
			const THeaderMaterialInstance* Header = (const THeaderMaterialInstance*)(HeaderStart + TMath::Align4((int32)sizeof(THeaderMaterialInstance)) * i);
			TMaterialInstancePtr MInstance = ti_new TMaterialInstance;

			const int32 TotalParamCount = Header->ParamDataCount + Header->ParamTextureCount;
			MInstance->ParamNames.reserve(TotalParamCount);
			MInstance->ParamTypes.reserve(TotalParamCount);

			const int32* ParamNameOffset = (const int32*)(MIDataStart + 0);
			const uint8* ParamTypeOffset = (const uint8*)(MIDataStart + sizeof(int32) * TotalParamCount);
			const uint8* ParamValueOffset = (const uint8*)(MIDataStart + sizeof(int32) * TotalParamCount + TMath::Align4(TotalParamCount));

			int32 TotalValueBufferLength = 0;
			for (int32 p = 0; p < TotalParamCount; ++p)
			{
				E_MI_PARAM_TYPE ParamType = (E_MI_PARAM_TYPE)(ParamTypeOffset[p]);
				const int32 ValueBytes = TMaterialInstance::GetParamTypeBytes(ParamType);
				if (ParamType != MIPT_TEXTURE)
				{
					TotalValueBufferLength += ValueBytes;
				}
			}
			if (TotalValueBufferLength > 0)
			{
				MInstance->ParamValueBuffer = ti_new TStream(TotalValueBufferLength);
			}

			// Load param names and types
			int32 ValueOffset = 0;
			for (int32 p = 0; p < TotalParamCount; ++p)
			{
				MInstance->ParamNames.push_back(GetString(ParamNameOffset[p]));
				E_MI_PARAM_TYPE ParamType = (E_MI_PARAM_TYPE)(ParamTypeOffset[p]);
				MInstance->ParamTypes.push_back(ParamType);
				const int32 ValueBytes = TMaterialInstance::GetParamTypeBytes(ParamType);
				if (ParamType == MIPT_TEXTURE)
				{
					// texture params
					int32 TextureNameIndex = *(const int32*)(ParamValueOffset + ValueOffset);
					const int16 * TextureSize = (const int16*)(ParamValueOffset + ValueOffset + sizeof(int32));
					TString TextureName = GetString(TextureNameIndex);
					TAssetPtr TextureAsset = TAssetLibrary::Get()->LoadAsset(TextureName);
					if (TextureAsset->GetResources().size() == 0)
					{
						_LOG(ELog::Error, "Failed to load texture [%s] for Material Instance [%s].\n", TextureName.c_str(), Filename.c_str());
					}
					TTexturePtr Texture = static_cast<TTexture*>(TextureAsset->GetResourcePtr());
					MInstance->ParamTextures.push_back(Texture);
				}
				else
				{
					// value params
					MInstance->ParamValueBuffer->Put(ParamValueOffset + ValueOffset, ValueBytes);
				}
				ValueOffset += ValueBytes;
			}

			// Link material
			TString MaterialResName = GetString(Header->LinkedMaterialIndex);
			TAssetPtr Material = TAssetLibrary::Get()->LoadAsset(MaterialResName);
			if (Material->GetResources().size() == 0)
			{
				_LOG(ELog::Error, "Failed to load material [%s] for Material Instance [%s].\n", MaterialResName.c_str(), Filename.c_str());
			}
			MInstance->LinkedMaterial = static_cast<TMaterial*>(Material->GetResourcePtr());

			OutResources.push_back(MInstance);
		}
	}

	void TAssetFile::CreateScene()
	{
		if (ChunkHeader[EChunkLib::Scene] == nullptr)
		{
			_LOG(ELog::Error, "Can not find scene chunk when loading scene %s.", Filename.c_str());
			return;
		}

		const uint8* ChunkStart = (const uint8*)ChunkHeader[EChunkLib::Scene];
		TI_ASSERT(ChunkHeader[EChunkLib::Scene]->ElementCount == 1);

		const uint8* HeaderStart = (const uint8*)(ChunkStart + TMath::Align4((int32)sizeof(TResfileChunkHeader)));
		const uint8* SceneDataStart = HeaderStart + TMath::Align4((int32)sizeof(THeaderScene)) * 1;

		{
			const THeaderScene* Header = (const THeaderScene*)(HeaderStart);

			// Setup Env Node
			TNodeEnvironment* Env = TEngine::Get()->GetScene()->GetEnvironment();
			Env->SetMainLightDirection(Header->MainLightDirection);
			Env->SetMainLightColor(Header->MainLightColor);
			Env->SetMainLightIntensity(Header->MainLightIntensity);

			// Load Cameras
			if (Header->NumCameras > 0)
			{
				const THeaderCameraInfo * CamInfoData = (const THeaderCameraInfo*)(SceneDataStart);
				// Pick the first for now
				const THeaderCameraInfo& CamInfo = CamInfoData[0];
				TNodeCamera * Camera = TEngine::Get()->GetScene()->GetActiveCamera();
				Camera->SetPosition(CamInfo.Location);
				Camera->SetTarget(CamInfo.Target);
				// UE4 exported fov in horizontal direction.
				Camera->SetFOVX(TMath::DegToRad(CamInfo.FOV));
				Camera->SetAspectRatio(CamInfo.Aspect);
				Camera->SetRotator(FFloat3(TMath::DegToRad(CamInfo.Rotate.X), TMath::DegToRad(CamInfo.Rotate.Y), TMath::DegToRad(CamInfo.Rotate.Z)));
			}

			// Load assets names
			const FHInt2* AssetsTiles = (const FHInt2*)(SceneDataStart + sizeof(THeaderCameraInfo) * Header->NumCameras);

			TAssetLibrary * AssetLib = TAssetLibrary::Get();
			
			// Create a level node
			TString LevelName = GetString(Header->NameIndex);
			TNodeLevel * NodeLevel = TNodeFactory::CreateNode<TNodeLevel>(TEngine::Get()->GetScene()->GetRoot(), LevelName);

			// Load scene tiles
			TVector<TAssetPtr> Tiles;
			Tiles.reserve(Header->NumTiles);
			int8 TileFileName[128];
			for (int32 t = 0; t < Header->NumTiles; ++t)
			{
				const FHInt2& Point = AssetsTiles[t];
				sprintf(TileFileName, "t%d_%d", Point.X, Point.Y);
				TString TileName = TileFileName;
				sprintf(TileFileName, "%s/t%d_%d.tasset", LevelName.c_str(), Point.X, Point.Y);
				TString TileFilePath = TileFileName;

				TSceneTileLoadingFinishDelegate * SceneTileLoadingFinishDelegate = ti_new TSceneTileLoadingFinishDelegate(LevelName, TileName);
				AssetLib->LoadAssetAysc(TileFilePath, SceneTileLoadingFinishDelegate);
			}
		}
	}

	inline void GetInstanceRotationScaleMatrix(FMat4& OutMatrix, const FQuat& Rotation, const FFloat3& Scale)
	{
		FMat4 MatInstanceTrans;
		Rotation.GetMatrix(MatInstanceTrans);
		MatInstanceTrans.PostScale(Scale);
		OutMatrix = MatInstanceTrans;
	}

	void TAssetFile::CreateSceneTile(TVector<TResourcePtr>& OutResources)
	{
		if (ChunkHeader[EChunkLib::SceneTile] == nullptr)
		{
			_LOG(ELog::Error, "Can not find scene tile chunk when loading scene %s.", Filename.c_str());
			return;
		}

		const uint8* ChunkStart = (const uint8*)ChunkHeader[EChunkLib::SceneTile];
		TI_ASSERT(ChunkHeader[EChunkLib::SceneTile]->ElementCount == 1);

		const uint8* HeaderStart = (const uint8*)(ChunkStart + TMath::Align4((int32)sizeof(TResfileChunkHeader)));
		const uint8* SceneTileDataStart = HeaderStart + TMath::Align4((int32)sizeof(THeaderSceneTile)) * 1;

		{
			const THeaderSceneTile* Header = (const THeaderSceneTile*)(HeaderStart);
			TSceneTileResourcePtr SceneTile = ti_new TSceneTileResource;
			SceneTile->LevelName = GetString(Header->LevelNameIndex);
			SceneTile->TotalEnvLights = Header->NumEnvLights;

			SceneTile->SMInfos.NumMeshes = Header->NumStaticMeshes;
			SceneTile->SMInstances.NumInstances = Header->NumSMInstances;

			SceneTile->Position.X = Header->Position.X;
			SceneTile->Position.Y = Header->Position.Y;
			SceneTile->BBox = Header->BBox;
			// Add mesh sections info to scene tile 

			TAssetLibrary* AssetLib = TAssetLibrary::Get();
			// Load reflection captures
			const THeaderEnvLight* EnvLightData = (const THeaderEnvLight*)SceneTileDataStart;
			SceneTile->EnvLights.reserve(Header->NumEnvLights);
			SceneTile->EnvLightInfos.reserve(Header->NumEnvLights);

			for (int32 rc = 0; rc < Header->NumEnvLights; ++rc)
			{
				const THeaderEnvLight& RC = EnvLightData[rc];
				// Load cubemap
				TString Cubemap = GetString(RC.LinkedCubemapIndex);
				TAssetPtr CubemapAsset = AssetLib->LoadAssetAysc(Cubemap);
				SceneTile->EnvLights.push_back(CubemapAsset);

				// Create actor
				TString Name = GetString(RC.NameIndex);

				TSceneTileResource::TEnvLightInfo EnvLightInfo;
				EnvLightInfo.Radius = 9999.f;
				EnvLightInfo.Position = RC.Position;
				SceneTile->EnvLightInfos.push_back(EnvLightInfo);
			}

			// Load assets names
			const int32* AssetsTextures = (const int32*)(SceneTileDataStart + sizeof(THeaderEnvLight) * Header->NumEnvLights);
			const int32* AssetsMaterials = AssetsTextures + Header->NumTextures;
			const int32* AssetsMaterialInstances = AssetsMaterials + Header->NumMaterials;
			const int32* AssetsSkeletons = AssetsMaterialInstances + Header->NumMaterialInstances;
			const int32* AssetsAnims = AssetsSkeletons + Header->NumSkeletons;
			const int32* AssetsSMs = AssetsAnims + Header->NumAnims;
			const int32* SMInstanceCount = AssetsSMs + Header->NumStaticMeshes;
			const int32* AssetsSMInstances = SMInstanceCount + Header->NumStaticMeshes;
			//const int32* AssetsSKMs = AssetsSMInstances + Header->NumSMInstances * sizeof(TResSMInstance) / sizeof(int32);
			//const int32* AssetsSKMActors = AssetsSKMs + Header->NumSkeletalMeshes;

			const TResSMInstance* SMInstanceData = (const TResSMInstance*)(AssetsSMInstances);
			//const TResSKMActor* SKMActorData = (const TResSKMActor*)(AssetsSKMActors);

			// Textures
			for (int32 t = 0; t < Header->NumTextures; ++t)
			{
				TString TextureName = GetString(AssetsTextures[t]);
				AssetLib->LoadAssetAysc(TextureName);
			}
			// Materials
			for (int32 m = 0; m < Header->NumMaterials; ++m)
			{
				TString MaterialName = GetString(AssetsMaterials[m]);
				AssetLib->LoadAssetAysc(MaterialName);
			}
			// Material Instances
			for (int32 mi = 0; mi < Header->NumMaterialInstances; ++mi)
			{
				TString MIName = GetString(AssetsMaterialInstances[mi]);
				AssetLib->LoadAssetAysc(MIName);
			}
			// Skeletons
			TVector<TAssetPtr> LoadedSkeletons;
			LoadedSkeletons.reserve(Header->NumSkeletons);
			for (int32 sk = 0; sk < Header->NumSkeletons; ++sk)
			{
				TString SKName = GetString(AssetsSkeletons[sk]);
				TAssetPtr Asset = AssetLib->LoadAssetAysc(SKName);
				LoadedSkeletons.push_back(Asset);
			}
			// Anims
			TVector<TAssetPtr> LoadedAnims;
			LoadedAnims.reserve(Header->NumAnims);
			for (int32 a = 0; a < Header->NumAnims; ++a)
			{
				TString AnimName = GetString(AssetsAnims[a]);
				TAssetPtr Asset = AssetLib->LoadAssetAysc(AnimName);
				LoadedAnims.push_back(Asset);
			}

			// Load meshes, add it to scene tile node when loading finished.
			SceneTile->SMInfos.MeshAssets.reserve(Header->NumStaticMeshes);
			for (int32 m = 0; m < Header->NumStaticMeshes; ++m)
			{
				TString MeshName = GetString(AssetsSMs[m]);
				TAssetPtr MeshAsset = AssetLib->LoadAssetAysc(MeshName);
				SceneTile->SMInfos.MeshAssets.push_back(MeshAsset);
			}
			//SceneTile->SKMInfos.MeshAssets.reserve(Header->NumSkeletalMeshes);
			//for (int32 m = 0; m < Header->NumSkeletalMeshes; ++m)
			//{
			//	TString MeshName = GetString(AssetsSKMs[m]);
			//	TAssetPtr MeshAsset = AssetLib->LoadAssetAysc(MeshName);
			//	SceneTile->SKMInfos.MeshAssets.push_back(MeshAsset);
			//}

			// Static Mesh Instances
			{
				uint32 TotalSMInstances = Header->NumSMInstances;
				uint32 TotalStaticMeshes = Header->NumStaticMeshes;

				SceneTile->SMInstances.InstanceCountAndOffset.reserve(TotalStaticMeshes);
				SceneTile->SMInstances.InstanceBuffer = ti_new TInstanceBuffer;
				SceneTile->SMInstances.InstanceBuffer->SetResourceName(Filename + "-TileInstance");
				uint8* Data = ti_new uint8[TInstanceBuffer::InstanceStride * TotalSMInstances];

				int32 InstanceOffsetSrc = 0;
				int32 InstanceOffsetDst = 0;
				int32 DataOffset = 0;
				for (int32 m = 0; m < Header->NumStaticMeshes; ++m)
				{
					const int32 InstanceCount = SMInstanceCount[m];

					// Compute instance data for the 1st mesh section
					const int32 InstanceDataStart = DataOffset;
					for (int32 i = 0; i < InstanceCount; ++i)
					{
						const TResSMInstance& Instance = SMInstanceData[i + InstanceOffsetSrc];

						FMat4 RotationScaleMat;
						GetInstanceRotationScaleMatrix(RotationScaleMat, Instance.Rotation, Instance.Scale);

						FMat34 Mat34;
						Mat34.SetTranslation(Instance.Position);
						Mat34[0] = RotationScaleMat[0];
						Mat34[1] = RotationScaleMat[1];
						Mat34[2] = RotationScaleMat[2];

						Mat34[4] = RotationScaleMat[4];
						Mat34[5] = RotationScaleMat[5];
						Mat34[6] = RotationScaleMat[6];

						Mat34[8] = RotationScaleMat[8];
						Mat34[9] = RotationScaleMat[9];
						Mat34[10] = RotationScaleMat[10];

						memcpy(Data + DataOffset, Mat34.Data(), sizeof(FMat34));
						DataOffset += sizeof(FMat34);
					}
					const int32 InstanceDataLength = DataOffset - InstanceDataStart;
					// Save instance offset and count
					SceneTile->SMInstances.InstanceCountAndOffset.push_back(FInt2(InstanceCount, InstanceOffsetDst));
					InstanceOffsetSrc += InstanceCount;
					InstanceOffsetDst += InstanceCount;
				}

				SceneTile->SMInstances.InstanceBuffer->SetInstanceStreamData(TInstanceBuffer::InstanceFormat, Data, TotalSMInstances);
				TI_ASSERT(InstanceOffsetDst == TotalSMInstances);
				ti_delete[] Data;
				FStats::Stats.InstancesLoaded += TotalSMInstances;
			}

			// Skeletal Mesh Actors
			//{
			//	uint32 TotalSKMActors = Header->NumSKMActors;
			//	SceneTile->SKMActorInfos.reserve(TotalSKMActors);
			//	for (uint32 a = 0; a < TotalSKMActors; a++)
			//	{
			//		const TResSKMActor& SKMActor = SKMActorData[a];

			//		TSkeletalMeshActorInfo ActorInfo;
			//		ActorInfo.SkeletonAsset = LoadedSkeletons[SKMActor.SKIndex];
			//		ActorInfo.MeshAssetRef = SceneTile->SKMInfos.MeshAssets[SKMActor.SKMIndex];
			//		ActorInfo.AnimAsset = SKMActor.AnimIndex >= 0 ? LoadedAnims[SKMActor.AnimIndex] : nullptr;
			//		ActorInfo.Pos = SKMActor.Position;
			//		ActorInfo.Rot = SKMActor.Rotation;
			//		ActorInfo.Scale = SKMActor.Scale;
			//		SceneTile->SKMActorInfos.push_back(ActorInfo);
			//	}
			//}

			OutResources.push_back(SceneTile);
		}
	}

	void TAssetFile::CreateSkeleton(TVector<TResourcePtr>& OutResources)
	{
		if (ChunkHeader[EChunkLib::Skeleton] == nullptr)
			return;

		const int8* ChunkStart = (const int8*)ChunkHeader[EChunkLib::Skeleton];
		const int32 SkeletonCount = ChunkHeader[EChunkLib::Skeleton]->ElementCount;
		if (SkeletonCount == 0)
		{
			return;
		}

		TI_TODO("Maybe we dont need TVector<TResourcePtr> to hold multi res. ONLY return 1 resource");
		// New mesh format should have 1 mesh buffer ONLY, and multiple mesh sections
		TI_ASSERT(SkeletonCount == 1);
		// Mesh sections and 1 collision
		OutResources.reserve(SkeletonCount + 1);

		// Load skeleton
		const int8* SkeletonDataStart = (const int8*)(ChunkStart + TMath::Align4((int32)sizeof(TResfileChunkHeader)));
		const int8* BoneDataStart = (const int8*)(SkeletonDataStart + TMath::Align4((int32)sizeof(THeaderSkeleton)));

		const THeaderSkeleton* Header = (const THeaderSkeleton*)(SkeletonDataStart);
		const TBoneInfo* BoneInfos = (const TBoneInfo*)(BoneDataStart);

		TI_ASSERT(Header->NumBones > 0);

		// Create skeleton resource
		TSkeletonPtr Skeleton = ti_new TSkeleton(Header->NumBones);
		Skeleton->SetResourceName(Filename + "-SK");

		for (int32 b = 0; b < Header->NumBones; b++)
		{
			const TBoneInfo& Bone = BoneInfos[b];
			Skeleton->AddBone(Bone);
		}
		Skeleton->ComputeInvBindMatrices();
		OutResources.push_back(Skeleton);
	}
	void TAssetFile::CreateAnimSequence(TVector<TResourcePtr>& OutResources)
	{
		if (ChunkHeader[EChunkLib::Animation] == nullptr)
			return;

		const int8* ChunkStart = (const int8*)ChunkHeader[EChunkLib::Animation];
		const int32 AnimCount = ChunkHeader[EChunkLib::Animation]->ElementCount;
		if (AnimCount == 0)
		{
			return;
		}

		TI_TODO("Maybe we dont need TVector<TResourcePtr> to hold multi res. ONLY return 1 resource");
		TI_ASSERT(AnimCount == 1);
		OutResources.reserve(AnimCount + 1);

		// Load animation
		const int8* AnimDataStart = (const int8*)(ChunkStart + TMath::Align4((int32)sizeof(TResfileChunkHeader)));
		const int8* TrackDataStart = (const int8*)(AnimDataStart + TMath::Align4((int32)sizeof(THeaderAnimSequence)));

		const THeaderAnimSequence* Header = (const THeaderAnimSequence*)(AnimDataStart);
		const TTrackInfo* TrackInfos = (const TTrackInfo*)(TrackDataStart);

		TI_ASSERT(Header->NumFrames > 0 && Header->NumTracks > 0 && Header->NumData > 0);
		const float* FrameDataStart = (const float*)(TrackDataStart + TMath::Align4((int32)sizeof(TTrackInfo) * Header->NumTracks));

		// Create animation resource
		TAnimSequencePtr AnimSequence = ti_new TAnimSequence(Header->NumFrames, Header->Length, Header->RateScale, Header->NumTracks);
		AnimSequence->SetResourceName(Filename + "-AS");

		for (int32 t = 0; t < Header->NumTracks; t++)
		{
			const TTrackInfo& Track = TrackInfos[t];
			AnimSequence->AddTrack(Track);
		}
		AnimSequence->SetFrameData(Header->NumData, FrameDataStart);

		OutResources.push_back(AnimSequence);
	}
	void TAssetFile::CreateRtxPipeline(TVector<TResourcePtr>& OutResources)
	{
		if (ChunkHeader[EChunkLib::RtxPipeline] == nullptr)
			return;

		const int8* ChunkStart = (const int8*)ChunkHeader[EChunkLib::RtxPipeline];
		const int32 PipelineCount = ChunkHeader[EChunkLib::RtxPipeline]->ElementCount;
		if (PipelineCount == 0)
		{
			return;
		}

		TI_TODO("Maybe we dont need TVector<TResourcePtr> to hold multi res. ONLY return 1 resource");
		TI_ASSERT(PipelineCount == 1);
		OutResources.reserve(PipelineCount + 1);

		// Load rtx pipeline
		const int8* DataStart = (const int8*)(ChunkStart + TMath::Align4((int32)sizeof(TResfileChunkHeader)));
		const THeaderRtxPipeline* HeaderRtxPipeline = (const THeaderRtxPipeline*)DataStart;
		const int32* ExportNameIndex = (const int32*)(DataStart + sizeof(THeaderRtxPipeline));

		// Create rtx pipeline resource
		TRtxPipelinePtr RtxPipeline = ti_new TRtxPipeline();
		RtxPipeline->SetResourceName(Filename + "-RTXPSO");

		RtxPipeline->SetHitGroupName(GetString(HeaderRtxPipeline->HitGroupName));
		if (HeaderRtxPipeline->HitGroupAnyHit >= 0)
			RtxPipeline->SetHitGroup(HITGROUP_ANY_HIT, GetString(HeaderRtxPipeline->HitGroupAnyHit));
		if (HeaderRtxPipeline->HitGroupClosestHit >= 0)
			RtxPipeline->SetHitGroup(HITGROUP_CLOSEST_HIT, GetString(HeaderRtxPipeline->HitGroupClosestHit));
		if (HeaderRtxPipeline->HitGroupIntersection >= 0)
			RtxPipeline->SetHitGroup(HITGROUP_INTERSECTION, GetString(HeaderRtxPipeline->HitGroupIntersection));
		for (int i = 0; i < HeaderRtxPipeline->NumExportNames; i++)
		{
			RtxPipeline->AddExportName(GetString(ExportNameIndex[i]));
		}
		RtxPipeline->SetMaxAttributeSizeInBytes(HeaderRtxPipeline->MaxAttributeSizeInDepth);
		RtxPipeline->SetMaxPayloadSizeInBytes(HeaderRtxPipeline->MaxPayloadSizeInBytes);
		RtxPipeline->SetMaxTraceRecursionDepth(HeaderRtxPipeline->MaxTraceRecursionDepth);

		// Load Shader code
		TString ShaderLibName = GetString(HeaderRtxPipeline->ShaderLibName);
		TShaderPtr Shader = ti_new TShader(ShaderLibName, EShaderType::ShaderLib);
		Shader->LoadShaderCode();
		RtxPipeline->SetShaderLib(Shader);

		OutResources.push_back(RtxPipeline);
	}
}
