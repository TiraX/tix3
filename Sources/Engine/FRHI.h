/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

#include "FRHIConfig.h"
#include "FViewport.h"
#include "FRenderResourceHeap.h"

namespace tix
{
	enum E_RHI_TYPE
	{
		ERHI_DX12 = 0,
		ERHI_METAL = 1,

		ERHI_NUM,
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

	class FFrameResources;
	// Render hardware interface
	class FRHI
	{
	public: 
		TI_API static FRHI* Get();
		static FRHI* CreateRHI(const TString& InRHIName);
		static void ReleaseRHI();

		static FRHIConfig RHIConfig;

		static uint32 GetGPUFrames()
		{
			return NumGPUFrames;
		}

		virtual FRHI* CreateAsyncRHI(const TString& InRHIName) = 0;
		virtual void InitRHI() = 0;
		virtual void BeginFrame();
		virtual void EndFrame() = 0;
        virtual void BeginRenderToFrameBuffer() {};

		virtual void BeginEvent(const int8* InEventName) = 0;
		virtual void BeginEvent(const int8* InEventName, int32 Index) = 0;
		virtual void EndEvent() = 0;

		virtual int32 GetCurrentEncodingFrameIndex() = 0;
		virtual void WaitingForGpu() = 0;

		// Create GPU Resource
		virtual FGPUBufferPtr CreateGPUBuffer() = 0;
		virtual FGPUTexturePtr CreateGPUTexture() = 0;

		// Create RTX related resources
		virtual FShaderPtr CreateRtxShaderLib(const TString& ShaderLibName) = 0;
		virtual FRtxPipelinePtr CreateRtxPipeline(FShaderPtr InShader) = 0;
		virtual FTopLevelAccelerationStructurePtr CreateTopLevelAccelerationStructure() = 0;
		virtual FBottomLevelAccelerationStructurePtr CreateBottomLevelAccelerationStructure() = 0;

		// Create Graphics and Compute related resources
		virtual FPipelinePtr CreatePipeline(FShaderPtr InShader) = 0;
		virtual FRenderTargetPtr CreateRenderTarget(int32 W, int32 H) = 0;
		virtual FRenderResourceTablePtr CreateRenderResourceTable(uint32 InSize, EResourceHeapType InHeap);
		virtual FShaderPtr CreateShader(const TShaderNames& InNames, E_SHADER_TYPE Type) = 0;
		virtual FShaderPtr CreateComputeShader(const TString& InComputeShaderName) = 0;
		virtual FArgumentBufferPtr CreateArgumentBuffer(int32 ReservedSlots) = 0;
		virtual FGPUCommandSignaturePtr CreateGPUCommandSignature(FPipelinePtr Pipeline, const TVector<E_GPU_COMMAND_TYPE>& CommandStructure) = 0;
		virtual FGPUCommandBufferPtr CreateGPUCommandBuffer(FGPUCommandSignaturePtr GPUCommandSignature, uint32 CommandsCount, uint32 Flag = 0) = 0;

		// RTX
		virtual bool UpdateHardwareResourceRtxPL(FRtxPipelinePtr Pipeline, TRtxPipelinePtr InPipelineDesc) = 0;
		virtual void SetRtxPipeline(FRtxPipelinePtr RtxPipeline) = 0;
		virtual void TraceRays(FRtxPipelinePtr RtxPipeline, const FInt3& Size) = 0;

		// Graphics and Compute
		virtual bool UpdateHardwareResourceGraphicsPipeline(FPipelinePtr Pipeline, const TPipelineDesc& Desc) = 0;
		virtual bool UpdateHardwareResourceComputePipeline(FPipelinePtr Pipeline) = 0;
        virtual bool UpdateHardwareResourceTilePL(FPipelinePtr Pipeline, TTilePipelinePtr InTilePipelineDesc) = 0;
		virtual bool UpdateHardwareResourceRT(FRenderTargetPtr RenderTarget) = 0;
		virtual bool UpdateHardwareResourceShader(FShaderPtr ShaderResource, const TVector<TStreamPtr>& ShaderCodes) = 0;
		virtual bool UpdateHardwareResourceAB(FArgumentBufferPtr ArgumentBuffer, FShaderPtr InShader, int32 SpecifiedBindingIndex = -1) = 0;
		virtual bool UpdateHardwareResourceGPUCommandSig(FGPUCommandSignaturePtr GPUCommandSignature) = 0;

		virtual TStreamPtr ReadGPUBufferToCPU(FGPUBufferPtr GPUBuffer) = 0;

		virtual void CopyTextureRegion(FGPUBufferPtr DstBuffer, FGPUTexturePtr SrcTexture, uint32 RowPitch) = 0;
		virtual void CopyGPUBuffer(FGPUBufferPtr DstBuffer, FGPUBufferPtr SrcBuffer) = 0;

