/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	enum E_LIGHT_FLAG
	{
		ELF_LIGHT_POS_DIRTY = 1 << 0,

		ELF_DISPLAYABLE_DIRTY = (ELF_LIGHT_POS_DIRTY ),
	};

	class TNodeLight : public TNode
	{
		DECLARE_NODE_WITH_CONSTRUCTOR(Light);
	public:
		virtual ~TNodeLight();

		void CreateFLight();

		// light do not need scale and rotate,
		// make these 2 functions empty
		virtual void SetScale(const FFloat3& scale);
		virtual void SetRotate(const FQuat& rotate);

		void SetLightFlag(uint32 flag, bool enable)
		{
			if (enable)
				LightFlag |= flag;
			else
				LightFlag &= ~flag;
		}
		uint32 GetLightFlag()
		{
			return	LightFlag;
		}
		void SetIntensity(float i)
		{
			Intensity = i;
		}
		float GetIntensity()
		{
			return	Intensity;
		}
		void SetColor(const SColor& c)
		{
			LightColor = c;
		}
		const SColor& GetColor()
		{
			return LightColor;
		}
		const FBox& GetAffectBox()
		{
			return	AffectBox;
		}

	protected:
		virtual void UpdateAbsoluteTransformation();

	protected:
		float Intensity;
		SColor LightColor;
		uint32 LightFlag;

		FBox AffectBox;
	};
} // end namespace tix

