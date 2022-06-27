/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/


#include "stdafx.h"

#include "TTimer.h"
#if defined (TI_PLATFORM_IOS)
#include <sys/time.h>
#elif defined (TI_PLATFORM_WIN32)
#include <time.h>
#pragma comment(lib, "winmm.lib")
#elif defined (TI_PLATFORM_ANDROID)
#include <time.h>
#endif

namespace tix
{
//#if defined (TI_PLATFORM_WIN32)
//#   define TI_LOCALTIME() \
//	const time_t _time = time(0); \
//	struct tm _t, *t; \
//	localtime_s(&_t, &_time); \
//	t = &_t
//#else
//#   define TI_LOCALTIME() \
//	const time_t _time = time(0); \
//	struct tm *tm; \
//	tm = localtime(&time)
//#endif
	uint64 TTimer::GetCurrentTimeMillis()
	{
#if defined (TI_PLATFORM_WIN32)
		return timeGetTime();
#elif defined (TI_PLATFORM_IOS) || defined (TI_PLATFORM_ANDROID)
		struct timeval currentTime;
		gettimeofday(&currentTime, NULL);
		long long millis = 0;
		//upscale to milliseconds
		millis += currentTime.tv_sec * 1e3;
		//downscale to milliseconds
		millis += currentTime.tv_usec / 1e3;
		return millis;
//#elif defined (TI_PLATFORM_ANDROID)
//		struct timespec now;
//		clock_gettime(CLOCK_MONOTONIC, &now);
//		return (long long) (now.tv_sec*1000000000LL + now.tv_nsec);
#else
#	error("Not supported platform")
#endif
	}

	int32 TTimer::GetCurrentTimeSeconds()
	{
		const time_t _time	= time(0);
#if defined (TI_PLATFORM_WIN32)
		struct tm t;
		localtime_s(&t, &_time);

		return	t.tm_hour * 60 * 60 + t.tm_min * 60 + t.tm_sec;
#else
		struct tm *tm;
		tm = localtime(&_time);
		
		return tm->tm_hour * 60 * 60 + tm->tm_min * 60 + tm->tm_sec;
#endif
	}

	int32 TTimer::GetCurrentTimeSeconds(int32& h, int32& m, int32& s)
	{
		const time_t _time	= time(0);
#if defined (TI_PLATFORM_WIN32)
		struct tm t;
		localtime_s(&t, &_time);

		h = t.tm_hour;
		m = t.tm_min;
		s = t.tm_sec;
#else
		struct tm *tm;
		tm = localtime(&_time);
		
		h = tm->tm_hour;
		m = tm->tm_min;
		s = tm->tm_sec;
#endif

		return	h * 60 * 60 + m * 60 + s;
	}

	int32 TTimer::GetCurrentDate()
	{
		const time_t _time	= time(0);
#if defined (TI_PLATFORM_WIN32)
		struct tm t;
		localtime_s(&t, &_time);

		TI_ASSERT(0); // Not continus
		
		return t.tm_year * 366 + t.tm_mon * 31 + t.tm_mday;
#else
		struct tm *tm;
		tm = localtime(&_time);
		
		return tm->tm_year * 366 + tm->tm_mon * 31 + tm->tm_mday;
#endif
	}

	void TTimer::GetYMDFromDate(int32 date, int32& y, int32& m, int32& d)
	{
		y		= date / 366;
		date	-= y * 366;
		m		= date / 31;
		d		= date - m * 31;
	}

	int32 TTimer::GetCurrentDate(int32& y, int32& m, int32& d)
	{
		const time_t _time = time(0);
#if defined (TI_PLATFORM_WIN32)
		struct tm t;
		localtime_s(&t, &_time);

		y = t.tm_year;
		m = t.tm_mon;
		d = t.tm_mday;
#else
		struct tm *tm;
		tm = localtime(&_time);
		
		y = tm->tm_year;
		m = tm->tm_mon;
		d = tm->tm_mday;
#endif

		return y * 366 + m * 31 + d;
	}

	void TTimer::GetCurrentWeekDay(int32& d)
	{
		const time_t _time = time(0);
#if defined (TI_PLATFORM_WIN32)
		struct tm t;
		localtime_s(&t, &_time);

		d = t.tm_wday;
#else
		struct tm *tm;
		tm = localtime(&_time);
		
		d = tm->tm_wday;
#endif
	}

	uint64 TTimer::QueryCPUFrequency()
	{
#if defined (TI_PLATFORM_WIN32)
		LARGE_INTEGER Freq;
		QueryPerformanceFrequency(&Freq);
		return Freq.QuadPart;
#else
		TI_ASSERT(0);
#endif
	}

	uint64 TTimer::QueryCPUCounter()
	{
#if defined (TI_PLATFORM_WIN32)
		LARGE_INTEGER C;
		QueryPerformanceCounter(&C);
		return C.QuadPart;
#else
		TI_ASSERT(0);
#endif
	}


