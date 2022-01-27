/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	// help functions
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
		Stream.FillZero4();
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
