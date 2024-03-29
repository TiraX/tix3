/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "TTaskThread.h"

namespace tix
{
	TTask::TTask()
	{
	}

	TTask::~TTask()
	{
	}

	/////////////////////////////////////////////////////////////
	TTaskThread::TTaskThread(const TString& Name)
		: TThread(Name)
		, Busy(false)
	{
	}

	TTaskThread::~TTaskThread()
	{
	}

	void TTaskThread::Run()
	{
		{
			TUniqueLock<TMutex> TaskLock(TaskMutex);
			TaskCond.wait(TaskLock);
		}

		DoTasks();
	}

	void TTaskThread::Stop()
	{
		if (Thread != nullptr)
		{
			{
				TUniqueLock<TMutex> CLock(TaskMutex);
				IsRunning = false;
				TaskCond.notify_one();
			}
			Thread->join();

			ti_delete Thread;
			Thread = nullptr;
		}
	}

	void TTaskThread::DoTasks()
	{
		TI_ASSERT(AccquireId() == ThreadId);
		TTask* Task;
		while (Tasks.GetSize() > 0)
		{
			Tasks.PopFront(Task);
			Task->Execute();

			// release task memory
			if (!Task->HasNextTask())
			{
				ti_delete Task;
				Task = nullptr;
			}
		}
		Busy = false;
	}

	void TTaskThread::AddTask(TTask* Task)
	{
		TUniqueLock<TMutex> CLock(TaskMutex);
		Busy = true;
		Tasks.PushBack(Task);
		TaskCond.notify_one();
	}
}