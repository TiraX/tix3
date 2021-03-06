/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "TNodeLight.h"

namespace tix
{
	TNodeLight::TNodeLight(TNode* parent)
		: TNode(TNodeLight::NODE_TYPE, parent)
		, Intensity(1.0f)
		, LightColor(255,255,255,255)
		, LightFlag(0)
	{
	}

	TNodeLight::~TNodeLight()
	{
		TI_ASSERT(0);
		TI_TODO("A new way to add lights to FScene");
		//if (LightResource != nullptr)
		//{
		//	 Remove FLight from FSceneLights
		//	FLightPtr InLight = LightResource;
		//	ENQUEUE_RENDER_COMMAND(RemoveFLightFromFScene)(
		//		[InLight]()
		//		{
		//			InLight->RemoveFromSceneLights_RenderThread();
		//		});
		//	LightResource = nullptr;
		//}
	}

	void TNodeLight::UpdateAbsoluteTransformation()
	{
		TNode::UpdateAbsoluteTransformation();
		if (NodeFlag & ENF_ABSOLUTETRANSFORMATION_UPDATED)
		{
			// Calculate affect box
			const float MinimumIntensity = 0.01f;
			float AttenuationDistance = sqrt(Intensity / MinimumIntensity);
			AffectBox.Min = FFloat3(-AttenuationDistance, -AttenuationDistance, -AttenuationDistance);
			AffectBox.Max = FFloat3(AttenuationDistance, AttenuationDistance, AttenuationDistance);

			AffectBox.Move(AbsoluteTransformation.GetTranslation());

			TI_ASSERT(0);
			TI_TODO("A new way to add lights to FScene");
			//// Update FLight Position in render thread
			//FLightPtr InLight = LightResource;
			//FFloat3 InPosition = GetAbsolutePosition();
			//ENQUEUE_RENDER_COMMAND(UpdateFLightPosition)(
			//	[InLight, InPosition]()
			//	{
			//		InLight->UpdateLightPosition_RenderThread(InPosition);
			//	});

			// Notify scene , lights get dirty
			TEngine::Get()->GetScene()->SetSceneFlag(SF_LIGHTS_DIRTY, true);
		}

		TEngine::Get()->GetScene()->AddToActiveList(ESLT_LIGHTS, this);
	}

	void TNodeLight::SetRotate(const FQuat &rotate)
	{
		// keep this empty, light do not need rotate
	}

	void TNodeLight::SetScale(const FFloat3& scale)
	{
		// keep this empty, light do not need scale
	}

	void TNodeLight::CreateFLight()
	{
		TI_ASSERT(0);
		TI_TODO("A new way to add lights to FScene");

		//TI_ASSERT(LightResource == nullptr);
		//LightResource = ti_new FLight(this);

		//// Add FLight to FSceneLights
		//FLightPtr InLight = LightResource;
		//ENQUEUE_RENDER_COMMAND(AddFLightToFScene)(
		//	[InLight]()
		//	{
		//		InLight->AddToSceneLights_RenderThread();
		//	});
	}
}
