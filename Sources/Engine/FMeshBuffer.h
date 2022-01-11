/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	// FMeshBuffer, hold vertex buffer and index buffer render resource
	class FMeshBuffer : public FRenderResource
	{
	public:
		FMeshBuffer();
		FMeshBuffer(const TMeshBufferDesc& InDesc);
		virtual ~FMeshBuffer();

		virtual void CreateGPUResource(TStreamPtr Data) override;

		const TMeshBufferDesc& GetDesc() const
		{
			return Desc;
		}
	protected:

	protected:
		TMeshBufferDesc Desc;

		FGPUResourceBufferPtr GPUResourceVB;
		FGPUResourceBufferPtr GPUResourceIB;
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
