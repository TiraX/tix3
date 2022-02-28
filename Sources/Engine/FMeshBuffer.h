/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	class TI_API FVertexBuffer : public FRenderResource
	{
	public:
		FVertexBuffer();
		FVertexBuffer(const TVertexBufferDesc& InDesc);
		virtual ~FVertexBuffer();

		virtual void CreateGPUBuffer(TStreamPtr Data) override;
		virtual FGPUResourcePtr GetGPUResource() override
		{
			return GPUResourceVB;
		}

		const TVertexBufferDesc& GetDesc() const
		{
			return Desc;
		}
	protected:

	protected:
		TVertexBufferDesc Desc;
		FGPUBufferPtr GPUResourceVB;
	};

	/////////////////////////////////////////////////////////////

	class TI_API FIndexBuffer : public FRenderResource
	{
	public:
		FIndexBuffer();
		FIndexBuffer(const TIndexBufferDesc& InDesc);
		virtual ~FIndexBuffer();

		virtual void CreateGPUBuffer(TStreamPtr Data) override;
		virtual FGPUResourcePtr GetGPUResource() override
		{
			return GPUResourceIB;
		}

		const TIndexBufferDesc& GetDesc() const
		{
			return Desc;
		}
	protected:

	protected:
		TIndexBufferDesc Desc;
		FGPUBufferPtr GPUResourceIB;
	};

	/////////////////////////////////////////////////////////////

	// FInstanceBuffer, hold instance buffer
	class TI_API FInstanceBuffer : public FRenderResource
	{
	public:
		FInstanceBuffer();
		FInstanceBuffer(const TInstanceBufferDesc& InDesc);
		virtual ~FInstanceBuffer();

		virtual void CreateGPUBuffer(TStreamPtr Data) override;
		virtual FGPUResourcePtr GetGPUResource() override
		{
			return GPUResourceInsB;
		}

		const TInstanceBufferDesc& GetDesc() const
		{
			return Desc;
		}

	private:
		TInstanceBufferDesc Desc;
		FGPUBufferPtr GPUResourceInsB;
	};
}
