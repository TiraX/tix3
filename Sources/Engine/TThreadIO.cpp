/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "TThreadIO.h"

namespace tix
{
	TThreadId TThreadIO::IOThreadId;

	TThreadIO::TThreadIO()
		: TTaskThread("IOThread")
	{
	}

	TThreadIO::~TThreadIO()
	{
	}

	void TThreadIO::OnThreadStart()
	{
		TTaskThread::OnThreadStart();

		IOThreadId = ThreadId;
	}
}
