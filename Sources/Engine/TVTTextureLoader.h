/*
	TiX Engine v2.0 Copyright (C) 2018~2021
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

#include "TAssetFileDef.h"

namespace tix
{
	class TVTTextureLoader
	{
	public:
		TVTTextureLoader(const TString& InFileName);

		TTexturePtr LoadTextureWithRegion(int32 Mip, int32 StartX, int32 StartY, int32 EndX, int32 EndY);
		static TTexturePtr LoadBakedVTPages(int32 Mip, int32 PageX, int32 PageY);

	private:
		TFile TextureFile;
		TTextureDesc TextureDesc;
		int32 DataOffset;
	};
}