		virtual void PutConstantBufferInHeap(FUniformBufferPtr InUniformBuffer, EResourceHeapType InHeapType, uint32 InHeapSlot) = 0;
		virtual void PutTextureInHeap(FTexturePtr InTexture, EResourceHeapType InHeapType, uint32 InHeapSlot) = 0;
		virtual void PutRWTextureInHeap(FTexturePtr InTexture, uint32 InMipLevel, EResourceHeapType InHeapType, uint32 InHeapSlot) = 0;
		virtual void PutUniformBufferInHeap(FUniformBufferPtr InBuffer, EResourceHeapType InHeapType, uint32 InHeapSlot) = 0;
		virtual void PutRWUniformBufferInHeap(FUniformBufferPtr InBuffer, EResourceHeapType InHeapType, uint32 InHeapSlot) = 0;
		virtual void PutVertexBufferInHeap(FVertexBufferPtr InBuffer, EResourceHeapType InHeapType, int32 InVBHeapSlot) = 0;
		virtual void PutIndexBufferInHeap(FIndexBufferPtr InBuffer, EResourceHeapType InHeapType, int32 InIBHeapSlot) = 0;
		virtual void PutInstanceBufferInHeap(FInstanceBufferPtr InBuffer, EResourceHeapType InHeapType, uint32 InHeapSlot) = 0;
		virtual void PutRTColorInHeap(FTexturePtr InTexture, uint32 InHeapSlot) = 0;
		virtual void PutRTDepthInHeap(FTexturePtr InTexture, uint32 InHeapSlot) = 0;
		virtual void PutTopAccelerationStructureInHeap(FTopLevelAccelerationStructurePtr InTLAS, EResourceHeapType InHeapType, uint32 InHeapSlot) = 0;

		// Graphics
		virtual void SetGraphicsPipeline(FPipelinePtr InPipeline) = 0;
		virtual void SetVertexBuffer(FVertexBufferPtr InVertexBuffer, FInstanceBufferPtr InInstanceBuffer) = 0;
		virtual void SetIndexBuffer(FIndexBufferPtr InIndexBuffer) = 0;

		virtual void SetUniformBuffer(E_SHADER_STAGE ShaderStage, int32 BindIndex, FUniformBufferPtr InUniformBuffer) = 0;
		virtual void SetRenderResourceTable(int32 BindIndex, FRenderResourceTablePtr RenderResourceTable) = 0;
		virtual void SetArgumentBuffer(int32 BindIndex, FArgumentBufferPtr InArgumentBuffer) = 0;

		virtual void SetGPUBufferName(FGPUBufferPtr GPUBuffer, const TString& Name) = 0;
		virtual void SetGPUTextureName(FGPUTexturePtr GPUTexture, const TString& Name) = 0;
		virtual void SetGPUBufferState(FGPUBufferPtr GPUBuffer, EGPUResourceState NewState) = 0;
		virtual void SetGPUTextureState(FGPUTexturePtr GPUTexture, EGPUResourceState NewState) = 0;
		virtual void FlushResourceStateChange() = 0;

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
		virtual void SetComputeConstant(int32 BindIndex, const FUInt4& InValue) = 0;
		virtual void SetComputeConstant(int32 BindIndex, const FFloat4& InValue) = 0;
		virtual void SetComputeConstantBuffer(int32 BindIndex, FUniformBufferPtr InUniformBuffer, uint32 BufferOffset = 0) = 0;
		virtual void SetComputeShaderResource(int32 BindIndex, FUniformBufferPtr InUniformBuffer, uint32 BufferOffset = 0) = 0;
		virtual void SetComputeResourceTable(int32 BindIndex, FRenderResourceTablePtr RenderResourceTable) = 0;
        virtual void SetComputeArgumentBuffer(int32 BindIndex, FArgumentBufferPtr InArgumentBuffer) = 0;
        virtual void SetComputeTexture(int32 BindIndex, FTexturePtr InTexture) = 0;
        
		virtual void DispatchCompute(const FInt3& GroupSize, const FInt3& GroupCount) = 0;

		// GPU Command buffer
		virtual void ExecuteGPUDrawCommands(FGPUCommandBufferPtr GPUCommandBuffer) = 0;
		virtual void ExecuteGPUComputeCommands(FGPUCommandBufferPtr GPUCommandBuffer) = 0;

		virtual void SetViewport(const FViewport& InViewport);
		virtual void BeginRenderToRenderTarget(FRenderTargetPtr RT, const int8* PassName = "UnnamedPass", uint32 MipLevel = 0);

		E_RHI_TYPE GetRHIType() const
		{
			return RHIType;
		}

		const FViewport& GetViewport() const
		{
			return Viewport;
		}

		FRenderResourceHeap& GetRenderResourceHeap(EResourceHeapType Type)
		{
			int32 Index = static_cast<int32>(Type);
			TI_ASSERT(Index >= 0 && Index < NumResourceHeapTypes);
			return RenderResourceHeap[Index];
		}

	protected:
		static FRHI* RHI;
		FRHI(E_RHI_TYPE InRHIType, const TString& InRHIName);
		virtual ~FRHI();

		virtual void FeatureCheck() = 0;
		virtual void SupportFeature(E_RHI_FEATURE InFeature);
		
		static void GPUFrameDone()
		{
			++NumGPUFrames;
		}
		// Frames count that GPU done
		static TI_API uint32 NumGPUFrames;
	protected:
		E_RHI_TYPE RHIType;
		TString RHIName;
		FViewport Viewport;
		FFrameResources * FrameResources[FRHIConfig::FrameBufferNum];

		FRenderTargetPtr CurrentRenderTarget;
		FViewport RtViewport;

		FRenderResourceHeap RenderResourceHeap[NumResourceHeapTypes];
		FBoundResource CurrentBoundResource;
	};
}
