// GTAOSample.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "NaniteLearningTicker.h"
#include "NaniteLearningRenderer.h"

int main()
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	//_CrtSetBreakAlloc(-1);
	{
		TEngineDesc Desc;
		Desc.Name = "NaniteTrees";
		Desc.Width = 1920;
		Desc.Height = 1080;

		// Create TiX engine
		TEngine::Create(Desc);

		TNaniteLearningTicker* Ticker = ti_new TNaniteLearningTicker;
		Ticker->SetupScene();

		FSceneInterface* Scene = TEngine::Get()->UseDefaultScene();
		FNaniteLearningRenderer* Renderer = ti_new FNaniteLearningRenderer(Scene);
		Renderer->SetShadowBias(0.005f);

		TEngine::Get()->AddTicker(Ticker);
		TEngine::Get()->SetRenderer(Renderer);

		// Start Loop
		TEngine::Get()->Start();

		// Destory
		ti_delete Ticker;
		TEngine::Destroy();
	}

	return 0;
}

