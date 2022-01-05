// MeshConverter.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "TCooker.h"
#include "ResHelper.h"
#include "TCookerMesh.h"
#include "TCookerTexture.h"
#include "TCookerMaterial.h"
#include "TCookerMaterialInstance.h"
#include "TCookerScene.h"
#include "TCookerSceneTile.h"
#include "TCookerSkeleton.h"
#include "TCookerAnimSequence.h"
#include "TCookerRtxPipeline.h"
#include "TCookerMultiThreadTask.h"

TString FilenameSrc;
TString FilenameDst;

TCookerSettings TCookerSettings::GlobalSettings;

void ShowUsage()
{
	_LOG(Log, "TiXCooker src_filename dst_filename\n");
}

bool bShowExample = false;
void ShowExample()
{
	_LOG(Log, "{\n");
	_LOG(Log, "\t\"name\": \"M_AddSpecular\",\n");
	_LOG(Log, "\t\"type\": \"material\",\n");
	_LOG(Log, "\t\"version\": 1,\n");
	_LOG(Log, "\t\"desc\": \"\",\n");
	_LOG(Log, "\t\"shaders\": [\n");
	_LOG(Log, "\t\t\"S_AddSpecularVS\",\n");
	_LOG(Log, "\t\t\"S_AddSpecularPS\",\n");
	_LOG(Log, "\t\t\"\",\n");
	_LOG(Log, "\t\t\"\",\n");
	_LOG(Log, "\t\t\"\"\n");
	_LOG(Log, "\t],\n");
	_LOG(Log, "\t\"vs_format\": [\n");
	_LOG(Log, "\t\t\"EVSSEG_POSITION\",\n");
	_LOG(Log, "\t\t\"EVSSEG_TEXCOORD0\"\n");
	_LOG(Log, "\t],\n");
	_LOG(Log, "\t\"rt_colors\": [\n");
	_LOG(Log, "\t\t\"EPF_RGBA16F\"\n");
	_LOG(Log, "\t],\n");
	_LOG(Log, "\t\"rt_depth\": \"EPF_DEPTH24_STENCIL8\",\n");
	_LOG(Log, "\t\"blend_mode\": \"BLEND_MODE_OPAQUE\",\n");
	_LOG(Log, "\t\"depth_write\": false,\n");
	_LOG(Log, "\t\"depth_test\": false,\n");
	_LOG(Log, "\t\"two_sides\": false,\n");
	_LOG(Log, "\t\"stencil_enable\": true,\n");
	_LOG(Log, "\t\"stencil_read_mask\": 1,\n");
	_LOG(Log, "\t\"stencil_write_mask\": 1,\n");
	_LOG(Log, "\t\"front_stencil_fail\": \"ESO_KEEP\",\n");
	_LOG(Log, "\t\"front_stencil_depth_fail\": \"ESO_KEEP\",\n");
	_LOG(Log, "\t\"front_stencil_pass\": \"ESO_KEEP\",\n");
	_LOG(Log, "\t\"front_stencil_func\": \"ECF_EQUAL\",\n");
	_LOG(Log, "\t\"back_stencil_fail\": \"ESO_KEEP\",\n");
	_LOG(Log, "\t\"back_stencil_depth_fail\": \"ESO_KEEP\",\n");
	_LOG(Log, "\t\"back_stencil_pass\": \"ESO_KEEP\",\n");
	_LOG(Log, "\t\"back_stencil_func\": \"ECF_NEVER\"\n");
	_LOG(Log, "}\n");
}

bool ParseParams(int argc, TIX_COOKER_CONST int8* argv[])
{
	for (int i = 1; i < argc; ++i)
	{
		if (argv[i][0] == '-')
		{
			// optional parameters
			TString param = argv[i] + 1;
			size_t pos = param.find('=');
			TString key, value;

			if (pos != TString::npos)
			{
				key = param.substr(0, pos);
				value = param.substr(pos + 1);
			}
			else
			{
				key = param;
			}

			tolower(key);
			tolower(value);

			if (key == "example")
			{
				bShowExample = true;
			}
			else if (key == "iterate")
			{
				TCookerSettings::GlobalSettings.Iterate = true;
			}
			else if (key == "force32bitindex")
			{
				TCookerSettings::GlobalSettings.Force32BitIndex = true;
			}
			else if (key == "forcealphachannel")
			{
				TCookerSettings::GlobalSettings.ForceAlphaChannel = true;
			}
			else if (key == "ignoretexture")
			{
				TCookerSettings::GlobalSettings.IgnoreTexture = true;
			}
			else if (key == "vtinfo")
			{
				TCookerSettings::GlobalSettings.VTInfoFile = value;
			}
			else if (key == "astc_quality")
			{
				if (value == "high")
				{
					TCookerSettings::GlobalSettings.AstcQuality = TCookerSettings::Astc_Quality_High;
				}
				else if (value == "mid")
				{
					TCookerSettings::GlobalSettings.AstcQuality = TCookerSettings::Astc_Quality_Mid;
				}
				else
				{
					TCookerSettings::GlobalSettings.AstcQuality = TCookerSettings::Astc_Quality_Low;
				}
			}
			else if (key == "cluster_size")
			{
				TCookerSettings::GlobalSettings.MeshClusterSize = atoi(value.c_str());
			}
			else if (key == "cluster_verbose")
			{
				TCookerSettings::GlobalSettings.ClusterVerbose = true;
			}

			//if (key == "texture_path")
			//{
			//	_config.TexturePath = value;
			//}
		}
		else if (FilenameSrc == (""))
		{
			FilenameSrc = argv[i];
			GetPathAndName(FilenameSrc, TCookerSettings::GlobalSettings.SrcPath, TCookerSettings::GlobalSettings.SrcName);
		}
		else if (FilenameDst == (""))
		{
			FilenameDst = argv[i];
		}
	}

	if (!FilenameSrc.empty() && FilenameDst.empty())
	{
		// Save dst file to the same directory with src file.
		size_t pos = FilenameSrc.rfind('.');
		if (pos != TString::npos)
		{
			FilenameDst = FilenameSrc.substr(0, pos) + ".tasset";
		}
		else
		{
			FilenameDst = FilenameSrc + ".tasset";
		}
	}
	return true;
}

