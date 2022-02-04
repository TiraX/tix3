/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"

#if COMPILE_WITH_RHI_DX12
#include "FGPUTextureDx12.h"
#include "FRHIDx12.h"
#include "FRHIDx12Conversion.h"

namespace tix
{
	inline int32 GetTextureArraySize(E_TEXTURE_TYPE TextureType)
	{
		switch (TextureType)
		{
		case tix::ETT_TEXTURE_2D:
			return 1;
		case tix::ETT_TEXTURE_CUBE:
			return 6;
		default:
			// Other types do NOT support yet
			RuntimeFail();
			break;
		}
		return 0;
	}

	void FGPUTextureDx12::Init(const FGPUTextureDesc& Desc, const TVector<TImagePtr>& Data)
	{
		FRHIDx12* RHIDx12 = static_cast<FRHIDx12*>(FRHI::Get());

		TI_ASSERT(Resource == nullptr);
		D3D12_RESOURCE_DESC TextureDesc = {};
		TextureDesc.Dimension = GetDx12TextureType(Desc.Texture.Type);
		TextureDesc.Alignment = 0;
		TextureDesc.Width = Desc.Texture.Width;
		TextureDesc.Height = Desc.Texture.Height;
		TextureDesc.DepthOrArraySize = GetTextureArraySize(Desc.Texture.Type);
		TextureDesc.MipLevels = Desc.Texture.Mips;
		TextureDesc.Format = GetDxPixelFormat(Desc.Texture.Format);
		TextureDesc.SampleDesc.Count = 1;
		TextureDesc.SampleDesc.Quality = 0;
		TextureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		TextureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		ResourceState = EGPUResourceState::CopyDest;

		D3D12_CLEAR_VALUE ClearValue = {}; 
		ClearValue.Format = TextureDesc.Format;
		if ((Desc.Flag & (uint32)EGPUResourceFlag::ColorBuffer) != 0)
		{
			TextureDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
			TextureDesc.Format = GetBaseFormat(TextureDesc.Format);
			SColorf ClearColor = Desc.Texture.ClearColor;
			ClearValue.Color[0] = ClearColor.R;
			ClearValue.Color[1] = ClearColor.G;
			ClearValue.Color[2] = ClearColor.B;
			ClearValue.Color[3] = ClearColor.A;
			ResourceState = EGPUResourceState::RenderTarget;
		}
		if ((Desc.Flag & (uint32)EGPUResourceFlag::DsBuffer) != 0)
		{
			TextureDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
			ClearValue.DepthStencil.Depth = 1.f;
			ClearValue.DepthStencil.Stencil = 0;
			ResourceState = EGPUResourceState::DepthWrite;
		}
		if ((Desc.Flag & (uint32)EGPUResourceFlag::Uav) != 0)
		{
			TextureDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		}

		D3D12_CLEAR_VALUE* pOptimizedClearValue = nullptr;
		if ((TextureDesc.Flags & (D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)) != 0)
		{
			pOptimizedClearValue = &ClearValue;
		}

		// Create upload heap resource and copy to default heap
		D3D12_RESOURCE_STATES InitState = GetDx12ResourceState(ResourceState);
		RHIDx12->CreateD3D12Resource(
			D3D12_HEAP_TYPE_DEFAULT,
			&TextureDesc,
			InitState,
			pOptimizedClearValue,
			Resource
		);

		if (Data.size() > 0)
		{
			TI_ASSERT(TextureDesc.DepthOrArraySize == (uint16)Data.size());
			const uint32 NumSubResources = TextureDesc.DepthOrArraySize * TextureDesc.MipLevels;
			const uint64 UploadBufferSize = RHIDx12->GetRequiredIntermediateSize(TextureDesc, 0, NumSubResources);
			ComPtr<ID3D12Resource> BufferUpload;
			CD3DX12_RESOURCE_DESC BufferUploadDesc = CD3DX12_RESOURCE_DESC::Buffer(UploadBufferSize);
			RHIDx12->CreateD3D12Resource(
				D3D12_HEAP_TYPE_UPLOAD,
				&BufferUploadDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				BufferUpload
			);

			// Copy data to the intermediate upload heap and then schedule a copy 
			// from the upload heap to the Texture2D.
			D3D12_SUBRESOURCE_DATA* TextureDatas = ti_new D3D12_SUBRESOURCE_DATA[NumSubResources];
			int32 SubIndex = 0;
			for (uint16 ArrayIndex = 0; ArrayIndex < TextureDesc.DepthOrArraySize; ArrayIndex++)
			{
				TImagePtr ImageData = Data[ArrayIndex];
				for (int32 Mip = 0; Mip < TextureDesc.MipLevels; Mip++)
				{
					D3D12_SUBRESOURCE_DATA& TextureData = TextureDatas[SubIndex];
					const TImage::TSurfaceData& SurfaceData = ImageData->GetMipmap(Mip);

					TextureData.pData = SurfaceData.Data.GetBuffer();
					TextureData.RowPitch = SurfaceData.RowPitch;
					TextureData.SlicePitch = SurfaceData.Data.GetLength();

					++SubIndex;
				}
			}

			// Upload the buffer data to the GPU.
			RHIDx12->UpdateD3D12Resource(
				Resource.Get(),
				BufferUpload.Get(),
				NumSubResources,
				TextureDatas);
			ti_delete[] TextureDatas;
			RHIDx12->HoldResourceReference(BufferUpload);
		}
	}
}
#endif	// COMPILE_WITH_RHI_DX12