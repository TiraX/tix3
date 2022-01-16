/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	enum class ELogLevel : uint8
	{
		Log = 0,
		Warning,
		Error,
		Fatal
	};

	class TLog
	{
	public:
		TLog();
		virtual ~TLog();

		TI_API static void DoLog(ELogLevel LogLevel, const char* Format, ...);

	public:
		static int32 SilenceLog;

	private:
		TString	LogFilename;
	};
}

#ifdef TIX_SHIPPING
#	define _LOG(LogLevel, Format, ...)
#else
#	define _LOG(LogLevel, Format, ...)	TLog::DoLog(LogLevel, Format, ##__VA_ARGS__)
#endif

#define _DLOG	_LOG