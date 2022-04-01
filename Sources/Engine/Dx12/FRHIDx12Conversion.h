/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

#if COMPILE_WITH_RHI_DX12

namespace tix
{
	inline DXGI_FORMAT GetDxPixelFormat(E_PIXEL_FORMAT InFormat)
	{
		switch (InFormat)
		{
		case EPF_A8:
			return DXGI_FORMAT_R8_UNORM;
		case EPF_RGBA8:
			return DXGI_FORMAT_R8G8B8A8_UNORM;
		case EPF_RGBA8_SRGB:
			return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		case EPF_BGRA8:
			return DXGI_FORMAT_B8G8R8A8_UNORM;
		case EPF_BGRA8_SRGB:
			return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
		case EPF_R16F:
			return DXGI_FORMAT_R16_FLOAT;
		case EPF_RG16F:
			return DXGI_FORMAT_R16G16_FLOAT;
		case EPF_RGBA16F:
			return DXGI_FORMAT_R16G16B16A16_FLOAT;
		case EPF_R32F:
			return DXGI_FORMAT_R32_FLOAT;
		case EPF_RG32F:
			return DXGI_FORMAT_R32G32_FLOAT;
		case EPF_RGB32F:
			return DXGI_FORMAT_R32G32B32_FLOAT;
		case EPF_RGBA32F:
			return DXGI_FORMAT_R32G32B32A32_FLOAT;
		case EPF_R11G11B10F:
			return DXGI_FORMAT_R11G11B10_FLOAT;
		case EPF_DEPTH16:
			return DXGI_FORMAT_D16_UNORM;
		case EPF_DEPTH32:
			return DXGI_FORMAT_D32_FLOAT;
		case EPF_DEPTH24_STENCIL8:
			return DXGI_FORMAT_D24_UNORM_S8_UINT;
		case EPF_DDS_DXT1:
			return DXGI_FORMAT_BC1_UNORM;
		case EPF_DDS_DXT1_SRGB:
			return DXGI_FORMAT_BC1_UNORM_SRGB;
		case EPF_DDS_DXT3:
			return DXGI_FORMAT_BC2_UNORM;
		case EPF_DDS_DXT3_SRGB:
			return DXGI_FORMAT_BC2_UNORM_SRGB;
		case EPF_DDS_DXT5:
			return DXGI_FORMAT_BC3_UNORM;
		case EPF_DDS_DXT5_SRGB:
			return DXGI_FORMAT_BC3_UNORM_SRGB;
		case EPF_DDS_BC5:
			return DXGI_FORMAT_BC5_UNORM;
		}
		RuntimeFail();
		return DXGI_FORMAT_UNKNOWN;
	};

	inline DXGI_FORMAT GetDxIndexFormat(E_INDEX_TYPE InFormat)
	{
		switch (InFormat)
		{
		case EIT_16BIT:
			return DXGI_FORMAT_R16_UINT;
		case EIT_32BIT:
			return DXGI_FORMAT_R32_UINT;
		default:
			RuntimeFail();
			break;
		}
		return DXGI_FORMAT_UNKNOWN;
	};

	static const DXGI_FORMAT k_MESHBUFFER_STREAM_FORMAT_MAP[ESSI_TOTAL] =
	{
		DXGI_FORMAT_R32G32B32_FLOAT,	// ESSI_POSITION,
		DXGI_FORMAT_R8G8B8A8_UNORM,		// ESSI_NORMAL,
		DXGI_FORMAT_R8G8B8A8_UNORM,	// ESSI_COLOR,
		DXGI_FORMAT_R16G16_FLOAT,	// ESSI_TEXCOORD0,
		DXGI_FORMAT_R16G16_FLOAT,	// ESSI_TEXCOORD1,
		DXGI_FORMAT_R8G8B8A8_UNORM,	// ESSI_TANGENT,
		DXGI_FORMAT_R8G8B8A8_UINT,	// ESSI_BLENDINDEX,
		DXGI_FORMAT_R8G8B8A8_UNORM,	// ESSI_BLENDWEIGHT
	};

	static const DXGI_FORMAT k_INSTANCEBUFFER_STREAM_FORMAT_MAP[EISI_TOTAL] =
	{
		DXGI_FORMAT_R32G32B32A32_FLOAT,	// EISI_TRANSFORM0,
		DXGI_FORMAT_R32G32B32A32_FLOAT,	// EISI_TRANSFORM1,
		DXGI_FORMAT_R32G32B32A32_FLOAT,	// EISI_TRANSFORM2,
		DXGI_FORMAT_R32G32B32A32_FLOAT,	// EISI_CUSTOM0,
	};

