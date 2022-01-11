/*
TiX Engine v3.0 Copyright (C) 2022~2025
By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "TScene.h"
#include "FSceneLights.h"

namespace tix
{
	void TScene::LoadSceneAync(const TString& InSceneAssetName)
	{
		TI_ASSERT(SceneAssetInLoading == nullptr);
		// Load resource to library
		SceneAssetInLoading = ti_new TAsset(InSceneAssetName);
		SceneAssetInLoading->Load(false);
	}
}
