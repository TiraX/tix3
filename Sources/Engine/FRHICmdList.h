/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

#include "FRHIConfig.h"

namespace tix
{
	enum class ERHICmdList
	{
		Direct,
		Compute,
		Copy
	};

	struct FBoundResource
	{
		FBoundResource()
			: PrimitiveType(EPrimitiveType::Invalid)
		{}

		EPrimitiveType PrimitiveType;
		FPipelinePtr Pipeline;
		FShaderBindingPtr ShaderBinding;

		void Reset()
		{
			PrimitiveType = EPrimitiveType::Invalid;
			Pipeline = nullptr;
			ShaderBinding = nullptr;
		}
	};

	class FRHICmdList
	{
	public:
		virtual ~FRHICmdList();

		virtual void BeginCmdList() = 0;
		virtual void EndCmdList() = 0;
		virtual void AddHeap(FRHIHeap* InHeap) = 0;
		virtual void Close() = 0;
		virtual void Execute() = 0;
		virtual void WaitingForGpu() = 0;
		virtual void MoveToNextFrame() = 0;

		virtual void BeginEvent(const int8 * InEventName) = 0;
		virtual void BeginEvent(const int8 * InEventName, int32 Index) = 0;
		virtual void EndEvent() = 0;

		virtual void CopyTextureRegion(FGPUBufferPtr DstBuffer, FGPUTexturePtr SrcTexture, uint32 RowPitch) = 0;
		virtual void CopyGPUBuffer(FGPUBufferPtr DstBuffer, FGPUBufferPtr SrcBuffer) = 0;

		// Graphics
		virtual void SetGraphicsPipeline(FPipelinePtr InPipeline) = 0;
		virtual void SetVertexBuffer(FVertexBufferPtr InVertexBuffer, FInstanceBufferPtr InInstanceBuffer) = 0;
		virtual void SetIndexBuffer(FIndexBufferPtr InIndexBuffer) = 0;

		virtual void SetUniformBuffer(E_SHADER_STAGE ShaderStage, int32 BindIndex, FUniformBufferPtr InUniformBuffer) = 0;
		virtual void SetRenderResourceTable(int32 BindIndex, FRenderResourceTablePtr RenderResourceTable) = 0;
		virtual void SetArgumentBuffer(int32 BindIndex, FArgumentBufferPtr InArgumentBuffer) = 0;

		virtual void SetGPUBufferState(FGPUBufferPtr GPUBuffer, EGPUResourceState NewState) = 0;
		virtual void SetGPUTextureState(FGPUTexturePtr GPUTexture, EGPUResourceState NewState) = 0;
		virtual void FlushBarriers() = 0;

		virtual void SetStencilRef(uint32 InRefValue) = 0;
		virtual void DrawPrimitiveInstanced(uint32 VertexCount, uint32 InstanceCount, uint32 InstanceOffset) = 0;
		virtual void DrawPrimitiveIndexedInstanced(uint32 IndexCount, uint32 InstanceCount, uint32 InstanceOffset) = 0;
		virtual void DrawPrimitiveIndexedInstanced(uint32 IndexCount, uint32 InstanceCount, uint32 IndexOffset, uint32 InstanceOffset) = 0;

		// Tile
		virtual void SetTilePipeline(FPipelinePtr InPipeline) = 0;
		virtual void SetTileBuffer(int32 BindIndex, FUniformBufferPtr InUniformBuffer) = 0;
		virtual void DispatchTile(const FInt3& GroupSize) = 0;

		// Compute
		virtual void SetComputePipeline(FPipelinePtr InPipeline) = 0;
		virtual void SetComputeConstant(int32 BindIndex, const void* ConstantData, int32 Size32Bit) = 0;
		virtual void SetComputeConstant(int32 BindIndex, const FUInt4& InValue) = 0;
		virtual void SetComputeConstant(int32 BindIndex, const FFloat4& InValue) = 0;
		virtual void SetComputeConstantBuffer(int32 BindIndex, FUniformBufferPtr InUniformBuffer, uint32 BufferOffset = 0) = 0;
		virtual void SetComputeShaderResource(int32 BindIndex, FUniformBufferPtr InUniformBuffer, uint32 BufferOffset = 0) = 0;
		virtual void SetComputeUnorderedAccessResource(int32 BindIndex, FUniformBufferPtr InUniformBuffer, uint32 BufferOffset = 0) = 0;
		virtual void SetComputeResourceTable(int32 BindIndex, FRenderResourceTablePtr RenderResourceTable) = 0;
		virtual void SetComputeArgumentBuffer(int32 BindIndex, FArgumentBufferPtr InArgumentBuffer) = 0;
		virtual void SetComputeTexture(int32 BindIndex, FTexturePtr InTexture) = 0;

		virtual void DispatchCompute(const FInt3& GroupSize, const FInt3& GroupCount) = 0;

		// RTX
		virtual void SetRtxPipeline(FRtxPipelinePtr RtxPipeline) = 0;
		virtual void TraceRays(FRtxPipelinePtr RtxPipeline, const FInt3& Size) = 0;

		// GPU Command buffer
		virtual void ExecuteGPUDrawCommands(FGPUCommandBufferPtr GPUCommandBuffer) = 0;
		virtual void ExecuteGPUComputeCommands(FGPUCommandBufferPtr GPUCommandBuffer) = 0;

		virtual void SetViewport(const FRecti& InViewport);
		virtual void SetScissorRect(const FRecti& InRect);
		virtual void BeginRenderToRenderTarget(FRenderTargetPtr RT, const int8* PassName = "UnnamedPass", uint32 MipLevel = 0);

		virtual void UAVBarrier(FBottomLevelAccelerationStructurePtr BLAS) = 0;

		virtual void HoldResourceReference(FRenderResourcePtr InResource) = 0;

		const FRecti& GetViewport() const
		{
			return Viewport;
		}
#ifdef TIX_DEBUG
		TThreadId WorkingThread;
#endif // TIX_DEBUG

	protected:
		FRHICmdList(ERHICmdList InType);

	protected:
		ERHICmdList Type;
		FRecti Viewport;
		FRecti ScissorRect;
		FRenderTargetPtr CurrentRenderTarget;
		FBoundResource CurrentBoundResource;
	};
}
