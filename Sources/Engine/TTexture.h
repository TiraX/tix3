/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	//! Texture types.
	enum E_TEXTURE_TYPE
	{
		ETT_TEXTURE_1D,
		ETT_TEXTURE_2D,
		ETT_TEXTURE_3D,

		ETT_TEXTURE_CUBE,

		ETT_TEXTURE_UNKNOWN,
		ETT_TEXTURE_TYPE_NUM
	};

	//! Texture filter types.
	enum E_TEXTURE_FILTER_TYPE
	{
		//! Nearest texel filter.
		ETFT_MINMAG_NEAREST_MIP_NEAREST = 0,

		//! Bilinear texel filter, no mipmaps.
		ETFT_MINMAG_LINEAR_MIP_NEAREST,
		
		//! Interpolated nearest texel filter between mipmap levels.
		ETFT_MINMAG_NEAREST_MIPMAP_LINEAR,

		//! Trilinear texel filter.
		ETFT_MINMAG_LINEAR_MIPMAP_LINEAR,

		ETFT_COUNT,
		ETFT_UNKNOWN = ETFT_COUNT,
	};

	//! Texture coord clamp mode outside [0.0, 1.0]
	enum E_TEXTURE_ADDRESS_MODE
	{
		//! Texture repeats
		ETC_REPEAT = 0,

		//! Texture is clamped to the edge pixel
		ETC_CLAMP_TO_EDGE,

		//! Texture is alternatingly mirrored (0..1..0..1..0..)
		ETC_MIRROR,

		ETC_COUNT,
		ETC_UNKNOWN = ETC_COUNT,
	};

	enum E_TEXTURE_PARAMETER
	{
		ETP_MIN_FILTER,
		ETP_MAG_FILTER,
		ETP_WRAP_S,
		ETP_WRAP_T,
		ETP_WRAP_R,

		ETP_COUNT,
	};

	struct TTextureDesc
	{
		E_TEXTURE_TYPE Type;
		E_PIXEL_FORMAT Format;
		int32 Width;
		int32 Height;
		int32 Depth;
		E_TEXTURE_ADDRESS_MODE AddressMode;
		uint32 SRGB;
		uint32 Mips;
		SColorf ClearColor;

		// Sphere Harmonic for IBL
		FSHVectorRGB3 SH;

		TTextureDesc()
			: Type(ETT_TEXTURE_2D)
			, Format(EPF_UNKNOWN)
			, Width(0)
			, Height(0)
			, Depth(1)
			, AddressMode(ETC_REPEAT)
			, SRGB(0)
			, Mips(1)
			, ClearColor(0, 0, 0, 0)
		{}
	};

	class TTexture : public TResource
	{
	public:
		TTexture(const TTextureDesc& InDesc);
		virtual ~TTexture();

		virtual void InitRenderThreadResource() override;
		virtual void DestroyRenderThreadResource() override;

		FTexturePtr TextureResource;

		TI_API void AddSurface(
			int32 FaceIndex,
			int32 MipLevel,
			int32 Width,
			int32 Height,
			const uint8* Data,
			int32 RowPitch,
			int32 DataSize
		);
		TI_API const TTextureDesc& GetDesc() const
		{
			return Desc;
		}
		TI_API TImagePtr GetTextureData(int32 Index) const
		{
			return  TextureData[Index];
		}
		TI_API void ClearSurfaceData();
	protected:

	protected:
		TTextureDesc Desc;
		// Texture data array
		// Texture2D, Array.size = 1;
		// TextureCube, Array.size = 6;
		// TextureArray Array.size = array_size
		TVector<TImagePtr> TextureData;
	};
}
