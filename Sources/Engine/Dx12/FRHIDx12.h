/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

#if COMPILE_WITH_RHI_DX12

#include <dxgi1_6.h>
#include <d3d12.h>
#include "FRHIDescriptorHeapDx12.h"
#include "FRootSignatureDx12.h"
#include "FRHICmdListDx12.h"
#include "FRHIDXR.h"

using namespace Microsoft::WRL;

namespace tix
{
	class FFrameResourcesDx12;
	class FGPUResourceDx12;
	class FRHICmdListDx12;
	// Render hardware interface use DirectX 12
	class FRHIDx12 : public FRHI
	{
	public:
		virtual ~FRHIDx12();

		static void ReportDx12LiveObjects();

		// RHI common methods
		virtual void InitRHI() override;
		virtual void BeginFrame() override;
		virtual void EndFrame() override;
		virtual void BeginRenderToFrameBuffer() override;

		virtual int32 GetCurrentEncodingFrameIndex() override;
		virtual void WaitingForGpu() override;

		virtual TStreamPtr CompileShader(const TString& InCode, const TString& Entry, const TString& Target, const TString& IncludePath, bool Debug = false) override;

		// Create Command List
		virtual FRHICmdList* CreateRHICommandList(
			ERHICmdList Type, 
			const TString& InNamePrefix, 
			int32 BufferCount) override;

		// Create Resource Heap
		virtual FRHIHeap* CreateHeap(EResourceHeapType Type) override;

		// Create GPU Resource
		virtual FGPUBufferPtr CreateGPUBuffer() override;
		virtual FGPUTexturePtr CreateGPUTexture() override;

		// Create RTX related resources
		virtual FShaderPtr CreateRtxShaderLib(const TString& ShaderLibName) override;
		virtual FRtxPipelinePtr CreateRtxPipeline(FShaderPtr InShader) override;
		virtual FTopLevelAccelerationStructurePtr CreateTopLevelAccelerationStructure() override;
		virtual FBottomLevelAccelerationStructurePtr CreateBottomLevelAccelerationStructure() override;

		// Create Graphics and Compute related resources
		virtual FPipelinePtr CreatePipeline(FShaderPtr InShader) override;
		virtual FRenderTargetPtr CreateRenderTarget(int32 W, int32 H) override;
		virtual FShaderPtr CreateShader(const TShaderNames& InNames, E_SHADER_TYPE Type) override;
		virtual FShaderPtr CreateComputeShader(const TString& InComputeShaderName) override;
		virtual FArgumentBufferPtr CreateArgumentBuffer(int32 ReservedSlots) override;
		virtual FGPUCommandSignaturePtr CreateGPUCommandSignature(FPipelinePtr Pipeline, const TVector<E_GPU_COMMAND_TYPE>& CommandStructure) override;
		virtual FGPUCommandBufferPtr CreateGPUCommandBuffer(FGPUCommandSignaturePtr GPUCommandSignature, uint32 CommandsCount, uint32 Flag = 0) override;

		// RTX
		virtual bool UpdateHardwareResourceRtxPL(FRtxPipelinePtr Pipeline, TRtxPipelinePtr InPipelineDesc) override;

		// Graphics and Compute
		virtual bool UpdateHardwareResourceGraphicsPipeline(FPipelinePtr Pipeline, const TPipelineDesc& Desc) override;
		virtual bool UpdateHardwareResourceComputePipeline(FPipelinePtr Pipeline) override;
		virtual bool UpdateHardwareResourceTilePL(FPipelinePtr Pipeline, TTilePipelinePtr InTilePipelineDesc) override;
		virtual bool UpdateHardwareResourceRT(FRenderTargetPtr RenderTarget) override;
		virtual bool UpdateHardwareResourceShader(FShaderPtr ShaderResource, const TVector<TStreamPtr>& ShaderCodes) override;
		virtual bool UpdateHardwareResourceAB(FArgumentBufferPtr ArgumentBuffer, FShaderPtr InShader, int32 SpecifiedBindingIndex = -1) override;
		virtual bool UpdateHardwareResourceGPUCommandSig(FGPUCommandSignaturePtr GPUCommandSignature) override;

		virtual TStreamPtr ReadGPUBufferToCPU(FGPUBufferPtr GPUBuffer) override;

		virtual void PutConstantBufferInTable(FRenderResourceTablePtr RRTable, FUniformBufferPtr InUniformBuffer, uint32 InTableSlot) override;
		virtual void PutTextureInTable(FRenderResourceTablePtr RRTable, FTexturePtr InTexture, uint32 InTableSlot) override;
		virtual void PutRWTextureInTable(FRenderResourceTablePtr RRTable, FTexturePtr InTexture, uint32 InMipLevel, uint32 InTableSlot) override;
		virtual void PutUniformBufferInTable(FRenderResourceTablePtr RRTable, FUniformBufferPtr InBuffer, uint32 InTableSlot) override;
		virtual void PutRWUniformBufferInTable(FRenderResourceTablePtr RRTable, FUniformBufferPtr InBuffer, uint32 InTableSlot) override;
		virtual void PutRTColorInTable(FRenderResourceTablePtr RRTable, FTexturePtr InTexture, uint32 InTableSlot) override;
		virtual void PutRTDepthInTable(FRenderResourceTablePtr RRTable, FTexturePtr InTexture, uint32 InTableSlot) override;
		virtual void PutTopAccelerationStructureInTable(FRenderResourceTablePtr RRTable, FTopLevelAccelerationStructurePtr InTLAS, uint32 InTableSlot) override;

