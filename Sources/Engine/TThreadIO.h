/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	class TThreadIO : public TTaskThread
	{
	public:
		TThreadIO();
		virtual ~TThreadIO();

		virtual void OnThreadStart() override;

		inline static bool IsIOThread()
		{
			return TThread::GetThreadId() == IOThreadId;
		}

	protected:

	protected:
		static TThreadId IOThreadId;
	};

	inline bool IsIOThread()
	{
		return TThreadIO::IsIOThread();
	}
}
