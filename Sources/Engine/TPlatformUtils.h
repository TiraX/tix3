/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	class TPlatformUtils
	{
	public:
		TI_API static TString GetExecutablePath(const TString& RelativePathStart = "tix3/");
		TI_API static int32 DeleteTempFile(TString FileName);
		TI_API static int32 OverwriteFile(TString SrcName, TString DstName);
		TI_API static bool CreateDirectoryIfNotExist(const TString& Path);
		TI_API static int32 GetProcessorCount();
	};
}