bool operator > (const SYSTEMTIME& time0, const SYSTEMTIME& time1)
{
	if (time0.wYear != time1.wYear)
		return time0.wYear > time1.wYear;
	if (time0.wMonth != time1.wMonth)
		return time0.wMonth > time1.wMonth;
	if (time0.wDay != time1.wDay)
		return time0.wDay > time1.wDay;
	if (time0.wHour != time1.wHour)
		return time0.wHour > time1.wHour;
	if (time0.wMinute != time1.wMinute)
		return time0.wMinute > time1.wMinute;
	if (time0.wSecond != time1.wSecond)
		return time0.wSecond > time1.wSecond;
	if (time0.wMilliseconds != time1.wMilliseconds)
		return time0.wMilliseconds > time1.wMilliseconds;
	return false;
}

bool IsTargetNeedUpdate(const TString& SrcFile, const TString& DstFile)
{
	bool bNeedUpdate = true;
	WIN32_FIND_DATA FindDataSrc, FindDataDst;
	FILETIME  LocalTime;
	SYSTEMTIME SysTimeSrc, SysTimeDst;

	// check src and dst file
	HANDLE hFileSrc, hFileDst;
	hFileSrc = FindFirstFile(SrcFile.c_str(), &FindDataSrc);
	hFileDst = FindFirstFile(DstFile.c_str(), &FindDataDst);
	if (hFileSrc != INVALID_HANDLE_VALUE &&
		hFileDst != INVALID_HANDLE_VALUE)
	{
		FileTimeToLocalFileTime(&(FindDataSrc.ftLastWriteTime), &LocalTime);
		FileTimeToSystemTime(&LocalTime, &SysTimeSrc);

		FileTimeToLocalFileTime(&(FindDataDst.ftLastWriteTime), &LocalTime);
		FileTimeToSystemTime(&LocalTime, &SysTimeDst);

		if (SysTimeDst > SysTimeSrc)
		{
			bNeedUpdate = false;
		}
	}
	return bNeedUpdate;
}

int32 DoCook(int32 argc, TIX_COOKER_CONST int8* argv[])
{
	if (argc < 2 || !ParseParams(argc, argv))
	{
		ShowUsage();
		return 0;
	}

	if (bShowExample)
	{
		ShowExample();
		return 0;
	}

	// Find path
	TStringReplace(FilenameDst, "\\", "/");
	if (TCookerSettings::GlobalSettings.Iterate && !IsTargetNeedUpdate(FilenameSrc, FilenameDst))
	{
		return 0;
	}

	return TCooker::Cook();
}

/////////////////////////////////////////////////////////////////////////////////

namespace tix
{
	const TString ChunkNames[static_cast<int32>(EChunkLib::Count)] =
	{
		"static_mesh",			//	ECL_MESHES,
		"texture",				//	ECL_TEXTURES,
		"material",				//	ECL_MATERIAL,
		"material_instance",	//	ECL_MATERIAL_INSTANCE,
		"scene",				//	ECL_SCENE,
		"scene_tile",			//	ECL_SCENETILE,
		"skeleton",				//	ECL_SKELETON,
		"animation",			//	ECL_ANIMATIONS,
		"rtx_pipeline",			//	ECL_RTX_PIPELINE,
	};

