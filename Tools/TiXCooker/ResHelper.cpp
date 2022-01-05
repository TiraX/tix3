/*
	TiX Engine v2.0 Copyright (C) 2018~2021
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"

#include "ResHelper.h"
#include <fstream>

namespace tix
{
	void ConvertJArrayToArray(const TJSONNode& JArray, TVector<float>& OutArray)
	{
		TI_ASSERT(JArray.IsArray());
		int32 JArraySize = JArray.Size();
		OutArray.reserve(JArraySize);
		for (int32 i = 0; i < JArraySize; ++i)
		{
			OutArray.push_back(JArray[i].GetFloat());
		}
	}
	void ConvertJArrayToArray(const TJSONNode& JArray, TVector<int32>& OutArray)
	{
		TI_ASSERT(JArray.IsArray());
		int32 JArraySize = JArray.Size();
		OutArray.reserve(JArraySize);
		for (int32 i = 0; i < JArraySize; ++i)
		{
			OutArray.push_back(JArray[i].GetInt());
		}
	}
	void ConvertJArrayToArray(const TJSONNode& JArray, TVector<TString>& OutArray)
	{
		TI_ASSERT(JArray.IsArray());
		int32 JArraySize = JArray.Size();
		OutArray.reserve(JArraySize);
		for (int32 i = 0; i < JArraySize; ++i)
		{
			OutArray.push_back(JArray[i].GetString());
		}
	}
	void ConvertJArrayToVec3(const TJSONNode& JArray, vector3df& V3)
	{
		TI_ASSERT(JArray.IsArray() && JArray.Size() == 3);
		int32 JArraySize = JArray.Size();
		V3.X = JArray[0].GetFloat();
		V3.Y = JArray[1].GetFloat();
		V3.Z = JArray[2].GetFloat();
	}
	void ConvertJArrayToQuat(const TJSONNode& JArray, quaternion& Q4)
	{
		TI_ASSERT(JArray.IsArray() && JArray.Size() == 4);
		int32 JArraySize = JArray.Size();
		Q4.X = JArray[0].GetFloat();
		Q4.Y = JArray[1].GetFloat();
		Q4.Z = JArray[2].GetFloat();
		Q4.W = JArray[3].GetFloat();
	}

}
