/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	class FVertexBuffer : public FRenderResource
	{
	public:
		FVertexBuffer();
		FVertexBuffer(const TVertexBufferDesc& InDesc);
		virtual ~FVertexBuffer();

		virtual void CreateGPUResource(TStreamPtr Data) override;

		const TVertexBufferDesc& GetDesc() const
		{
			return Desc;
		}
	protected:

	protected:
		TVertexBufferDesc Desc;
		FGPUResourceBufferPtr GPUResourceVB;
	};

	///////////////////////////////////////////////////////////

	class FIndexBuffer : public FRenderResource
	{
	public:
		FIndexBuffer();
		FIndexBuffer(const TIndexBufferDesc& InDesc);
		virtual ~FIndexBuffer();

		virtual void CreateGPUResource(TStreamPtr Data) override;

		const TIndexBufferDesc& GetDesc() const
		{
			return Desc;
		}
	protected:

	protected:
		TIndexBufferDesc Desc;
		FGPUResourceBufferPtr GPUResourceIB;
	};

	///////////////////////////////////////////////////////////

	// FInstanceBuffer, hold instance buffer
	class FInstanceBuffer : public FRenderResource
	{
	public:
		FInstanceBuffer();
		FInstanceBuffer(const TInstanceBufferDesc& InDesc);
		virtual ~FInstanceBuffer();

		virtual void CreateGPUResource(TStreamPtr Data) override;

		const TInstanceBufferDesc& GetDesc() const
		{
			return Desc;
		}

	private:
		TInstanceBufferDesc Desc;
		FGPUResourceBufferPtr GPUResourceInsB;
	};
}