	static const D3D12_BLEND k_BLEND_FUNC_MAP[EBF_COUNT] =
	{
		D3D12_BLEND_ZERO,	//EBF_ZERO,
		D3D12_BLEND_ONE,	//EBF_ONE,
		D3D12_BLEND_SRC_COLOR,	//EBF_SRC_COLOR,
		D3D12_BLEND_INV_SRC_COLOR,	//EBF_ONE_MINUS_SRC_COLOR,
		D3D12_BLEND_DEST_COLOR,	//EBF_DEST_COLOR,
		D3D12_BLEND_INV_DEST_COLOR,	//EBF_ONE_MINUS_DEST_COLOR,
		D3D12_BLEND_SRC_ALPHA,	//EBF_SRC_ALPHA,
		D3D12_BLEND_INV_SRC_ALPHA,	//EBF_ONE_MINUS_SRC_ALPHA,
		D3D12_BLEND_DEST_ALPHA,	//EBF_DST_ALPHA,
		D3D12_BLEND_INV_DEST_ALPHA,	//EBF_ONE_MINUS_DST_ALPHA,
		D3D12_BLEND_SRC1_COLOR,	//EBF_CONSTANT_COLOR,
		D3D12_BLEND_INV_SRC1_COLOR,	//EBF_ONE_MINUS_CONSTANT_COLOR,
		D3D12_BLEND_SRC1_ALPHA,	//EBF_CONSTANT_ALPHA,
		D3D12_BLEND_INV_SRC1_ALPHA,	//EBF_ONE_MINUS_CONSTANT_ALPHA,
		D3D12_BLEND_SRC_ALPHA_SAT,	//EBF_SRC_ALPHA_SATURATE
	};

	static const D3D12_BLEND_OP k_BLEND_EQUATION_MAP[EBE_COUNT] =
	{
		D3D12_BLEND_OP_ADD,	//EBE_FUNC_ADD,
		D3D12_BLEND_OP_SUBTRACT,	//EBE_FUNC_SUBTRACT,
		D3D12_BLEND_OP_REV_SUBTRACT,	//EBE_FUNC_REVERSE_SUBTRACT,
		D3D12_BLEND_OP_MIN,	//EBE_MIN,
		D3D12_BLEND_OP_MAX,	//EBE_MAX
	};

