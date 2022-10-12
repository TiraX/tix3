/*
	TiX Engine v2.0 Copyright (C) 2018~2021
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
}
