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
		FGPUResourceBufferPtr GPUResourceVertexBuffer;
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
		FGPUResourceBufferPtr GPUResourceIndexBuffer;
	};

	///////////////////////////////////////////////////////////

	// FInstanceBuffer, hold instance buffer
	class FInstanceBuffer : public FRenderResource
	{
	public:
		FInstanceBuffer();
		FInstanceBuffer(uint32 TotalInstancesCount, uint32 InstanceStride);
		virtual ~FInstanceBuffer();

		void TI_API SetFromTInstanceBuffer(TInstanceBufferPtr InstanceData);

		uint32 GetInstancesCount() const
		{
			return InstanceCount;
		}

		uint32 GetStride() const
		{
			return Stride;
		}

	private:
		uint32 InstanceCount;
		uint32 Stride;
	};
}
