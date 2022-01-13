/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

#if COMPILE_WITH_RHI_DX12

namespace tix
{
	class FUniformBufferDx12 : public FUniformBuffer
	{
	public:
		FUniformBufferDx12(uint32 InStructureSizeInBytes, uint32 Elements, uint32 InFlag);
		virtual ~FUniformBufferDx12();

		static uint32 AlignForUavCounter(uint32 InSize)
		{
			const uint32 Alignment = D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT;
			return (InSize + (Alignment - 1)) & ~(Alignment - 1);
		}

		virtual uint32 GetCounterOffset() const override
		{
			if ((Flag & (uint32)EGPUResourceFlag::UavWithCounter) != 0)
			{
				return AlignForUavCounter(GetElements() * GetStructureSizeInBytes());
			}
			return 0;
		}

		ComPtr<ID3D12Resource> GetResource()
		{
			return nullptr;// BufferResource.GetResource();
		}

		
	protected:

	protected:
		//FGPUResourceDx12 BufferResource;
		friend class FRHIDx12;
	};

	/////////////////////////////////////////////////////////
	class FUniformBufferReadableDx12 : public FUniformBufferDx12
	{
	public:
		FUniformBufferReadableDx12(uint32 InStructureSizeInBytes, uint32 Elements, uint32 InFlag);
		virtual ~FUniformBufferReadableDx12();

		virtual TStreamPtr ReadBufferData() override;
	protected:
		FGPUResourceDx12 ReadbackResource;
		friend class FRHIDx12;
	};
}

#endif	// COMPILE_WITH_RHI_DX12
