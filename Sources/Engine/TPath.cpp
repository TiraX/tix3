/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "TPath.h"

namespace tix
{
	TString TPath::GetAbsolutePath(const TString& RelativePath)
	{
		return TEngine::Get()->GetDevice()->GetAbsolutePath() + RelativePath;
	}
}