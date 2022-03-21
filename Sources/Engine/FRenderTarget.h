/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	enum E_RT_ATTACH_TYPE
	{
		ERTAT_TEXTURE,
		ERTAT_TEXTURE_CUBE,
		ERTAT_RENDERBUFFER,

		ERTAT_COUNT,
	};

	enum E_RT_FLAG
	{
		ERTF_COMPILED = 1 << 0,
	};

	enum class ERenderTargetLoadAction : uint8
	{
		DontCare,
		Load,
		Clear
	};

	enum class ERenderTargetStoreAction : uint8
	{
		DontCare,
		Store,
		MultiSampleResolve,
		StoreAndMultiSampleResolve
	};

	class TI_API FRenderTarget : public FRenderResource
	{
	public:
		static FRenderTargetPtr Create(int32 W, int32 H);

		struct RTBuffer
		{
			FTexturePtr Texture;
			//FRenderTargetResourcePtr RTResource;
			E_RT_COLOR_BUFFER BufferIndex;
			ERenderTargetLoadAction LoadAction;
			ERenderTargetStoreAction StoreAction;

			RTBuffer()
				: BufferIndex(ERTC_INVALID)
				, LoadAction(ERenderTargetLoadAction::DontCare)
				, StoreAction(ERenderTargetStoreAction::DontCare)
			{}
			~RTBuffer()
			{
				Texture = nullptr;
				//RTResource = nullptr;
			}
		};

		const FInt2& GetDemension() const
		{
			return Demension;
		}

		int32 GetColorBufferCount() const
		{
			return ColorBuffers;
		}

		const RTBuffer& GetColorBuffer(int32 ColorBufferIndex)
		{
			TI_ASSERT(ColorBufferIndex >= ERTC_COLOR0 && ColorBufferIndex < ERTC_COUNT);
			return RTColorBuffers[ColorBufferIndex];
		}
		const RTBuffer& GetDepthStencilBuffer()
		{
			return RTDepthStencilBuffer;
		}

		virtual void AddColorBuffer(E_PIXEL_FORMAT Format, uint32 Mips, E_RT_COLOR_BUFFER ColorBufferIndex, ERenderTargetLoadAction LoadAction, ERenderTargetStoreAction StoreAction);
		virtual void AddColorBuffer(FTexturePtr Texture, E_RT_COLOR_BUFFER ColorBufferIndex, ERenderTargetLoadAction LoadAction, ERenderTargetStoreAction StoreAction);
		virtual void AddDepthStencilBuffer(E_PIXEL_FORMAT Format, uint32 Mips, ERenderTargetLoadAction LoadAction, ERenderTargetStoreAction StoreAction);
		virtual void AddDepthStencilBuffer(FTexturePtr Texture, ERenderTargetLoadAction LoadAction, ERenderTargetStoreAction StoreAction);
		
		// For metal tile shader
		virtual void SetTileSize(const FInt2& InTileSize)
		{}
		virtual void SetThreadGroupMemoryLength(uint32 Length)
		{}

		virtual void Compile();
	protected:
		FRenderTarget(int32 W, int32 H);
		virtual ~FRenderTarget();

	protected:
		FInt2 Demension;

		RTBuffer RTColorBuffers[ERTC_COUNT];
		RTBuffer RTDepthStencilBuffer;

		int32 ColorBuffers;
		friend class FRHI;
	};
}
