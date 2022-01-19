/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	// help functions
	inline void FillZero4(TStream& Stream)
	{
		char zero[64] = { 0 };
		int32 bytes = TMath::Align4(Stream.GetLength()) - Stream.GetLength();
		TI_ASSERT(bytes <= 64);
		Stream.Put(zero, bytes);
	}

	inline int32 AddStringToList(TVector<TString>& Strings, const TString& String)
	{
		for (int32 s = 0; s < (int32)Strings.size(); ++s)
		{
			if (Strings[s] == String)
			{
				return s;
			}
		}
		Strings.push_back(String);
		return (int32)Strings.size() - 1;
	}

	inline void SaveStringList(const TVector<TString>& Strings, TStream& Stream)
	{
		char zero[8] = { 0 };

		int32* string_offsets = ti_new int32[Strings.size()];
		int32 offset = 0;
		for (int32 i = 0; i < (int32)Strings.size(); ++i)
		{
			const TString& s = Strings[i];
			offset += ((s.size() + 4) & ~3);
			string_offsets[i] = offset;
		}

		Stream.Put(string_offsets, (int32)Strings.size() * sizeof(int32));
		FillZero4(Stream);
		for (int i = 0; i < (int)Strings.size(); ++i)
		{
			const TString& s = Strings[i];
			int32 len = (((int32)s.size() + 4) & ~3);
			int32 real_len = (int32)s.size();
			Stream.Put(s.c_str(), real_len);
			Stream.Put(zero, len - real_len);;
		}
		ti_delete[] string_offsets;
	}

	inline uint8 FloatToUNorm(float n)
	{
		if (n < -1.f)
			n = -1.f;
		if (n > 1.f)
			n = 1.f;
		n = n * 0.5f + 0.5f;
		float n0 = n * 255.f + 0.5f;
		return (uint8)n0;
	}
	inline uint8 FloatToColor(float n)
	{
		if (n < 0.f)
			n = 0.f;
		if (n > 1.f)
			n = 1.f;
		float n0 = n * 255.f + 0.5f;
		return (uint8)n0;
	}

	template< class T >
	int32 IndexInArray (const T& v, const TVector<T>& Arr)
	{
		for (int32 i = 0; i < (int32)Arr.size(); i++)
		{
			if (Arr[i] == v)
				return i;
		}
		return -1;
	};
	/////////////////////////////////////////////////////////////////
}