	int32 TCooker::Cook()
	{
		TResMTTaskExecuter::Create();

		TChunkFile ChunkFile;
		TFile TjsFile;
		if (!TjsFile.Open(FilenameSrc, EFA_READ))
		{
			_LOG(Error, "Failed to open file : %s.\n", FilenameSrc.c_str());
			return -1;
		}

		// Load tjs file to buffer
		int8* FileContent = ti_new int8[TjsFile.GetSize() + 1];
		TjsFile.Read(FileContent, TjsFile.GetSize(), TjsFile.GetSize());
		FileContent[TjsFile.GetSize()] = 0;
		TjsFile.Close();

		// Parse json file
		TJSON JsonDoc;
		JsonDoc.Parse(FileContent);

		TString AssetTypeName;
		JsonDoc["type"] << AssetTypeName;
		TCooker* Cooker = TCooker::GetCookerByName(AssetTypeName);
		if (Cooker == nullptr)
		{
			ti_delete[] FileContent;
			_LOG(Error, "Unknown asset type : %s.\n", AssetTypeName);
			return -1;
		}

		// Load asset content
		Cooker->Load(JsonDoc);
		Cooker->SaveTrunk(ChunkFile);

		ti_delete[] FileContent;
		ti_delete Cooker;

		// Find path
		TString DstPath;
		TString::size_type SlashPos = FilenameDst.rfind('/');
		if (SlashPos != TString::npos)
		{
			DstPath = FilenameDst.substr(0, SlashPos);
			TPlatformUtils::CreateDirectoryIfNotExist(DstPath);
		}

		if (!ChunkFile.SaveFile(FilenameDst))
		{
			_LOG(Error, "Failed to save resfile : %s\n", FilenameDst.c_str());
		}
		TResMTTaskExecuter::Destroy();

		return 0;
	}

	TCooker* TCooker::GetCookerByName(const TString& Name)
	{
		const int32 Count = static_cast<int32>(EChunkLib::Count);
		for (int32 i = 0; i < Count; i++)
		{
			if (Name == ChunkNames[i])
			{
				return GetCookerByType(static_cast<EChunkLib>(i));
			}
		}
		return nullptr;
	}

	TCooker* TCooker::GetCookerByType(EChunkLib ChunkType)
	{
		switch (ChunkType)
		{
		case EChunkLib::Mesh:
			return ti_new TCookerMesh;
		case EChunkLib::Texture:
			return ti_new TCookerTexture;
		case EChunkLib::Material:
			return ti_new TCookerMaterial;
		case EChunkLib::MaterialInstance:
			return ti_new TCookerMaterialInstance;
		case EChunkLib::Scene:
			return ti_new TCookerScene;
		case EChunkLib::SceneTile:
			return ti_new TCookerSceneTile;
		case EChunkLib::Skeleton:
			return ti_new TCookerSkeleton;
		case EChunkLib::Animation:
			return ti_new TCookerAnimSequence;
		case EChunkLib::RtxPipeline:
			return ti_new TCookerRtxPipeline;
		default:
			TI_ASSERT(0);	// Fail to find Cooker
			break;
		}
		return nullptr;
	}

	TCooker::TCooker()
	{
	}

	TCooker::~TCooker()
	{
	}

	TStream& TChunkFile::GetChunk(EChunkLib ChunkType)
	{
		return ChunkStreams[ChunkType];
	}

	bool TChunkFile::SaveFile(const TString& Filename)
	{
		// Header
		TResfileHeader HeaderResfile;
		HeaderResfile.ID = TIRES_ID_RESFILE;
		HeaderResfile.Version = TIRES_VERSION_MAINFILE;
		int32 Chunks = 0;
		for (int32 c = 0; c < EChunkLib::Count; ++c)
		{
			if (ChunkStreams[c].GetLength() > 0)
			{
				++Chunks;
			}
		}
		HeaderResfile.ChunkCount = Chunks;
		HeaderResfile.FileSize = TMath::Align4((int32)sizeof(TResfileHeader));
		for (int32 c = 0; c < EChunkLib::Count; ++c)
		{
			if (ChunkStreams[c].GetLength() > 0)
			{
				HeaderResfile.FileSize += ChunkStreams[c].GetLength();
			}
		}
		HeaderResfile.StringCount = (int32)Strings.size();
		HeaderResfile.StringOffset = HeaderResfile.FileSize;

		// Strings
		TStream StringStream;
		SaveStringList(Strings, StringStream);
		HeaderResfile.FileSize += StringStream.GetLength();

		TStream ChunkHeaderStream;
		ChunkHeaderStream.Put(&HeaderResfile, sizeof(TResfileHeader));
		FillZero4(ChunkHeaderStream);

		// Write to file
		TFile file;
		if (file.Open(Filename, EFA_CREATEWRITE))
		{
			// header
			file.Write(ChunkHeaderStream.GetBuffer(), ChunkHeaderStream.GetLength());

			// chunk
			for (int32 c = 0; c < EChunkLib::Count; ++c)
			{
				if (ChunkStreams[c].GetLength() > 0)
				{
					file.Write(ChunkStreams[c].GetBuffer(), ChunkStreams[c].GetLength());
				}
			}

			// strings
			file.Write(StringStream.GetBuffer(), StringStream.GetLength());
			file.Close();
			return true;
		}

		return false;
	}
};