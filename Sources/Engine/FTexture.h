/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	class TI_API FTexture : public FRenderResource
	{
	public:
		static FTexturePtr CreateTexture(const TTextureDesc& Desc, uint32 InFlag = 0);
		static FTexturePtr CreateReadableTexture(const TTextureDesc& Desc, uint32 InFlag = 0);

		virtual void CreateGPUTexture(const TVector<TImagePtr>& Data = TVector<TImagePtr>()) override;
		virtual FGPUResourcePtr GetGPUResource() override
		{
			return GPUTexture;
		}

		const TTextureDesc& GetDesc() const
		{
			return TextureDesc;
		}

		bool HasTextureFlag(EGPUResourceFlag Flag) const
		{
			return (TextureFlag & (uint32)Flag) != 0;
		}
		
		void SetTextureFlag(EGPUResourceFlag Flag, bool bEnable)
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
		FTexture(const TTextureDesc& Desc, uint32 InFlag);
		virtual ~FTexture();

	protected:
		TTextureDesc TextureDesc;
		uint32 TextureFlag;
		FGPUTexturePtr GPUTexture;
	};

	/////////////////////////////////////////////////////////////
	class TI_API FTextureReadable : public FTexture
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
