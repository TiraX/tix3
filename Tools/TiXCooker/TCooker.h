/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	class TCookerSettings
	{
	public:
		enum
		{
			Astc_Quality_High,
			Astc_Quality_Mid,
			Astc_Quality_Low
		};

		static TCookerSettings Setting;
		TCookerSettings()
			: Iterate(false)
			, Force32BitIndex(false)
			, ForceAlphaChannel(false)
			, IgnoreTexture(false)
			, AstcQuality(Astc_Quality_High)
			, MeshClusterSize(0)
			, ClusterVerbose(false)
		{}

		TString SrcPath;
		TString SrcName;
		bool Iterate;
		bool Force32BitIndex;
		bool ForceAlphaChannel;
		bool IgnoreTexture;
		TString VTInfoFile;
		int32 AstcQuality;
		int32 MeshClusterSize;
		bool ClusterVerbose;
	};

	class TChunkFile
	{
	public:
		TStream& GetChunk(EChunkLib ChunkType);
		bool SaveFile(const TString& Filename);

		TVector<TString> Strings;
		TStream ChunkStreams[EChunkLib::NumChunks];
	};

	class TCooker
	{
	public:
		TCooker();
		virtual ~TCooker();

		static int32 Cook();

		virtual EChunkLib GetCookerType() const = 0;
		virtual bool Load(const TJSON& JsonDoc) = 0;
		virtual void SaveTrunk(TChunkFile& OutChunkFile) = 0;
	private:
		static TCooker* GetCookerByName(const TString& Name);
		static TCooker* GetCookerByType(EChunkLib ChunkType);

	private:
	};
};

#if defined (TI_PLATFORM_IOS)
#   define TIX_COOKER_CONST const
#else
#   define TIX_COOKER_CONST
#endif
int32 DoCook(int32 argc, TIX_COOKER_CONST int8* argv[]);
