/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "NaniteLearningTicker.h"
#include "NaniteLearningRenderer.h"

TNaniteLearningTicker::TNaniteLearningTicker()
{
}

TNaniteLearningTicker::~TNaniteLearningTicker()
{
}

void TNaniteLearningTicker::Tick(float Dt)
{
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
	const float CamScale = 100.f;
	Cam->SetPosition(FFloat3(7.42432976f, 2.80526447f, 4.27628517f) * CamScale);
	Cam->SetTarget(FFloat3(-1.16132188f, -0.711122870f, 6.25598240f) * CamScale);

	TNodeCameraNav* NavCam = dynamic_cast<TNodeCameraNav*>(Cam);
	TI_ASSERT(NavCam);
	NavCam->SetDollySpeed(NavCam->GetDollySpeed() * CamScale);
}