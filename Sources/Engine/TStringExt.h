/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	inline void TStringTrim(TString& String)
	{
		if (!String.empty())
		{
			String.erase(0, String.find_first_not_of(" \r"));
			String.erase(String.find_last_not_of(" \r") + 1);
		}
	}

	inline void TStringReplace(TString &String, const TString& StringSrc, const TString& StringDest)
	{
		TString::size_type Pos = 0;
		TString::size_type SrcLen = StringSrc.size();
		TString::size_type DesLen = StringDest.size();
		Pos = String.find(StringSrc, Pos);
		while ((Pos != string::npos))
		{
			String.replace(Pos, SrcLen, StringDest);
			Pos = String.find(StringSrc, (Pos + DesLen));
		}
	}

	inline TString& TStringToLower(TString& text)
	{
		transform(text.begin(), text.end(), text.begin(), ::tolower);
		return text;
	}

	inline TString& TStringToUpper(TString& text)
	{
		transform(text.begin(), text.end(), text.begin(), ::toupper);
		return text;
	}

	inline void TStringSplit(const TString& S, int8 C, TVector<TString>& OutStrings)
	{
		TString::size_type Pos1, Pos2;
		Pos2 = S.find(C);
		Pos1 = 0;
		while (std::string::npos != Pos2)
		{
			OutStrings.push_back(S.substr(Pos1, Pos2 - Pos1));

			Pos1 = Pos2 + 1;
			Pos2 = S.find(C, Pos1);
		}
		if (Pos1 != S.length())
			OutStrings.push_back(S.substr(Pos1));
	}

	inline void GetPathAndName(const TString& FullPathName, TString& Path, TString& Name)
	{
		TString FullPathName1 = FullPathName;
		TStringReplace(FullPathName1, "\\", "/");
		TString::size_type SlashPos = FullPathName1.rfind('/');
		if (SlashPos != TString::npos)
		{
			Path = FullPathName1.substr(0, SlashPos + 1);
			Name = FullPathName1.substr(SlashPos + 1);
		}
		else
		{
			Path = "";
			Name = FullPathName;
		}
	}

	inline TString GetExtName(const TString& FullPathName)
	{
		TString::size_type DotPos = FullPathName.rfind('.');
		if (DotPos == TString::npos)
			return "";
		else
			return FullPathName.substr(DotPos + 1);
	}
}
