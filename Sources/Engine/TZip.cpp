/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "TZip.h"
#include "zlib.h"

#ifdef TI_PLATFORM_WIN32
#pragma comment (lib, "zlib")
#else
#error("Unknown platform for libpng library.")
#endif

namespace tix
{
	TStreamPtr ZCompress(const uint8* SrcData, int32 SrcLength, int32 Quality)
	{
		unsigned long DecompressSize;
		DecompressSize = SrcLength + 1024;
		TStreamPtr OutData = ti_new TStream(DecompressSize);

		int32 ErrorCode = compress2((Bytef*)OutData->GetBuffer(), (uLongf*)&DecompressSize, (const Bytef*)SrcData, (uLongf)SrcLength, Quality);
		if (ErrorCode != Z_OK)
		{
			RuntimeFail();
			return nullptr;
		}

		OutData->Seek(DecompressSize);
		return OutData;
	}

	TStreamPtr ZDecompress(const uint8* SrcData, int32 SrcLength, int32 DestLength)
	{
		uLongf DecompressLength = DestLength;
		TStreamPtr OutData = ti_new TStream(DestLength);

		int32 ErrorCode = uncompress((Bytef*)OutData->GetBuffer(), (uLongf*)&DecompressLength, (const Bytef*)SrcData, (uLongf)SrcLength);
		TI_ASSERT(DestLength == DecompressLength);

		if (ErrorCode != Z_OK)
		{
			RuntimeFail();
			return nullptr;
		}

		OutData->Seek(DecompressLength);
		return OutData;
	}

	inline int32 GetZlibQuality(TZip::CompressQuality Quality)
	{
		switch (Quality)
		{
		case TZip::CompressQuality::Store:
			return Z_NO_COMPRESSION;
		case TZip::CompressQuality::Default:
			return Z_DEFAULT_COMPRESSION;
		case TZip::CompressQuality::BestSpeed:
			return Z_BEST_SPEED;
		case TZip::CompressQuality::BestCompression:
			return Z_BEST_COMPRESSION;
		default:
			RuntimeFail();
			return Z_DEFAULT_COMPRESSION;
		}
	}

	TStreamPtr TZip::CompressTZip(TStreamPtr InData)
	{
		return CompressTZip((const uint8*)InData->GetBuffer(), InData->GetBufferSize());
	}

	TStreamPtr TZip::CompressTZip(const uint8* SrcData, int32 SrcLength, CompressQuality Quality)
	{
		if (SrcData == nullptr)
			return nullptr;

		TStreamPtr Compressed = ZCompress(SrcData, SrcLength, GetZlibQuality(Quality));

		const uint32 ID = TIRES_ID_ZIPFILE;
		const uint32 DecompressLength = SrcLength;

		TStreamPtr Data = ti_new TStream(Compressed->GetLength() + sizeof(uint32) * 2);
		Data->Put(&ID, sizeof(uint32));
		Data->Put(&DecompressLength, sizeof(uint32));
		Data->Put(Compressed->GetBuffer(), Compressed->GetLength());

		return Data;
	}

	TStreamPtr TZip::DecompressTZip(TStreamPtr InData)
	{
		return DecompressTZip((const uint8*)InData->GetBuffer(), InData->GetBufferSize());
	}

	TStreamPtr TZip::DecompressTZip(const uint8* SrcData, int32 SrcLength)
	{
		if (SrcData == nullptr)
			return nullptr;

		uint32 ID = *(uint32*)(SrcData);
		if (ID != TIRES_ID_ZIPFILE)
		{
			RuntimeFail();
			return nullptr;
		}

		uint32 DecompressLength = *(uint32*)(SrcData + sizeof(uint32));
		TStreamPtr Data = ZDecompress(SrcData + sizeof(uint32) * 2, SrcLength - sizeof(uint32) * 2, DecompressLength);
		return Data;
	}
}
