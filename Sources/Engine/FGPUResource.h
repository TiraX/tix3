/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	class FGPUResource : public IReferenceCounted
	{
	public:
		FGPUResource()
		{}
		virtual ~FGPUResource()
		{}

	protected:
	};

	/////////////////////////////////////////////////////////////
	class FGPUResourceBuffer : public FGPUResource
	{
	public:
		FGPUResourceBuffer()
		{}
		virtual ~FGPUResourceBuffer()
		{}
	};

	/////////////////////////////////////////////////////////////
	class FGPUResourceTexture : public FGPUResource
	{
	public:
		FGPUResourceTexture()
		{}
		virtual ~FGPUResourceTexture()
		{}
	};
}
