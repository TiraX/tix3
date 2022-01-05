/*
	TiX Engine v2.0 Copyright (C) 2018~2021
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once
#include "TCooker.h"

namespace tix
{
	struct TMIParamValue
	{
		union
		{
			int32 ValueInt;
			float ValueFloat;
			vector3df ValueVec;
			quaternion ValueQuat;
			SColorf ValueClr;
		};
		TString ValueString;

		TMIParamValue()
		{
			memset(&ValueClr, 0, sizeof(SColorf));
		}

		TMIParamValue(const TMIParamValue& Other)
		{
			memcpy(&ValueClr, &Other.ValueClr, sizeof(SColorf));
			ValueString = Other.ValueString;
		}
	};
	struct TMIParam
	{
		TString ParamName;
		uint8 ParamType;
		TMIParamValue ParamValue;
		vector2di ParamValueSize;

		TMIParam()
			: ParamName("None")
			, ParamType(MIPT_UNKNOWN)
		{
		}

		TMIParam(const TString& InParamName)
			: ParamName(InParamName)
			, ParamType(MIPT_UNKNOWN)
		{
		}
	};

	class TCookerMaterialInstance : public TCooker
	{
	public:
		TCookerMaterialInstance();
		virtual ~TCookerMaterialInstance();

		virtual EChunkLib GetCookerType() const override
		{
			return EChunkLib::MaterialInstance;
		};
		virtual bool Load(const TJSON& JsonDoc) override;
		virtual void SaveTrunk(TChunkFile& OutChunkFile) override;

		void AddParameter(const TString& InParamName, int32 Value);
		void AddParameter(const TString& InParamName, float Value);
		void AddParameter(const TString& InParamName, const vector3df& Value);
		void AddParameter(const TString& InParamName, const quaternion& Value);
		void AddParameter(const TString& InParamName, const SColorf& Value);
		void AddParameter(const TString& InParamName, const TString& Value, const vector2di& Size);

	private:
		bool IsParamExisted(const TString& InParamName);
	private:
		TString InstanceName;
		TString LinkedMaterial;
		TVector<TMIParam> ValueParameters;
		TVector<TMIParam> TextureParameters;
	};
}