/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"

namespace tix
{
	TTexture::TTexture(const TTextureDesc& InDesc)
		: TResource(ERES_TEXTURE)
		, Desc(InDesc)
	{
		// Init Default Array
		int32 Faces = 0;
		if (InDesc.Type == ETT_TEXTURE_2D)
			Faces = 1;
		else if (InDesc.Type == ETT_TEXTURE_CUBE)
			Faces = 6;
		else
		{
			// Add more support in future
			RuntimeFail();
		}
		TextureData.resize(Faces);
	}

	TTexture::~TTexture()
	{
		ClearSurfaceData();
	}

	void TTexture::AddSurface(
		int32 FaceIndex, 
		int32 MipLevel, 
		int32 Width, 
		int32 Height, 
		const uint8* Data, 
		int32 RowPitch, 
		int32 DataSize
	)
	{
		if (MipLevel == 0)
		{
			TextureData[FaceIndex] = ti_new TImage(Desc.Format, Width, Height);
			if (Desc.Mips > 1)
				TextureData[FaceIndex]->AllocEmptyMipmaps();
			TI_ASSERT(TextureData[FaceIndex]->GetMipmapCount() == Desc.Mips);
		}

		TImage::TSurfaceData& MipData = TextureData[FaceIndex]->GetMipmap(MipLevel);
		TI_ASSERT(MipData.W == Width && MipData.H == Height);
		TI_ASSERT(MipData.RowPitch == RowPitch && MipData.Data.GetLength() == DataSize);

		memcpy(MipData.Data.GetBuffer(), Data, DataSize);
	}

	void TTexture::ClearSurfaceData()
	{
		for (auto& Image : TextureData)
		{
			Image = nullptr;
		}
		TextureData.clear();
	}

	void TTexture::InitRenderThreadResource()
	{
		TI_ASSERT(TextureResource == nullptr);
		TextureResource = FTexture::CreateTexture(Desc);
		TextureResource->SetResourceName(GetResourceName());

		FTexturePtr Texture_RT = TextureResource;
		TVector<TImagePtr> Data = TextureData;
		ENQUEUE_RENDER_COMMAND(TTextureUpdateFTexture)(
			[Texture_RT, Data]()
			{
				Texture_RT->CreateGPUTexture(FRHI::Get()->GetDefaultCmdList(), Data);
			});
		Data.clear();
		TextureData.clear();
	}

	void TTexture::DestroyRenderThreadResource()
	{
		if (TextureResource != nullptr)
		{
			FTexturePtr Texture_RT = TextureResource;
			ENQUEUE_RENDER_COMMAND(TTextureDestroyFTexture)(
				[Texture_RT]()
				{

					//_LOG(Log, "Verify ref count %d.\n", Texture_RT->referenceCount());
					//Texture_RT = nullptr;
				});
			TextureResource = nullptr;
		}
	}
}
