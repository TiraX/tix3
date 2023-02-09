/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "NaniteLearningTicker.h"
#include "NaniteLearningRenderer.h"
#include "Shaders/Dx12/NaniteEnableMeshShader.h"

TNaniteLearningTicker::TNaniteLearningTicker()
{
}

TNaniteLearningTicker::~TNaniteLearningTicker()
{
}

void TNaniteLearningTicker::Tick(float Dt)
{
}

bool TNaniteLearningTicker::OnEvent(const TEvent& E)
{
	if (E.type == EET_KEY_DOWN)
	{
		if (E.param == KEY_SPACE)
			FNaniteLearningRenderer::FreezeCulling();
	}
	return true;
}

void TNaniteLearningTicker::SetupScene()
{
	// Preload Env cube
	const TString IBL = "EnvIBL.tasset";
	TAssetLibrary::Get()->LoadAsset(IBL);

	// PreLoad default material first
	//const TString DefaultMaterial = "M_Debug.tasset";
	//const TString DefaultMaterialInstance = "DebugMaterial.tasset";
	//TAssetLibrary::Get()->LoadAsset(DefaultMaterial);
	//TAssetLibrary::Get()->LoadAsset(DefaultMaterialInstance);

	// Setup camera
	TNodeCamera* Cam = TEngine::Get()->GetScene()->GetActiveCamera();
	Cam->SetNearValue(1.f);
#if NANITE_DEMO == NANITE_TREE_NO_TESS
	const float CamScale = 100.f;
	Cam->SetPosition(FFloat3(7.42432976f, 2.80526447f, 4.27628517f) * CamScale);
	Cam->SetTarget(FFloat3(-1.16132188f, -0.711122870f, 6.25598240f) * CamScale);

	Cam->SetPosition(FFloat3(-229.362961f, -127.972183f, 1562.98779f));
	Cam->SetTarget(FFloat3(-116.132187f, -71.1122894f, 625.598267f));

	Cam->SetNearValue(10.f);
	Cam->SetFarValue(10000.f);
#elif NANITE_DEMO == NANITE_GRID_WITH_TESS
	// Camera for SM_NaniteTess
	//Cam->SetPosition(FFloat3(90.5831604f, -99.6057205f, 208.611404f));
	//Cam->SetTarget(FFloat3(-61.1573257f, 31.3225346f, 94.3450317f));

	// Camera for SM_NaniteTessGrid
	Cam->SetPosition(FFloat3(17.9838886f, -47.2413940f, 111.341827f));
	Cam->SetTarget(FFloat3(-14.9610243f, -13.4057360f, -15.2703915f));
#endif



	TNodeCameraNav* NavCam = dynamic_cast<TNodeCameraNav*>(Cam);
	TI_ASSERT(NavCam);
#if NANITE_DEMO == NANITE_TREE_NO_TESS
	NavCam->SetDollySpeed(NavCam->GetDollySpeed() * CamScale * 0.4f);
#endif

	// For mesh shader tess topology debug
	//Cam->SetPosition(FFloat3(4.32689857f, -4.29783916f, 4.67585325f));
	//Cam->SetTarget(FFloat3(2.74339199f, -2.67151904f, -15.4096231f));
	//NavCam->SetDollySpeed(NavCam->GetDollySpeed() * CamScale * 0.02f);

	//Cam->SetPosition(FFloat3(48.2126007f, -46.6343880f, 7.32477570f));
	//Cam->SetTarget(FFloat3(46.9464569f, -45.3535156f, -22.3504772f));
}