		virtual void SetGPUBufferName(FGPUBufferPtr GPUBuffer, const TString& Name) override;
		virtual void SetGPUTextureName(FGPUTexturePtr GPUTexture, const TString& Name) override;

		virtual FRHICmdList* GetDefaultCmdList() override
		{
			return CmdListDefault;
		}
		virtual FRHIHeap* GetHeapById(uint32 HeapId) override
		{
			return DescriptorHeaps[HeapId];
		}
		virtual FRHIHeap* GetDefaultHeapRtv() override
		{
			return HeapRtv;
		}
		virtual FRHIHeap* GetDefaultHeapDsv() override
		{
			return HeapDsv;
		}
		virtual FRHIHeap* GetDefaultHeapSampler() override
		{
			return HeapSampler;
		}
		virtual FRHIHeap* GetDefaultHeapCbvSrvUav() override
		{
			return HeapCbvSrvUav;
		}

		// Direct12 Specified Functions
		void CreateD3D12Resource(
			D3D12_HEAP_TYPE HeapType,
			const D3D12_RESOURCE_DESC* pDesc,
			D3D12_RESOURCE_STATES InitState,
			const D3D12_CLEAR_VALUE* pOptimizedClearValue,
			ComPtr<ID3D12Resource>& OutResource
		);

		uint64 GetRequiredIntermediateSize(
			const D3D12_RESOURCE_DESC& Desc,
			uint32 FirstSubresource,
			uint32 NumSubresources);

		ID3D12Device* GetD3dDevice()
		{
			return D3dDevice.Get();
		}

		ID3D12Device5* GetDXRDevice()
		{
			return DXR->DXRDevice.Get();
		}

		ID3D12GraphicsCommandList4* GetDXRCommandList()
		{
			return DXR->DXRCommandList.Get();
		}

		static uint32 GetUavCounterOffset(uint32 InBufferSize);
		static uint32 GetUavSizeWithCounter(uint32 InBufferSize);

		static D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView(FVertexBufferPtr VB);
		static D3D12_VERTEX_BUFFER_VIEW GetInstanceBufferView(FInstanceBufferPtr IB);
		static D3D12_INDEX_BUFFER_VIEW GetIndexBufferView(FIndexBufferPtr IB);
		static D3D12_CONSTANT_BUFFER_VIEW_DESC GetConstantBufferView(FUniformBufferPtr UB);

		static D3D12_GPU_VIRTUAL_ADDRESS GetGPUBufferGPUAddress(FGPUResourcePtr GPUBuffer);
		static D3D12_GPU_VIRTUAL_ADDRESS GetGPUTextureGPUAddress(FGPUResourcePtr GPUTexture);

		D3D12_CPU_DESCRIPTOR_HANDLE GetCpuDescriptorHandle(FRenderResourceTablePtr RRTable, uint32 SlotIndex);
		D3D12_GPU_DESCRIPTOR_HANDLE GetGpuDescriptorHandle(FRenderResourceTablePtr RRTable, uint32 SlotIndex);
	protected: 
		FRHIDx12();

		virtual void FeatureCheck() override;

	private:
		virtual void GetHardwareAdapter(IDXGIAdapter1** ppAdapter);
		virtual void CreateWindowsSizeDependentResources();
		virtual void MoveToNextFrame();

		FShaderBindingPtr CreateShaderBinding(const D3D12_ROOT_SIGNATURE_DESC& RSDesc);

		virtual bool InitRaytracing();
	protected:
		ComPtr<ID3D12Device> D3dDevice;
		ComPtr<IDXGIFactory4> DxgiFactory;
		ComPtr<IDXGISwapChain3> SwapChain;
		
		// Back buffers and Depth Stencil buffers
		ComPtr<ID3D12Resource> BackBufferRTs[FRHIConfig::FrameBufferNum];
		FRenderResourceTablePtr BackBufferDescriptorTable;
		D3D12_CPU_DESCRIPTOR_HANDLE BackBufferDescriptors[FRHIConfig::FrameBufferNum];
		ComPtr<ID3D12Resource> DepthStencil;
		FRenderResourceTablePtr DepthStencilDescriptorTable;
		D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilDescriptor;
		FUInt2 BackBufferSize;

		// Descriptor heaps, canbe access from different threads, make it thread safe.
		TMutex _DHeapMutex;
		TThreadSafeVector<FDescriptorHeapDx12*> DescriptorHeaps;
		FDescriptorHeapDx12* HeapRtv;
		FDescriptorHeapDx12* HeapDsv;
		FDescriptorHeapDx12* HeapSampler;
		FDescriptorHeapDx12* HeapCbvSrvUav;

		uint32 CurrentFrame;

		// Root Descriptor cache
		THMap<uint32, FShaderBindingPtr> ShaderBindingCache;

		// Raytracing component
		FRHIDXR* DXR;

		// Command Lists
		TThreadSafeVector<FRHICmdListDx12*> CmdLists;
		FRHICmdListDx12* CmdListDefault;

		friend class FRHI;
		friend class FDescriptorHeapDx12;
	};
}
#endif	// COMPILE_WITH_RHI_DX12