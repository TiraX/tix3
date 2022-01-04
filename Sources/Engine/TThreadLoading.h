/*
	TiX Engine v2.0 Copyright (C) 2018~2021
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	class ILoadingFinishDelegate
	{
	public:
		ILoadingFinishDelegate() {}
		virtual ~ILoadingFinishDelegate() {}
		virtual void LoadingFinished(TAssetPtr InAsset) = 0;
	};

	class TLoadingTask : public TTask
	{
	public:
		enum 
		{
			STEP_IO,
			STEP_LOADING,
			STEP_BACK_TO_MAINTHREAD,
			STEP_FINISHED,
		};
		TLoadingTask()
			: LoadingStep(STEP_IO)
		{}
		virtual ~TLoadingTask() {}

		virtual void ExecuteInIOThread() = 0;
		virtual void ExecuteInLoadingThread() = 0;
		virtual void ExecuteInMainThread() = 0;

		virtual void Execute() override;

		virtual bool HasNextTask() override
		{
			return LoadingStep != STEP_FINISHED;
		}

		int32 GetLoadingStep() const
		{
			return LoadingStep;
		}

	private:
		int32 LoadingStep;
	};

	class TThreadIO;
	class TThreadLoading : public TTaskThread
	{
	public:
		static void CreateLoadingThread();
		static void DestroyLoadingThread();
		static TThreadLoading * Get();

		virtual void AddTask(TTask* Task) override;

		virtual void OnThreadStart() override;
		inline static bool IsLoadingThread()
		{
			return TThread::GetThreadId() == LoadingThreadId;
		}

	private:
		static TThreadLoading* LoadingThread;
		TThreadLoading();
		virtual ~TThreadLoading();

		virtual void Start() override;
		virtual void Stop() override;

	protected:

	protected:
		TThreadIO * IOThread;

		static TThreadId LoadingThreadId;
	};

	inline bool IsLoadingThread()
	{
		return TThreadLoading::IsLoadingThread();
	}
}
