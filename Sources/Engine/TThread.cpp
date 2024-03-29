/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "TThread.h"

namespace tix
{
	TThreadId TThread::GameThreadId;
	TThreadId TThread::RenderThreadId;
	TThreadId TThread::VTLoadingThreadId;

	TThread::TThread(const TString& Name)
		: IsRunning(false)
		, Thread(nullptr)
		, ThreadName(Name)
	{
	}

	TThread::~TThread()
	{
		Stop();
	}

	TThreadId TThread::AccquireId()
	{
		return std::this_thread::get_id();
	}

	void TThread::ThreadSleep(uint32 milliseconds)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
	}

	void TThread::ThreadSleepAccurate(double milliseconds)
	{
		// from https://blat-blatnik.github.io/computerBear/making-accurate-sleep-function/
		static double Estimate = 5;
		static double Mean = 5;
		static double M2 = 0;
		static uint64 Count = 1;
		while (milliseconds > Estimate)
		{
			auto TimeStart = std::chrono::high_resolution_clock::now();
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			auto TimeEnd = std::chrono::high_resolution_clock::now();

			double TimeElapsed = (TimeEnd - TimeStart).count() / 1e6 ;
			milliseconds -= TimeElapsed;

			++Count;
			double Delta = TimeElapsed - Mean;
			Mean += Delta / Count;
			M2 += Delta * (TimeElapsed - Mean);
			double StdDev = sqrt(M2 / (Count - 1));
			Estimate = Mean + StdDev;
		}

		// spin lock
		auto TimeStart = std::chrono::high_resolution_clock::now();
		while ((std::chrono::high_resolution_clock::now() - TimeStart).count() / 1e6 < milliseconds);
	}

	void TThread::IndicateGameThread()
	{
		GameThreadId = AccquireId();
	}

	void TThread::IndicateRenderThread()
	{
		RenderThreadId = AccquireId();
	}

	void TThread::IndicateVTLoadingThread()
	{
		VTLoadingThreadId = AccquireId();
	}

	bool TThread::IsGameThread()
	{
		return AccquireId() == GameThreadId;
	}

	bool TThread::IsRenderThread()
	{
		return AccquireId() == RenderThreadId;
	}

	bool TThread::IsVTLoadingThread()
	{
		return AccquireId() == VTLoadingThreadId;
	}

	void* TThread::ThreadExecute(void* param)
	{
		TThread* t = (TThread*)param;
		t->SetThreadName();
		t->SetThreadDescription();

		t->OnThreadStart();

		while (t->IsRunning)
		{
			t->Run();
		}

		t->OnThreadEnd();

		return 0;
	}

	void TThread::Start()
	{
		if (IsRunning) 
		{
			return;
		}
	
		IsRunning = true;

		TI_ASSERT(Thread == nullptr);
		Thread = ti_new TStdThread(TThread::ThreadExecute, this);
	}

	void TThread::Stop()
	{
		if (Thread != nullptr)
		{
			IsRunning = false;
			Thread->join();

			ti_delete Thread;
			Thread = nullptr;
		}
	}

	void TThread::OnThreadStart()
	{
		ThreadId = AccquireId();
#ifdef TIX_DEBUG
		_LOG(ELog::Log, "Thread %s started with priority %d.\n", ThreadName.c_str(), GetPriority());
#endif
	}

	void TThread::SetPriority(EPriority InPriority)
	{
#ifdef TI_PLATFORM_IOS
		TI_ASSERT(Thread != nullptr);
		struct sched_param param;
		param.sched_priority = InPriority;
		int32 Result = pthread_setschedparam(Thread->native_handle(), SCHED_RR, &param);
		TI_ASSERT(Result == 0);
#elif defined (TI_PLATFORM_WIN32)
		TI_ASSERT(Thread != nullptr);
		int32 WinPriority;
		switch (InPriority)
		{
		case EPriority::Highest:
			WinPriority = THREAD_PRIORITY_HIGHEST;
			break;
		case EPriority::AboveNormal:
			WinPriority = THREAD_PRIORITY_ABOVE_NORMAL;
			break;
		case EPriority::Normal:
			WinPriority = THREAD_PRIORITY_NORMAL;
			break;
		case EPriority::BelowNormal:
			WinPriority = THREAD_PRIORITY_BELOW_NORMAL;
			break;
		case EPriority::Lowest:
			WinPriority = THREAD_PRIORITY_LOWEST;
			break;
		case EPriority::Idle:
			WinPriority = THREAD_PRIORITY_IDLE;
			break;
		default:
			WinPriority = THREAD_PRIORITY_NORMAL;
			break;
		};
		bool Result = SetThreadPriority(Thread->native_handle(), WinPriority);
		TI_ASSERT(Result);
#else
#error("unknown platform.")
#endif
	}

	int32 TThread::GetPriority()
	{
#ifdef TI_PLATFORM_IOS
		TI_ASSERT(0);
		return 0;
#elif defined (TI_PLATFORM_WIN32)
		TI_ASSERT(Thread != nullptr);
		return GetThreadPriority(Thread->native_handle());
#else
#error("unknown platform.")
#endif
	}

#ifdef TI_PLATFORM_WIN32
	const DWORD MS_VC_EXCEPTION = 0x406D1388;
#pragma pack(push,8)  
	typedef struct tagTHREADNAME_INFO
	{
		DWORD dwType; // Must be 0x1000.  
		LPCSTR szName; // Pointer to name (in user addr space).  
		DWORD dwThreadID; // Thread ID (-1=caller thread).  
		DWORD dwFlags; // Reserved for future use, must be zero.  
	} THREADNAME_INFO;
#pragma pack(pop)  
#endif

	void TThread::SetThreadName()
	{
#ifdef TI_PLATFORM_WIN32
		THREADNAME_INFO info;
		info.dwType = 0x1000;
		info.szName = ThreadName.c_str();
		info.dwThreadID = GetCurrentThreadId();
		info.dwFlags = 0;
#pragma warning(push)  
#pragma warning(disable: 6320 6322)  
		__try {
			RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
		}
#pragma warning(pop)
#elif defined (TI_PLATFORM_IOS)
		NSString * NameStr = [NSString stringWithUTF8String: ThreadName.c_str()];
		[[NSThread currentThread] setName: NameStr];
#else
#error("TThread::SetThreadName() Not implement for other platform yet.")
#endif
	}

	void TThread::SetThreadDescription()
	{
#ifdef TI_PLATFORM_WIN32
		// From UE4
		// SetThreadDescription is only available from Windows 10 version 1607 / Windows Server 2016
		//
		// So in order to be compatible with older Windows versions we probe for the API at runtime
		// and call it only if available.

		typedef HRESULT(WINAPI* SetThreadDescriptionFnPtr)(HANDLE hThread, PCWSTR lpThreadDescription);

		// Use thread name as description
		TWString ThreadDescription = FromString(ThreadName);
#pragma warning( push )
#pragma warning( disable: 4191 )	// unsafe conversion from 'type of expression' to 'type required'
		static SetThreadDescriptionFnPtr RealSetThreadDescription = (SetThreadDescriptionFnPtr)GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), "SetThreadDescription");
#pragma warning( pop )

		if (RealSetThreadDescription)
		{
			RealSetThreadDescription(::GetCurrentThread(), ThreadDescription.c_str());
		}
#elif defined (TI_PLATFORM_IOS)

#else
#error("TThread::SetThreadDescription() Not implement for other platform yet.")
#endif
	}
}
