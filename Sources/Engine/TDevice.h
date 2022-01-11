/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	class TDevice
	{
	public:
		static TDevice* CreateDevice(const TString& Name, int32 Width, int32 Height);
		static void DestoryDevice(TDevice* Device);

		virtual bool Run() = 0;

		FInt2 GetDeviceSize()
		{
			return FInt2(Width, Height);
		}

		int32 GetWidth()
		{
			return Width;
		}

		int32 GetHeight()
		{
			return Height;
		}

		TInput* GetInput()
		{
			return Input;
		}

		const TString& GetAbsolutePath() const
		{
			return AbsolutePath;
		}

		virtual void Resize(int32 w, int32 h);

	protected:
		TDevice(int32 w, int32 h);
		virtual ~TDevice();

	protected:
		int32 Width;
		int32 Height;

		TInput* Input;
		TString AbsolutePath;
	};
}