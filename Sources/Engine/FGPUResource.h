/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	struct FGPUResourceDesc
	{
		uint32 Flag;
		uint32 Type;
		uint32 BufferSize;
	};

	class FGPUResource : public IReferenceCounted
	{
	public:
		FGPUResource()
		{}
		virtual ~FGPUResource()
		{}

		virtual void Init(const FGPUResourceDesc& Desc, TStreamPtr Data) = 0;

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
