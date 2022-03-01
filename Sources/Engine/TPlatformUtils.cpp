/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "TPlatformUtils.h"
#if defined (TI_PLATFORM_IOS)
#include <mach-o/dyld.h>
#include <unistd.h>
#include <sys/stat.h>
#endif

namespace tix
{
	TString TPlatformUtils::GetExecutablePath(const TString& RelativePathStart)
	{
		TString Ret;
		int8 Path[512];
#if defined (TI_PLATFORM_WIN32)
		::GetModuleFileName(NULL, Path, 512);
		Ret = Path;
		TStringReplace(Ret, "\\", "/");
#elif defined (TI_PLATFORM_IOS)
		uint32 BufferSize = 512;
		_NSGetExecutablePath(Path, &BufferSize);
		TI_ASSERT(BufferSize <= 512);
		Ret = Path;
		if (Ret.find("/Binaries") == TString::npos && Ret.find("tix3/") == TString::npos)
		{
			Ret = getcwd(NULL, 0);
		}
#else
#	error("Not supported platform")
#endif

		Ret = Ret.substr(0, Ret.rfind('/'));

		// if dir is not in Binaries/ then find from root "tix3"
		if (Ret.find("/Binary") == TString::npos)
		{
			TString::size_type root_pos = Ret.find(RelativePathStart);
			TI_ASSERT(root_pos != TString::npos);
			Ret = Ret.substr(0, root_pos + 5) + "Binaries/";
#if defined (TI_PLATFORM_WIN32)
			Ret += "Windows";
#elif defined (TI_PLATFORM_IOS)
			Ret += "Mac";
#else
#	error("Not supported platform")
#endif
		}

		return Ret;
	}

	int32 TPlatformUtils::DeleteTempFile(TString FileName)
	{
		TString CommandLine;
#if defined (TI_PLATFORM_WIN32)
		CommandLine = "del ";
		TStringReplace(FileName, "/", "\\");
#elif defined (TI_PLATFORM_IOS)
		CommandLine = "rm ";
#else
#	error("Not supported platform")
#endif
		CommandLine += FileName;
		return system(CommandLine.c_str());
	}

	int32 TPlatformUtils::OverwriteFile(TString SrcName, TString DstName)
	{
		TString CommandLine;
#if defined (TI_PLATFORM_WIN32)
		CommandLine = "copy /Y ";
		TStringReplace(SrcName, "/", "\\");
		TStringReplace(DstName, "/", "\\");
#elif defined (TI_PLATFORM_IOS)
		CommandLine = "cp ";
#else
#	error("Not supported platform")
#endif
		CommandLine += SrcName + " " + DstName;
		return system(CommandLine.c_str());
	}
	
	bool IsDirectoryExists(const TString& Path)
	{
#if defined (TI_PLATFORM_WIN32)
		DWORD dwAttrib = GetFileAttributes(Path.c_str());

		return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
			(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
#elif defined (TI_PLATFORM_IOS)
        struct stat sb;
        
        if (stat(Path.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode))
        {
            return true;
        }
        return false;
#else
#	error("Not supported platform")
#endif
	}

	void MakeDirectory(const TString& Dir)
	{
#if defined (TI_PLATFORM_WIN32)
		if (!CreateDirectory(Dir.c_str(), nullptr))
		{
			_LOG(ELog::Error, "Failed to create directory : %s.\n", Dir.c_str());
		}
#elif defined (TI_PLATFORM_IOS)
        if (mkdir(Dir.c_str(), 0755) != 0)
        {
            _LOG(ELog::Error, "Failed to create directory : %s.\n", Dir.c_str());
        }
#else
#	error("Not supported platform")
#endif
	}

	void TryMakeDirectory(const TString& InTargetPath)
	{
		TString TargetPath = InTargetPath;
		TVector<TString> Dirs;
		TString::size_type SplitIndex;
		SplitIndex = TargetPath.find('/');
		while (SplitIndex != TString::npos)
		{
			TString Dir = TargetPath.substr(0, SplitIndex);
			Dirs.push_back(Dir);
			TargetPath = TargetPath.substr(SplitIndex + 1);
			SplitIndex = TargetPath.find('/');
		}
		if (!TargetPath.empty())
		{
			Dirs.push_back(TargetPath);
		}

		TString TargetDir = "";
		for (int32 Dir = 0; Dir < (int32)Dirs.size(); ++Dir)
		{
			TargetDir += Dirs[Dir] + "/";
			if (!IsDirectoryExists(TargetDir))
			{
				MakeDirectory(TargetDir);
			}
		}
	}

	bool TPlatformUtils::CreateDirectoryIfNotExist(const TString& Path)
	{
		// Directory Exists? 
		if (!IsDirectoryExists(Path))
		{
			TryMakeDirectory(Path);
		}

		if (!IsDirectoryExists(Path)) 
		{
			return false;
		}
		return true;
	}

	int32 TPlatformUtils::GetProcessorCount()
	{
#if defined (TI_PLATFORM_WIN32)
		// Figure out how many cores there are on this machine
		SYSTEM_INFO sysinfo;
		GetSystemInfo(&sysinfo);
		return sysinfo.dwNumberOfProcessors;
#elif defined (TI_PLATFORM_IOS)
        NSUInteger a = [[NSProcessInfo processInfo] processorCount];
        return (int32)a;
#else
#	error("Not supported platform")
#endif
	}
}
