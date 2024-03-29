	/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	class FRenderThread;

	enum E_Platform
	{
		EP_Unknown,
		EP_Windows,
		EP_Mac,
		EP_IOS,
		EP_Android,
	};
	
	struct TAppInfo
	{
		int32 Width;
		int32 Height;
		float ContentScale;
		
		TAppInfo()
			: Width(0)
			, Height(0)
			, ContentScale(1.f)
		{
		}
	};

	class TEngine
	{
	public:
		TI_API static const TAppInfo& GetAppInfo();
		TI_API static TEngine* Get();
		TI_API static void	Create(const TEngineDesc& Config);
		TI_API static void	Destroy();

		TI_API static float GameTime();

		static E_Platform GetPlatform()
		{
			return CurrentPlatform;
		}

		TI_API void Start();
		TI_API void Shutdown();
		TI_API TDevice*	GetDevice();

		TScene* GetScene()
		{
			return Scene;
		}

		TI_API FSceneInterface* UseDefaultScene();
		TI_API void UseDefaultRenderer(FSceneInterface* Scene);
		TI_API void SetScene(FSceneInterface* Scene);
		TI_API void SetRenderer(FRendererInterface* Renderer);
		TI_API void AddTicker(TTicker* Ticker);

		TI_API void FreezeTick();
		TI_API void TickStepNext();

		TI_API void LowMemoryWarning() {};
		
#if defined (TI_PLATFORM_IOS)
		void TickIOS();
#endif
		// Tasks
		TI_API void AddTask(TTask * Task);

	private:
		static TAppInfo AppInfo;
		TEngine();
		~TEngine();
		static TEngine* s_engine;

	protected:
		// Init every thing for engine
		void Init(const TEngineDesc& Config);
		void Tick();
		void TickFinished();

		void BeginFrame();
		void EndFrame();

		void DoTasks();

	private:
		static E_Platform CurrentPlatform;
		bool IsRunning;
		TLog* LogSystem;
		TDevice * Device;

		TScene * Scene;
		TAssetLibrary * ResourceLibrary;

		uint64 LastFrameTime;
		TVector<TTicker*> Tickers;

		float GameTimeElapsed;

		bool bFreezeTick;
		bool bStepNext;

		typedef TThreadSafeQueue<TTask*> TTaskQueue;
		TTaskQueue MainThreadTasks;
	};
}
