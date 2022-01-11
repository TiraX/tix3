/*
	TiX Engine v2.0 Copyright (C) 2018~2021
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	class FLight : public IReferenceCounted
	{
	public:
		FLight(TNodeLight * Light);
		~FLight();

		void AddToSceneLights_RenderThread();
		void RemoveFromSceneLights_RenderThread();
		void UpdateLightPosition_RenderThread(const FFloat3& InPosition);

		void SetLightIndex(uint32 Index)
		{
			LightIndex = Index;
		}
		uint32 GetLightIndex() const
		{
			return LightIndex;
		}
		const FFloat3& GetLightPosition() const
		{
			return Position;
		}
		const SColorf& GetLightColor() const
		{
			return Color;
		}
	protected:
		void InitFromLightNode(TNodeLight * Light);

	protected:
		uint32 LightIndex;	// The index light allocated in FSceneLights
		FFloat3 Position;
		SColorf Color;
	};
} // end namespace tix

