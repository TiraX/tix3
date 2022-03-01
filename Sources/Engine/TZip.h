/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	/**
	* Compress and decompress zip format
	**/
	class TZip
	{
	public:
		enum class CompressQuality : uint8
		{
			Store,
			Default,
			BestSpeed,
			BestCompression
		};

		// Compress data into TZip format
		// TZip is a zip data with a 8 bytes header: TIRES_ID_ZIPFILE and SrcLength
		static TStreamPtr CompressTZip(const uint8* SrcData, int32 SrcLength, CompressQuality Quality = CompressQuality::Default);

		// Decompress data from TZip format
		// TZip is a zip data with a 8 bytes header: TIRES_ID_ZIPFILE and SrcLength
		static TStreamPtr DecompressTZip(const uint8* SrcData, int32 SrcLength);
	};
}