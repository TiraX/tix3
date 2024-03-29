/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "FRenderThread.h"
#include "FRendererInterface.h"
#include "FRHI.h"

namespace tix
{
#if COMPILE_WITH_RHI_METAL
#   define AUTORELEASE_POOL_START @autoreleasepool {
#   define AUTORELEASE_POOL_END }
#else
#   define AUTORELEASE_POOL_START
#   define AUTORELEASE_POOL_END
#endif
	FRenderThread* FRenderThread::RenderThread = nullptr;
	bool FRenderThread::Inited = false;
	bool FRenderThread::ThreadEnabled = true;
	uint32 FRenderThread::FrameNum = 0;

	void FRenderThread::CreateRenderThread(bool ForceDisableThread)
	{
		FRenderThread::ThreadEnabled = !ForceDisableThread;
		TI_ASSERT(RenderThread == nullptr);
		RenderThread = ti_new FRenderThread;
		if (FRenderThread::ThreadEnabled)
		{
			RenderThread->Start();
#ifdef TI_PLATFORM_IOS
			// Give render thread priority, as iOS required 45
			// 612_hd_metal_game_performance_optimization.mp4 33'35"
			RenderThread->SetPriority(45);
#endif
		}
		else
		{
			RenderThread->OnThreadStart();
		}
	}

	void FRenderThread::DestroyRenderThread()
	{
		TI_ASSERT(RenderThread != nullptr);
		if (FRenderThread::ThreadEnabled)
		{
			RenderThread->Stop();
		}
		else
		{
			RenderThread->OnThreadEnd();
		}
		ti_delete RenderThread;
		RenderThread = nullptr;
	}

	FRenderThread* FRenderThread::Get()
	{
		return RenderThread;
	}

	bool FRenderThread::IsInited()
	{
		return Inited;
	}

	FRenderThread::FRenderThread()
		: TThread("RenderThread")
		, TriggerNum(0)
		, LastRTFrameTime(0)
		, RTLiveTime(0.f)
		, RTFrameTime(0.f)
		, PreFrameIndex(0)
		, RenderFrameIndex(0)
		, RHI(nullptr)
		, Renderer(nullptr)
		, RenderScene(nullptr)
	{
		RHI = FRHI::CreateRHI();
	}

	FRenderThread::~FRenderThread()
	{
	}

	void FRenderThread::AssignScene(FSceneInterface* InScene)
	{
		TI_ASSERT(IsRenderThread());
		RenderScene = InScene;
	}

	void FRenderThread::AssignRenderer(FRendererInterface* InRenderer)
	{
		TI_ASSERT(IsRenderThread());
		Renderer = InRenderer;
		Renderer->InitInRenderThread();
	}

	void FRenderThread::CreateRenderComponents()
	{
		// Init RHI in render thread.
		RHI->InitRHI();
	}

	void FRenderThread::DestroyRenderComponents()
	{
		// Finish all in-flight rendering
		RHI->WaitingForGpu();

		// Finish all un-finished tasks
		for (int32 i = 0; i < FRHIConfig::FrameBufferNum; i++)
		{
			DoRenderTasks();
		}

		// Release renderer
		TI_ASSERT(Renderer != nullptr);
		ti_delete Renderer;

		SAFE_DELETE(RenderScene);

		// Release RHI
		FRHI::ReleaseRHI();
	}

	void FRenderThread::Run()
	{
		// Waiting for Game thread tick
		WaitForRenderSignal();

		// Update Render Thread live time
		{
			if (LastRTFrameTime == 0)
				LastRTFrameTime = TTimer::GetCurrentTimeMillis();
			uint64 CurrentFrameTime = TTimer::GetCurrentTimeMillis();
			uint32 Delta = (uint32)(CurrentFrameTime - LastRTFrameTime);
			RTFrameTime = Delta * 0.001f;
			RTLiveTime += RTFrameTime;
			LastRTFrameTime = CurrentFrameTime;
		}
		
		AUTORELEASE_POOL_START
		
		RHI->BeginFrame();
		// Do render thread tasks
		DoRenderTasks();

		if (Renderer != nullptr)
		{
			Renderer->InitRenderFrame();
			Renderer->Render(RHI->GetDefaultCmdList());
			Renderer->EndRenderFrame();
		}
		RHI->EndFrame();
		
		++FrameNum;
		AUTORELEASE_POOL_END
	}

	void FRenderThread::OnThreadStart()
	{
		TThread::OnThreadStart();
		TThread::IndicateRenderThread();

		CreateRenderComponents();

		Inited = true;
	}

	void FRenderThread::OnThreadEnd()
	{
		TThread::OnThreadEnd();

		DestroyRenderComponents();
	}

	void FRenderThread::Stop()
	{
		if (FRenderThread::ThreadEnabled)
		{
			if (Thread != nullptr)
			{
				TriggerRenderAndStop();
				Thread->join();

				ti_delete Thread;
				Thread = nullptr;
			}
		}
	}

	void FRenderThread::DoRenderTasks()
	{
		TI_ASSERT(IsRenderThread());
		TTask* Task;
		auto& Tasks = RenderFrames[RenderFrameIndex].FrameTasks;
		while (Tasks.GetSize() > 0)
		{
			Tasks.PopFront(Task);
			Task->Execute();
#if TIX_DEBUG_RENDER_TASK_NAME
			_LOG(ELog::Log, "%d - Do RT Task: %s\n", RenderFrameIndex, Task->GetTaskName().c_str());
#endif

			// release task memory
			if (!Task->HasNextTask())
			{
				ti_delete Task;
				Task = nullptr;
			}
		}

		// Move to next
		RenderFrameIndex = (RenderFrameIndex + 1) % FRHIConfig::FrameBufferNum;
	}

	void FRenderThread::TriggerRender()
	{
		if (FRenderThread::ThreadEnabled)
		{
			// Send a signal to trigger Run() in a single thread
			TUniqueLock<TMutex> RenderLock(RenderMutex);
			// Add Trigger Number
			++TriggerNum;
			// Frame index move to next frame. Close current frame data
			PreFrameIndex = (PreFrameIndex + 1) % FRHIConfig::FrameBufferNum;
			RenderCond.notify_one();
		}
		else
		{
			// Call Run() in this thread
			Run();
		}
	}

	void FRenderThread::TriggerRenderAndStop()
	{
		if (FRenderThread::ThreadEnabled)
		{
			TUniqueLock<TMutex> RenderLock(RenderMutex);
			// Add Trigger Number
			++TriggerNum;
			// Frame index move to next frame. Close current frame data
			PreFrameIndex = (PreFrameIndex + 1) % FRHIConfig::FrameBufferNum;
			IsRunning = false;
			RenderCond.notify_one();
		}
	}

	void FRenderThread::WaitForRenderSignal()
	{
		if (FRenderThread::ThreadEnabled)
		{
			TUniqueLock<TMutex> RenderLock(RenderMutex);
			--TriggerNum;
			RenderCond.wait(RenderLock);
		}
	}

	void FRenderThread::AddTaskToFrame(TTask* Task)
	{
		TI_ASSERT(!IsRenderThread());
		RenderFrames[PreFrameIndex].FrameTasks.PushBack(Task);
	}
}
