/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	enum E_RESOURCE_FAMILY
	{
		ERF_Dx12,
		ERF_Metal,
		ERF_Vulkan
	};

	enum E_INDEX_TYPE 
	{
		EIT_16BIT,
		EIT_32BIT,
		
		EIT_COUNT,
	};

	enum E_MESH_STREAM_INDEX
	{
		ESSI_POSITION		= 0,
		ESSI_NORMAL,
		ESSI_COLOR,
		ESSI_TEXCOORD0,
		ESSI_TEXCOORD1,
		ESSI_TANGENT,
		ESSI_BLENDINDEX,
		ESSI_BLENDWEIGHT,

		ESSI_TOTAL,
	};

	enum E_VERTEX_STREAM_SEGMENT
	{
		EVSSEG_POSITION = 1,
		EVSSEG_NORMAL = EVSSEG_POSITION << 1,
		EVSSEG_COLOR = EVSSEG_NORMAL << 1,
		EVSSEG_TEXCOORD0 = EVSSEG_COLOR << 1,
		EVSSEG_TEXCOORD1 = EVSSEG_TEXCOORD0 << 1,
		EVSSEG_TANGENT = EVSSEG_TEXCOORD1 << 1,
		EVSSEG_BLENDINDEX = EVSSEG_TANGENT << 1,
		EVSSEG_BLENDWEIGHT = EVSSEG_BLENDINDEX << 1,

		EVSSEG_TOTAL = EVSSEG_BLENDWEIGHT,
	};

	// Instance transform with a FMat3x4
	enum E_INSTANCE_STREAM_INDEX
	{
		EISI_TRANSFORM0,
		EISI_TRANSFORM1,
		EISI_TRANSFORM2,
		EISI_CUSTOM0,

		EISI_TOTAL,
	};

	enum E_INSTANCE_STREAM_SEGMENT
	{
		EINSSEG_TRANSFORM0 = 1,
		EINSSEG_TRANSFORM1 = EINSSEG_TRANSFORM0 << 1,
		EINSSEG_TRANSFORM2 = EINSSEG_TRANSFORM1 << 1,
		EINSSEG_CUSTOM0 = EINSSEG_TRANSFORM2 << 1,

		EINSSEG_TRANSFORM_ALL = EINSSEG_TRANSFORM0 | EINSSEG_TRANSFORM1 | EINSSEG_TRANSFORM2,

		EINSSEG_TOTAL = EINSSEG_CUSTOM0,
	};

	enum class EPrimitiveType : int8
	{
		Invalid = -1,
		PointList = 0,
		Lines,
		LineStrip,
		TriangleList,
		TriangleStrip
	};

	enum class EResourceHeapType : int8
	{
		RenderTarget = 0,
		DepthStencil,
		Sampler,
		ShaderResource,	// Constants, Buffers, Textures and UAV

		Count,
	};
	static const int32 NumResourceHeapTypes = static_cast<int32>(EResourceHeapType::Count);
}
