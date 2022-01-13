/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	enum class ETextureFlag : uint32
	{
		None = 0,
		ColorBuffer = 1 << 0,
		DsBuffer = 1 << 1,
		Uav = 1 << 2,

		// Used for iOS Metal
		MemoryLess = 1 << 3,
	};

	class FTexture : public FRenderResource
	{
	public:
		FTexture(const TTextureDesc& Desc, uint32 InFlag = 0);
		virtual ~FTexture();

		virtual void CreateGPUTexture(const TVector<TImagePtr>& Data = TVector<TImagePtr>()) override;
		virtual FGPUResourcePtr GetGPUResource() override
		{
			return GPUTexture;
		}

		const TTextureDesc& GetDesc() const
		{
			return TextureDesc;
		}

		bool HasTextureFlag(ETextureFlag Flag) const
		{
			return (TextureFlag & (uint32)Flag) != 0;
		}
		
		void SetTextureFlag(ETextureFlag Flag, bool bEnable)
		{
			if (bEnable)
			{
				TextureFlag |= (uint32)Flag;
			}
			else
			{
				TextureFlag &= ~((uint32)Flag);
			}
		}

		int32 GetWidth() const
		{
			return TextureDesc.Width;
		}
		int32 GetHeight() const
		{
			return TextureDesc.Height;
		}

		virtual void PrepareDataForCPU() {}
		virtual TImagePtr ReadTextureData() { return nullptr; }

	protected:


	protected:
		TTextureDesc TextureDesc;
		uint32 TextureFlag;
		FGPUTexturePtr GPUTexture;
	};

	///////////////////////////////////////////////////////////
	class FTextureReadable : public FTexture
	{
	public:
		FTextureReadable(const TTextureDesc& Desc);
		virtual ~FTextureReadable();

		virtual void CreateGPUTexture(const TVector<TImagePtr>& Data = TVector<TImagePtr>()) override;

		virtual void PrepareDataForCPU();
		virtual TImagePtr ReadTextureData() override;

	protected:

	protected:
		FGPUBufferPtr GPUReadbackBuffer;
	};
}
