/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	enum class EGPUResourceFlag : uint8
	{
		None = 0,
		Uav = 1 << 0,
		UavWithCounter = 1 << 1,
		Readback = 1 << 2,

		// For metal, do not create buffer (usually less than 4k bytes), bind raw memory to gpu as apple doc recommended.
		// https://developer.apple.com/library/archive/documentation/3DDrawing/Conceptual/MTLBestPracticesGuide/BufferBindings.html
		// For Dx12, use a D3D12_HEAP_TYPE_UPLOAD heap to manage data directly
		Intermediate = 1 << 3,
	};

	enum class EGPUResourceState : uint8
	{
		Common,
		VertexAndConstantBuffer,
		IndexBuffer,
		RenderTarget,
		UnorderedAccess,
		DepthWrite,
		DepthRead,
		NonPixelShaderResource,
		PixelShaderResource,
		StreamOut,
		IndirectArgument,
		CopyDest,
		CopySource,
		AccelerationStructure,
		GenericRead,
	};

	struct FGPUResourceDesc
	{
		uint32 Flag;
		uint32 BufferSize;
	};

	class FGPUResource : public IReferenceCounted
	{
	public:
		FGPUResource()
		{}
		virtual ~FGPUResource()
		{}

		virtual void Init(const FGPUResourceDesc& Desc, TStreamPtr Data) = 0;

	protected:

	protected:
	};

	/////////////////////////////////////////////////////////////
	class FGPUResourceBuffer : public FGPUResource
	{
	public:
		FGPUResourceBuffer()
		{}
		virtual ~FGPUResourceBuffer()
		{}
	};

	/////////////////////////////////////////////////////////////
	class FGPUResourceTexture : public FGPUResource
	{
	public:
		FGPUResourceTexture()
		{}
		virtual ~FGPUResourceTexture()
		{}
	};
}
