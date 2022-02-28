/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	class TTimer
	{
	public:
		TI_API static uint64 GetCurrentTimeMillis();
		TI_API static int32 GetCurrentTimeSeconds();
		TI_API static int32 GetCurrentTimeSeconds(int32& h, int32& m, int32& s);
		TI_API static int32 GetCurrentDate();
		TI_API static int32 GetCurrentDate(int32& y, int32& m, int32& d);
		TI_API static void GetYMDFromDate(int32 date, int32& y, int32& m, int32& d);
		TI_API static void GetCurrentDateAndSeconds(int32& d, int32& s);
		TI_API static void GetCurrentWeekDay(int32& d);

		TI_API static uint64 QueryCPUFrequency();
		TI_API static uint64 QueryCPUCounter();

		TI_API static bool IsLeapYear(int32 year);
		// m from 1 - 12; d from 1 - 31
		TI_API static int32 DayInYear(int32 y, int32 m, int32 d);
		TI_API static int32 DayBetweenDates(int32 y1, int32 m1, int32 d1, int32 y2, int32 m2, int32 d2);
	};

	/////////////////////////////////////////////////////////////

	class TI_API TTimeRecorder
	{
	public:
		TTimeRecorder(bool UseHighPrecision = false);
		TTimeRecorder(const TString& InName, bool UseHighPrecision = false);
		~TTimeRecorder();

		void LogTimeUsed();
	private:
		void Start();
		void End();

	private:
		TString Name;
		bool High;
		uint64 StartTime;
		uint64 EndTime;
		uint64 Freq;
	};

#ifdef TIX_SHIPPING
#	define TIMER_RECORDER(name)
#	define TIMER_RECORDER_FUNC()
#	define TIMER_RECORDER_HIGH(name)
#	define TIMER_RECORDER_HIGH_FUNC()
#else
#	define TIMER_RECORDER(name) TTimeRecorder Rec(name)
#	define TIMER_RECORDER_FUNC() TTimeRecorder Rec(__FUNCTION__)
#	define TIMER_RECORDER_HIGH(name) TTimeRecorder Rec(name, true)
#	define TIMER_RECORDER_HIGH_FUNC() TTimeRecorder Rec(__FUNCTION__, true)
#endif // TIX_SHIPPING

}