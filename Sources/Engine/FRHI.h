/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

#include "FRHIConfig.h"
#include "FRHICmdList.h"

namespace tix
{
	enum class ERHIType
	{
		Dx12,
		Metal
	};

	// Render hardware interface
	class FRHI
	{
	public: 
		TI_API static FRHI* Get();
		static FRHI* CreateRHI();
		static void ReleaseRHI();

		static FRHIConfig RHIConfig;

		static uint32 GetGPUFrames()
		{
			return NumGPUFrames;
		}

		virtual void InitRHI() = 0;
		virtual void BeginFrame();
		virtual void EndFrame() = 0;
		virtual void BeginRenderToFrameBuffer() {};

		virtual int32 GetCurrentEncodingFrameIndex() = 0;
		virtual void WaitingForGpu() = 0;

		virtual TStreamPtr CompileShader(const TString& InCode, const TString& Entry, const TString& Target, const TString& IncludePath, bool Debug = false) = 0;

		// Create Command List
		virtual FRHICmdList* CreateRHICommandList(
			ERHICmdList Type,
			const TString& InNamePrefix,
			int32 BufferCount) = 0;
		// Create Resource Heap
		virtual FRHIHeap* CreateHeap(EResourceHeapType Type) = 0;

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
		virtual FShaderPtr CreateShader(const TShaderNames& InNames, E_SHADER_TYPE Type) = 0;
		virtual FShaderPtr CreateComputeShader(const TString& InComputeShaderName) = 0;
		virtual FArgumentBufferPtr CreateArgumentBuffer(int32 ReservedSlots) = 0;
		virtual FGPUCommandSignaturePtr CreateGPUCommandSignature(FPipelinePtr Pipeline, const TVector<E_GPU_COMMAND_TYPE>& CommandStructure) = 0;

		// RTX
		virtual bool UpdateHardwareResourceRtxPL(FRtxPipelinePtr Pipeline, TRtxPipelinePtr InPipelineDesc) = 0;

		// Graphics and Compute
		virtual bool UpdateHardwareResourceGraphicsPipeline(FPipelinePtr Pipeline, const TPipelineDesc& Desc) = 0;
		virtual bool UpdateHardwareResourceComputePipeline(FPipelinePtr Pipeline) = 0;
		virtual bool UpdateHardwareResourceTilePL(FPipelinePtr Pipeline, TTilePipelinePtr InTilePipelineDesc) = 0;
		virtual bool UpdateHardwareResourceRT(FRenderTargetPtr RenderTarget) = 0;
		virtual bool UpdateHardwareResourceShader(FShaderPtr ShaderResource, const TVector<TStreamPtr>& ShaderCodes) = 0;
		virtual bool UpdateHardwareResourceAB(FArgumentBufferPtr ArgumentBuffer, FShaderPtr InShader, int32 SpecifiedBindingIndex = -1) = 0;
		virtual bool UpdateHardwareResourceGPUCommandSig(FGPUCommandSignaturePtr GPUCommandSignature) = 0;

		virtual TStreamPtr ReadGPUBufferToCPU(FGPUBufferPtr GPUBuffer) = 0;

		virtual void PutConstantBufferInTable(FRenderResourceTablePtr RRTable, FUniformBufferPtr InUniformBuffer, uint32 InTableSlot) = 0;
		virtual void PutTextureInTable(FRenderResourceTablePtr RRTable, FTexturePtr InTexture, uint32 InTableSlot) = 0;
		virtual void PutRWTextureInTable(FRenderResourceTablePtr RRTable, FTexturePtr InTexture, uint32 InMipLevel, uint32 InTableSlot) = 0;
		virtual void PutUniformBufferInTable(FRenderResourceTablePtr RRTable, FUniformBufferPtr InBuffer, uint32 InTableSlot) = 0;
		virtual void PutRWUniformBufferInTable(FRenderResourceTablePtr RRTable, FUniformBufferPtr InBuffer, uint32 InTableSlot) = 0;
		virtual void PutRTColorInTable(FRenderResourceTablePtr RRTable, FTexturePtr InTexture, uint32 InTableSlot) = 0;
		virtual void PutRTDepthInTable(FRenderResourceTablePtr RRTable, FTexturePtr InTexture, uint32 InTableSlot) = 0;
		virtual void PutTopAccelerationStructureInTable(FRenderResourceTablePtr RRTable, FTopLevelAccelerationStructurePtr InTLAS, uint32 InTableSlot) = 0;

		virtual void SetGPUBufferName(FGPUBufferPtr GPUBuffer, const TString& Name) = 0;
		virtual void SetGPUTextureName(FGPUTexturePtr GPUTexture, const TString& Name) = 0;


		ERHIType GetRHIType() const
		{
			return RHIType;
		}

		virtual FRHICmdList* GetDefaultCmdList()
		{
			return nullptr;
		}
		virtual FRHIHeap* GetHeapById(uint32 HeapId)
		{
			return nullptr;
		}
		virtual FRHIHeap* GetDefaultHeapRtv()
		{
			return nullptr;
		}
		virtual FRHIHeap* GetDefaultHeapDsv()
		{
			return nullptr;
		}
		virtual FRHIHeap* GetDefaultHeapSampler()
		{
			return nullptr;
		}
		virtual FRHIHeap* GetDefaultHeapCbvSrvUav()
		{
			return nullptr;
		}

	protected:
		static FRHI* RHI;
		FRHI(ERHIType InRHIType);
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
		ERHIType RHIType;
	};
}
