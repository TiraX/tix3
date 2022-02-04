/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	struct FEnvironmentInfo
	{
		FFloat3 MainLightDirection;
		SColorf MainLightColor;
		float MainLightIntensity;

		// Sky Irradiance go with IBL Textures
		//FSHVectorRGB3 SkyIrradiance;

		FEnvironmentInfo()
			: MainLightDirection(0, 0, -1)
			, MainLightColor(1.f, 1.f, 1.f, 1.f)
			, MainLightIntensity(3.14f)
		{}
	};

	class TNodeEnvironment : public TNode
	{
		DECLARE_NODE_WITH_CONSTRUCTOR(Environment);
	public:
		enum E_ENV_FLAG
		{
			ENVF_MAIN_LIGHT_DIRTY = 1 << 0,
			ENVF_SKY_LIGHT_DIRTY = 1 << 1
		};
		virtual ~TNodeEnvironment();

		virtual void UpdateAllTransformation() override;

		TI_API void SetMainLightDirection(const FFloat3& InDir);
		TI_API void SetMainLightColor(const SColorf& InColor);
		TI_API void SetMainLightIntensity(float InIntensity);

		const FFloat3& GetMainLightDirection() const
		{
			return EnvInfo.MainLightDirection;
		}
		const SColorf& GetMainLightColor() const
		{
			return EnvInfo.MainLightColor;
		}
		float GetMainLightIntensity() const
		{
			return EnvInfo.MainLightIntensity;
		}
	protected:

	protected:
		uint32 EnvFlags;
		FEnvironmentInfo EnvInfo;
	};

} // end namespace tix

