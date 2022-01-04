/*
	TiX Engine v2.0 Copyright (C) 2018~2021
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	enum E_LOG_LEVEL
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

		TI_API static void DoLog(E_LOG_LEVEL LogLevel, const char* Format, ...);

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