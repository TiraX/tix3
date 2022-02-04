/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once
#include "TCooker.h"
#include "TImage.h"

namespace tix
{
	struct TResTextureDefine
	{
		TString Name;
		TString Path;
		int32 LodBias;

		TTextureDesc Desc;
		TVector<TImage*> ImageSurfaces;

		// Extra Info
		int32 TGASourcePixelDepth;

		TResTextureDefine()
			: LodBias(0)
			, TGASourcePixelDepth(0)
		{}

		~TResTextureDefine()
		{
			for (auto I : ImageSurfaces)
			{
				ti_delete I;
			}
			ImageSurfaces.clear();
		}
	};

	struct TResTextureSourceInfo
	{
		TString TextureSource;
		int32 LodBias;
		E_PIXEL_FORMAT TargetFormat;
		E_TEXTURE_ADDRESS_MODE AddressMode;
		bool SRGB;
		bool IsNormalmap;
		bool IsIBL;
		bool HasMips;

		TResTextureSourceInfo()
			: LodBias(0)
			, TargetFormat(EPF_RGBA8)
			, AddressMode(ETC_REPEAT)
			, SRGB(false)
			, IsNormalmap(false)
			, IsIBL(false)
			, HasMips(false)
		{}
	};

	class TImage;
	class TCookerTexture : public TCooker
	{
	public:
		TCookerTexture();
		virtual ~TCookerTexture();

		virtual EChunkLib GetCookerType() const override
		{
			return EChunkLib::Texture;
		};
		virtual bool Load(const TJSON& JsonDoc) override;
		virtual void SaveTrunk(TChunkFile& OutChunkFile) override;

		static SColorf SampleLongLatPoint(TImage* LongLat, const FFloat3& Dir, int32 Mip);
		static SColorf SampleLongLatLinear(TImage* LongLat, const FFloat3& Dir, int32 Mip);
		static SColorf SampleLongLatLinearMip(TImage* LongLat, const FFloat3& Dir, float Mip);
	private:
		static TResTextureDefine* LoadDdsFile(const TResTextureSourceInfo& SrcInfo);
		static TResTextureDefine* LoadTgaFile(const TResTextureSourceInfo& SrcInfo);
		static TResTextureDefine* LoadHdrFile(const TResTextureSourceInfo& SrcInfo);

		static TResTextureDefine* ConvertToDds(TResTextureDefine* SrcImage);
		static TResTextureDefine* ConvertToAstc(TResTextureDefine* SrcImage);
		static TResTextureDefine* Convert32FTo16F(TResTextureDefine* SrcImage);

		static TResTextureDefine* LongLatToCubeAndFilter(TResTextureDefine* SrcImage);
		static void ComputeDiffuseIrradiance(TResTextureDefine* SrcImage, FSHVectorRGB3& OutIrrEnvMap);

		static TVector<TImage*> LongLatToCube(TImage* LongLat, bool WithMips);
		static FFloat3 TransformSideToWorldSpace(uint32 CubemapFace, const FFloat3& InDirection);
		static FFloat3 TransformWorldToSideSpace(uint32 CubemapFace, const FFloat3& InDirection);
		static FFloat3 ComputeSSCubeDirectionAtTexelCenter(uint32 x, uint32 y, float InvSideExtent);
		static FFloat3 ComputeWSCubeDirectionAtTexelCenter(uint32 CubemapFace, uint32 x, uint32 y, float InvSideExtent);

		// For Debug
		static void ExportCubeMap(const TVector<TImage*>& FaceImages, const int8* NamePrefix);

		void AddTexture(TResTextureDefine* Texture);

	private:
		TVector<TResTextureDefine*> Textures;
	};

	TImage* DecodeDXT(TResTextureDefine* Texture);
}
