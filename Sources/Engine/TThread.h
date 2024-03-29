/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	class TI_API TThread
	{
	public:
		TThread(const TString& Name);
		virtual ~TThread();

		enum class EPriority
		{
			Highest,
			AboveNormal,
			Normal,
			BelowNormal,
			Lowest,
			Idle
		};

		static TThreadId AccquireId();
		static void ThreadSleep(uint32 milliseconds);
		static void ThreadSleepAccurate(double milliseconds);

		static void IndicateGameThread();
		static void IndicateRenderThread();
		static void IndicateVTLoadingThread();

		static bool IsGameThread();
		static bool IsRenderThread();
		static bool IsVTLoadingThread();
		
		virtual void Start();
		virtual void Stop();

		virtual void Run() = 0;
		virtual void OnThreadStart();
		virtual void OnThreadEnd() {};

		virtual void SetPriority(EPriority InPriority);
		virtual int32 GetPriority();
		
		virtual bool ThreadRunning()
		{
			return	IsRunning;
		}
		TThreadId GetThreadId()
		{
			return ThreadId;
		}
	private:
		static void* ThreadExecute(void* param);
		void SetThreadName();
		void SetThreadDescription();
	protected:
		bool IsRunning;
		TStdThread* Thread;
		TThreadId ThreadId;

	private:
		TString	ThreadName;

		static TThreadId GameThreadId;
		static TThreadId RenderThreadId;
		static TThreadId VTLoadingThreadId;
	};

	inline bool IsGameThread()
	{
		return TThread::IsGameThread();
	}

	inline bool IsRenderThread()
	{
		return TThread::IsRenderThread();
	}
}