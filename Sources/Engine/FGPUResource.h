/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	enum class EGPUResourceFlag : uint32
	{
		None = 0,
		Uav = 1 << 0,
		UavCounter = 1 << 1,
		Readback = 1 << 2,

		// For metal, do not create buffer (usually less than 4k bytes), bind raw memory to gpu as apple doc recommended.
		// https://developer.apple.com/library/archive/documentation/3DDrawing/Conceptual/MTLBestPracticesGuide/BufferBindings.html
		// For Dx12, use a D3D12_HEAP_TYPE_UPLOAD heap to manage data directly
		Intermediate = 1 << 3,

		// For texture resources
		ColorBuffer = 1 << 4,
		DsBuffer = 1 << 5,

		// Used for iOS Metal
		MemoryLess = 1 << 6,
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

	struct FGPUBufferDesc
	{
		uint32 Flag;
		uint32 BufferSize;

		FGPUBufferDesc()
			: Flag(0)
			, BufferSize(0)
		{}
	};

	struct FGPUTextureDesc
	{
		uint32 Flag;
		TTextureDesc Texture;

		FGPUTextureDesc()
			: Flag(0)
		{}
	};

	class FGPUResource : public IReferenceCounted
	{
	public:
		FGPUResource()
		{}
		virtual ~FGPUResource()
		{}

	protected:

	protected:
	};

	/////////////////////////////////////////////////////////////
	class FGPUBuffer : public FGPUResource
	{
	public:
		FGPUBuffer()
		{}
		virtual ~FGPUBuffer()
		{}

		virtual void Init(const FGPUBufferDesc & Desc, TStreamPtr Data) = 0;
	};

	/////////////////////////////////////////////////////////////
	class FGPUTexture : public FGPUResource
	{
	public:
		FGPUTexture()
		{}
		virtual ~FGPUTexture()
		{}

		virtual void Init(const FGPUTextureDesc& Desc, const TVector<TImagePtr>& Data) = 0;
	};
}