	inline void MakeDx12BlendState(const TPipelineDesc& Desc, D3D12_BLEND_DESC& BlendState)
	{
		BlendState.AlphaToCoverageEnable = FALSE;
		BlendState.IndependentBlendEnable = FALSE;

		D3D12_RENDER_TARGET_BLEND_DESC BlendDesc;
		BlendDesc.BlendEnable = Desc.IsEnabled(EPSO_BLEND);
		BlendDesc.LogicOpEnable = FALSE;
		BlendDesc.SrcBlend = k_BLEND_FUNC_MAP[Desc.BlendState.SrcBlend];
		BlendDesc.DestBlend = k_BLEND_FUNC_MAP[Desc.BlendState.DestBlend];
		BlendDesc.BlendOp = k_BLEND_EQUATION_MAP[Desc.BlendState.BlendOp];
		BlendDesc.SrcBlendAlpha = k_BLEND_FUNC_MAP[Desc.BlendState.SrcBlendAlpha];
		BlendDesc.DestBlendAlpha = k_BLEND_FUNC_MAP[Desc.BlendState.DestBlendAlpha];
		BlendDesc.BlendOpAlpha = k_BLEND_EQUATION_MAP[Desc.BlendState.BlendOpAlpha];
		BlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
		BlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		for (int32 i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
			BlendState.RenderTarget[i] = BlendDesc;
	}

	static const D3D12_COMPARISON_FUNC k_COMPARISON_FUNC_MAP[ECF_COUNT] =
	{
		D3D12_COMPARISON_FUNC_NEVER,	//ECF_NEVER,
		D3D12_COMPARISON_FUNC_LESS,	//ECF_LESS,
		D3D12_COMPARISON_FUNC_LESS_EQUAL,	//ECF_LESS_EQUAL,
		D3D12_COMPARISON_FUNC_EQUAL,	//ECF_EQUAL,
		D3D12_COMPARISON_FUNC_GREATER,	//ECF_GREATER,
		D3D12_COMPARISON_FUNC_NOT_EQUAL,	//ECF_NOT_EQUAL,
		D3D12_COMPARISON_FUNC_GREATER_EQUAL,	//ECF_GREATER_EQUAL,
		D3D12_COMPARISON_FUNC_ALWAYS,	//ECF_ALWAYS,
	};
	static const D3D12_STENCIL_OP k_STENCIL_OP_MAP[ESO_COUNT] =
	{
		D3D12_STENCIL_OP_KEEP,	//ESO_KEEP,
		D3D12_STENCIL_OP_ZERO,	//ESO_ZERO,
		D3D12_STENCIL_OP_REPLACE,	//ESO_REPLACE,
		D3D12_STENCIL_OP_INCR_SAT,	//ESO_INCR_SAT,
		D3D12_STENCIL_OP_DECR_SAT,	//ESO_DECR_SAT,
		D3D12_STENCIL_OP_INVERT,	//ESO_INVERT,
		D3D12_STENCIL_OP_INCR,	//ESO_INCR,
		D3D12_STENCIL_OP_DECR,	//ESO_DECR,
	};

	inline void MakeDx12DepthStencilState(const TPipelineDesc& Desc, D3D12_DEPTH_STENCIL_DESC& DepthStencilState)
	{
		DepthStencilState.DepthEnable = Desc.IsEnabled(EPSO_DEPTH) || Desc.IsEnabled(EPSO_DEPTH_TEST);
		DepthStencilState.DepthWriteMask = Desc.IsEnabled(EPSO_DEPTH) ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
		DepthStencilState.DepthFunc = Desc.IsEnabled(EPSO_DEPTH_TEST) ? k_COMPARISON_FUNC_MAP[Desc.DepthStencilDesc.DepthFunc] : D3D12_COMPARISON_FUNC_ALWAYS;
		DepthStencilState.StencilEnable = Desc.IsEnabled(EPSO_STENCIL);
		DepthStencilState.StencilReadMask = Desc.DepthStencilDesc.StencilReadMask;
		DepthStencilState.StencilWriteMask = Desc.DepthStencilDesc.StencilWriteMask;

		DepthStencilState.FrontFace.StencilFailOp = k_STENCIL_OP_MAP[Desc.DepthStencilDesc.FrontFace.StencilFailOp];
		DepthStencilState.FrontFace.StencilDepthFailOp = k_STENCIL_OP_MAP[Desc.DepthStencilDesc.FrontFace.StencilDepthFailOp];
		DepthStencilState.FrontFace.StencilPassOp = k_STENCIL_OP_MAP[Desc.DepthStencilDesc.FrontFace.StencilPassOp];
		DepthStencilState.FrontFace.StencilFunc = k_COMPARISON_FUNC_MAP[Desc.DepthStencilDesc.FrontFace.StencilFunc];

		DepthStencilState.BackFace.StencilFailOp = k_STENCIL_OP_MAP[Desc.DepthStencilDesc.BackFace.StencilFailOp];
		DepthStencilState.BackFace.StencilDepthFailOp = k_STENCIL_OP_MAP[Desc.DepthStencilDesc.BackFace.StencilDepthFailOp];
		DepthStencilState.BackFace.StencilPassOp = k_STENCIL_OP_MAP[Desc.DepthStencilDesc.BackFace.StencilPassOp];
		DepthStencilState.BackFace.StencilFunc = k_COMPARISON_FUNC_MAP[Desc.DepthStencilDesc.BackFace.StencilFunc];
	}

	inline D3D12_FILL_MODE GetDx12FillMode(EFillMode FillMode)
	{
		switch (FillMode)
		{
		case EFillMode::Wireframe:
			return D3D12_FILL_MODE_WIREFRAME;
		case EFillMode::Solid:
			return D3D12_FILL_MODE_SOLID;
		}
		return D3D12_FILL_MODE_SOLID;
	}

	inline D3D12_CULL_MODE GetDx12CullMode(ECullMode CullMode)
	{
		switch (CullMode)
		{
		case ECullMode::None:
			return D3D12_CULL_MODE_NONE;
		case ECullMode::Front:
			return D3D12_CULL_MODE_FRONT;
		case ECullMode::Back:
			return D3D12_CULL_MODE_BACK;
		default:
			RuntimeFail();
			break;
		}
		return D3D12_CULL_MODE_NONE;
	}

	inline void MakeDx12RasterizerDesc(const TPipelineDesc& Desc, D3D12_RASTERIZER_DESC& RasterizerDesc)
	{
		RasterizerDesc.FillMode = GetDx12FillMode(static_cast<EFillMode>(Desc.RasterizerDesc.FillMode));
		RasterizerDesc.CullMode = GetDx12CullMode(static_cast<ECullMode>(Desc.RasterizerDesc.CullMode));
		RasterizerDesc.FrontCounterClockwise = TRUE;
		RasterizerDesc.DepthBias = Desc.RasterizerDesc.DepthBias;
		RasterizerDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
		RasterizerDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
		RasterizerDesc.DepthClipEnable = TRUE;
		RasterizerDesc.MultisampleEnable = Desc.RasterizerDesc.MultiSampleCount != 0;
		RasterizerDesc.AntialiasedLineEnable = FALSE;
		RasterizerDesc.ForcedSampleCount = 0;
		RasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	}

	inline D3D12_PRIMITIVE_TOPOLOGY_TYPE GetDx12TopologyType(EPrimitiveType PrimitiveType)
	{
		switch (PrimitiveType)
		{
		case EPrimitiveType::PointList:
			return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
		case EPrimitiveType::Lines:
			return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
		case EPrimitiveType::LineStrip:
			return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
		case EPrimitiveType::TriangleList:
			return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		case EPrimitiveType::TriangleStrip:
			return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		default:
			RuntimeFail();
			break;
		}
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
	}

	inline D3D_PRIMITIVE_TOPOLOGY GetDx12Topology(EPrimitiveType PrimitiveType)
	{
		switch (PrimitiveType)
		{
		case EPrimitiveType::PointList:
			return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
		case EPrimitiveType::Lines:
			return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
		case EPrimitiveType::LineStrip:
			return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
		case EPrimitiveType::TriangleList:
			return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		case EPrimitiveType::TriangleStrip:
			return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
		default:
			RuntimeFail();
			break;
		}
		return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
	}

	inline EResourceHeapType GetTiXHeapTypeFromDxHeap(D3D12_DESCRIPTOR_HEAP_TYPE DxHeap)
	{
		switch (DxHeap)
		{
		case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
			return EResourceHeapType::ShaderResource;
		case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
			return EResourceHeapType::Sampler;
		case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
			return EResourceHeapType::RenderTarget;
		case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
			return EResourceHeapType::DepthStencil;
		default:
			RuntimeFail();
			break;
		}
		return EResourceHeapType::ShaderResource;
	}

	inline D3D12_DESCRIPTOR_HEAP_TYPE GetDxHeapTypeFromTiXHeap(EResourceHeapType TiXHeap)
	{
		switch (TiXHeap)
		{
		case tix::EResourceHeapType::RenderTarget:
			return D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		case tix::EResourceHeapType::DepthStencil:
			return D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		case tix::EResourceHeapType::Sampler:
			return D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
		case tix::EResourceHeapType::ShaderResource:
			return D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		default:
			RuntimeFail();
			break;
		}
		return D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	}

	static const D3D12_FILTER TiX2DxTextureFilterMap[ETFT_COUNT] =
	{
		D3D12_FILTER_MIN_MAG_MIP_POINT,	//ETFT_MINMAG_NEAREST_MIP_NEAREST = 0,
		D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT,	//ETFT_MINMAG_LINEAR_MIP_NEAREST,
		D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR,	//ETFT_MINMAG_NEAREST_MIPMAP_LINEAR,
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,	//ETFT_MINMAG_LINEAR_MIPMAP_LINEAR,
	};

	inline D3D12_FILTER GetDxTextureFilterFromTiX(E_TEXTURE_FILTER_TYPE Filter)
	{
		return TiX2DxTextureFilterMap[Filter];
	}

	static const D3D12_TEXTURE_ADDRESS_MODE TiX2DxTextureAddressMode[ETC_COUNT] =
	{
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,	//ETC_REPEAT = 0,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,	//ETC_CLAMP_TO_EDGE,
		D3D12_TEXTURE_ADDRESS_MODE_MIRROR,	//ETC_MIRROR,
	};

	inline D3D12_TEXTURE_ADDRESS_MODE GetDxTextureAddressModeFromTiX(E_TEXTURE_ADDRESS_MODE AddressMode)
	{
		return TiX2DxTextureAddressMode[AddressMode];
	}

	static const D3D12_SHADER_VISIBILITY TiX2DxShaderStage[ESS_COUNT] = 
	{
		D3D12_SHADER_VISIBILITY_VERTEX,	//ESS_VERTEX_SHADER,
		D3D12_SHADER_VISIBILITY_PIXEL,	//ESS_PIXEL_SHADER,
		D3D12_SHADER_VISIBILITY_DOMAIN,	//ESS_DOMAIN_SHADER,
		D3D12_SHADER_VISIBILITY_HULL,	//ESS_HULL_SHADER,
		D3D12_SHADER_VISIBILITY_GEOMETRY,	//ESS_GEOMETRY_SHADER,
	};

	inline D3D12_SHADER_VISIBILITY GetDxShaderVisibilityFromTiX(E_SHADER_STAGE ShaderStage)
	{
		return TiX2DxShaderStage[ShaderStage];
	}


	inline D3D12_RESOURCE_STATES GetDx12ResourceState(EGPUResourceState State)
	{
		switch (State)
		{
		case EGPUResourceState::Common:
			return D3D12_RESOURCE_STATE_COMMON;
		case EGPUResourceState::VertexAndConstantBuffer:
			return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		case EGPUResourceState::IndexBuffer:
			return D3D12_RESOURCE_STATE_INDEX_BUFFER;
		case EGPUResourceState::RenderTarget:
			return D3D12_RESOURCE_STATE_RENDER_TARGET;
		case EGPUResourceState::UnorderedAccess:
			return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		case EGPUResourceState::DepthWrite:
			return D3D12_RESOURCE_STATE_DEPTH_WRITE;
		case EGPUResourceState::DepthRead:
			return D3D12_RESOURCE_STATE_DEPTH_READ;
		case EGPUResourceState::NonPixelShaderResource:
			return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		case EGPUResourceState::PixelShaderResource:
			return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		case EGPUResourceState::StreamOut:
			return D3D12_RESOURCE_STATE_STREAM_OUT;
		case EGPUResourceState::IndirectArgument:
			return D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
		case EGPUResourceState::CopyDest:
			return D3D12_RESOURCE_STATE_COPY_DEST;
		case EGPUResourceState::CopySource:
			return D3D12_RESOURCE_STATE_COPY_SOURCE;
		case EGPUResourceState::AccelerationStructure:
			return D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
		case EGPUResourceState::GenericRead:
			return D3D12_RESOURCE_STATE_GENERIC_READ;
		default:
			RuntimeFail();
			break;
		}
		return D3D12_RESOURCE_STATE_COMMON;
	};

	inline D3D12_RESOURCE_DIMENSION GetDx12TextureType(E_TEXTURE_TYPE Type)
	{
		switch (Type)
		{
		case tix::ETT_TEXTURE_1D:
			return D3D12_RESOURCE_DIMENSION_TEXTURE1D;
		case tix::ETT_TEXTURE_2D:
			return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		case tix::ETT_TEXTURE_3D:
			return D3D12_RESOURCE_DIMENSION_TEXTURE3D;
		case tix::ETT_TEXTURE_CUBE:
			return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		case tix::ETT_TEXTURE_UNKNOWN:
			return D3D12_RESOURCE_DIMENSION_UNKNOWN;
		default:
			RuntimeFail();
			break;
		}
		return D3D12_RESOURCE_DIMENSION_UNKNOWN;
	}

	inline DXGI_FORMAT GetBaseFormat(DXGI_FORMAT DefaultFormat)
	{
		switch (DefaultFormat)
		{
		case DXGI_FORMAT_R8G8B8A8_UNORM:
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
			return DXGI_FORMAT_R8G8B8A8_TYPELESS;

		case DXGI_FORMAT_B8G8R8A8_UNORM:
		case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
			return DXGI_FORMAT_B8G8R8A8_TYPELESS;

		case DXGI_FORMAT_B8G8R8X8_UNORM:
		case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
			return DXGI_FORMAT_B8G8R8X8_TYPELESS;

			// 32-bit Z w/ Stencil
		case DXGI_FORMAT_R32G8X24_TYPELESS:
		case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
		case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
		case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
			return DXGI_FORMAT_R32G8X24_TYPELESS;

			// No Stencil
		case DXGI_FORMAT_R32_TYPELESS:
		case DXGI_FORMAT_D32_FLOAT:
		case DXGI_FORMAT_R32_FLOAT:
			return DXGI_FORMAT_R32_TYPELESS;

			// 24-bit Z
		case DXGI_FORMAT_R24G8_TYPELESS:
		case DXGI_FORMAT_D24_UNORM_S8_UINT:
		case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
		case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
			return DXGI_FORMAT_R24G8_TYPELESS;

			// 16-bit Z w/o Stencil
		case DXGI_FORMAT_R16_TYPELESS:
		case DXGI_FORMAT_D16_UNORM:
		case DXGI_FORMAT_R16_UNORM:
			return DXGI_FORMAT_R16_TYPELESS;

		default:
			return DefaultFormat;
		}
	}

	inline DXGI_FORMAT GetDSVFormat(DXGI_FORMAT DefaultFormat)
	{
		switch (DefaultFormat)
		{
			// 32-bit Z w/ Stencil
		case DXGI_FORMAT_R32G8X24_TYPELESS:
		case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
		case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
		case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
			return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;

			// No Stencil
		case DXGI_FORMAT_R32_TYPELESS:
		case DXGI_FORMAT_D32_FLOAT:
		case DXGI_FORMAT_R32_FLOAT:
			return DXGI_FORMAT_D32_FLOAT;

			// 24-bit Z
		case DXGI_FORMAT_R24G8_TYPELESS:
		case DXGI_FORMAT_D24_UNORM_S8_UINT:
		case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
		case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
			return DXGI_FORMAT_D24_UNORM_S8_UINT;

			// 16-bit Z w/o Stencil
		case DXGI_FORMAT_R16_TYPELESS:
		case DXGI_FORMAT_D16_UNORM:
		case DXGI_FORMAT_R16_UNORM:
			return DXGI_FORMAT_D16_UNORM;

		default:
			return DefaultFormat;
		}
	}
	inline DXGI_FORMAT GetDepthSrvFormat(DXGI_FORMAT DefaultFormat)
	{
		switch (DefaultFormat)
		{
			// 32-bit Z w/ Stencil
		case DXGI_FORMAT_R32G8X24_TYPELESS:
		case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
		case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
		case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
			return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;

			// No Stencil
		case DXGI_FORMAT_R32_TYPELESS:
		case DXGI_FORMAT_D32_FLOAT:
		case DXGI_FORMAT_R32_FLOAT:
			return DXGI_FORMAT_R32_FLOAT;

			// 24-bit Z
		case DXGI_FORMAT_R24G8_TYPELESS:
		case DXGI_FORMAT_D24_UNORM_S8_UINT:
		case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
		case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
			return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;

			// 16-bit Z w/o Stencil
		case DXGI_FORMAT_R16_TYPELESS:
		case DXGI_FORMAT_D16_UNORM:
		case DXGI_FORMAT_R16_UNORM:
			return DXGI_FORMAT_R16_UNORM;

		default:
			return DXGI_FORMAT_UNKNOWN;
		}
	}
	inline DXGI_FORMAT GetUAVFormat(DXGI_FORMAT DefaultFormat)
	{
		switch (DefaultFormat)
		{
		case DXGI_FORMAT_R8G8B8A8_TYPELESS:
		case DXGI_FORMAT_R8G8B8A8_UNORM:
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
			return DXGI_FORMAT_R8G8B8A8_UNORM;

		case DXGI_FORMAT_B8G8R8A8_TYPELESS:
		case DXGI_FORMAT_B8G8R8A8_UNORM:
		case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
			return DXGI_FORMAT_B8G8R8A8_UNORM;

		case DXGI_FORMAT_B8G8R8X8_TYPELESS:
		case DXGI_FORMAT_B8G8R8X8_UNORM:
		case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
			return DXGI_FORMAT_B8G8R8X8_UNORM;

		case DXGI_FORMAT_R32_TYPELESS:
		case DXGI_FORMAT_R32_FLOAT:
			return DXGI_FORMAT_R32_FLOAT;

#if defined (TIX_DEBUG)
		case DXGI_FORMAT_R32G8X24_TYPELESS:
		case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
		case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
		case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
		case DXGI_FORMAT_D32_FLOAT:
		case DXGI_FORMAT_R24G8_TYPELESS:
		case DXGI_FORMAT_D24_UNORM_S8_UINT:
		case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
		case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
		case DXGI_FORMAT_D16_UNORM:

			_LOG(ELog::Fatal, "Requested a UAV format for a depth stencil format.\n");
#endif

		default:
			return DefaultFormat;
		}
	}
}
#endif	// COMPILE_WITH_RHI_DX12