	void TTimer::GetCurrentDateAndSeconds(int32& d, int32& s)
	{
		const time_t _time = time(0);
#if defined (TI_PLATFORM_WIN32)
		struct tm t;
		localtime_s(&t, &_time);

		d = t.tm_year * 366 + t.tm_mon * 31 + t.tm_mday;
		s = t.tm_hour * 60 * 60 + t.tm_min * 60 + t.tm_sec;
#else
		struct tm *tm;
		tm = localtime(&_time);
		
		d = tm->tm_year * 366 + tm->tm_mon * 31 + tm->tm_mday;
		s = tm->tm_hour * 60 * 60 + tm->tm_min * 60 + tm->tm_sec;
#endif
	}

	bool TTimer::IsLeapYear(int32 year)
	{
		return (year % 4 == 0 || year % 400 == 0) && (year % 100 != 0);
	}

	int32 TTimer::DayInYear(int32 y, int32 m, int32 d)
	{
		int32 DAY[12]	= {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
		if (IsLeapYear(y))
			DAY[1]	= 29;

		for (int32 i = 0 ; i < m - 1 ; ++ i)
		{
			d		+= DAY[i];
		}
		return d;
	}

	int32 TTimer::DayBetweenDates(int32 y1, int32 m1, int32 d1, int32 y2, int32 m2, int32 d2)
	{
		if (y1 == y2 && m1 == m2)
		{
			return TMath::Abs(d1 - d2);
		}
		else if (y1 == y2)
		{
			int32 _d1, _d2;
			_d1		= DayInYear(y1, m1, d1);
			_d2		= DayInYear(y2, m2, d2);
			return TMath::Abs(_d1 - _d2);
		}
		else
		{
			if (y1 > y2)
			{
				TMath::Swap(y1, y2);
				TMath::Swap(m1, m2);
				TMath::Swap(d1, d2);
			}
			int32 _d1, _d2, _d3;
			// get days left in last year
			if (IsLeapYear(y1))
				_d1		= 366 - DayInYear(y1, m1, d1);	
			else
				_d1		= 365 - DayInYear(y1, m1, d1);

			// get days in current year
			_d2			= DayInYear(y2, m2, d2);

			_d3			= 0;
			for (int32 y = y1 + 1 ; y < y2 ; ++ y)
			{
				if (IsLeapYear(y))
					_d3	+= 366;
				else
					_d3	+= 365;
			}
			return _d1 + _d2 + _d3;
		}
	}

	/////////////////////////////////////////////////////////////
	int32 TTimeRecorder::TimerRecorderStack = 0;
	TStringStream TTimeRecorder::Profiles;
	void TTimeRecorder::DumpProfile()
	{
		_LOG(ELog::Log, Profiles.str().c_str());
		Profiles.clear();
	}

	void TTimeRecorder::LogSpaceForStack()
	{
		static char spaces[128] = { 0 };
		if (TimerRecorderStack > 0)
		{
			memset(spaces, ' ', TimerRecorderStack * 2);
			spaces[TimerRecorderStack * 2] = 0;
			Profiles << spaces;
		}
	}
	TTimeRecorder::TTimeRecorder(bool UseHighPrecision)
		: High(UseHighPrecision)
		, StartTime(0)
		, EndTime(0)
		, Freq(1)
	{
		Start();
	}

	TTimeRecorder::TTimeRecorder(const TString& InName, bool UseHighPrecision)
		: Name(InName)
		, High(UseHighPrecision)
		, StartTime(0)
		, EndTime(0)
		, Freq(1)
	{
		Start();
	}

	TTimeRecorder::~TTimeRecorder()
	{
		LogTimeUsed();
	}

	void TTimeRecorder::Start()
	{
		LogSpaceForStack();
		Profiles << Name << " started." << endl;
		++TimerRecorderStack;
		if (High)
		{
			Freq = TTimer::QueryCPUFrequency();
			StartTime = TTimer::QueryCPUCounter();
		}
		else
			StartTime = TTimer::GetCurrentTimeMillis();
	}

	void TTimeRecorder::End()
	{
		if (High)
			EndTime = TTimer::QueryCPUCounter();
		else
			EndTime = TTimer::GetCurrentTimeMillis();
		--TimerRecorderStack;
	}

	void TTimeRecorder::LogTimeUsed()
	{
		End();
		LogSpaceForStack();

		uint64 Diff = EndTime - StartTime;
		if (High)
		{
			double Dt = double(EndTime - StartTime) / double(Freq);
			if (Dt > 0.001)
				Profiles << Name << " used : " << Dt * 1000 << " ms." << endl;
			else
				Profiles << Name << " used : " << Dt * 1000000 << " us." << endl;
		}
		else
		{
			uint32 ms = (uint32)(Diff % 1000);
			uint32 s = (uint32)((Diff / 1000) % 60);
			uint32 m = (uint32)((Diff / 1000) / 60);
			Profiles << Name << " used : " << m << ", " << s << ", " << ms << endl;
		}
	}
}
