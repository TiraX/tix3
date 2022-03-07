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

		// RHI common methods
		virtual FRHI* CreateAsyncRHI(const TString& InRHIName) override;
		virtual void InitRHI() override;
		virtual void BeginFrame() override;
		virtual void EndFrame() override;
		virtual void BeginRenderToFrameBuffer() override;

		virtual int32 GetCurrentEncodingFrameIndex() override;
		virtual void WaitingForGpu() override;

		// Create Command List
		virtual FRHICmdList* CreateRHICommandList(
			ERHICmdList Type, 
			const TString& InNamePrefix, 
			int32 BufferCount) override;

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

		virtual void PutConstantBufferInHeap(FUniformBufferPtr InUniformBuffer, EResourceHeapType InHeapType, uint32 InHeapSlot) override;
		virtual void PutTextureInHeap(FTexturePtr InTexture, EResourceHeapType InHeapType, uint32 InHeapSlot) override;
		virtual void PutRWTextureInHeap(FTexturePtr InTexture, uint32 InMipLevel, EResourceHeapType InHeapType, uint32 InHeapSlot) override;
		virtual void PutUniformBufferInHeap(FUniformBufferPtr InBuffer, EResourceHeapType InHeapType, uint32 InHeapSlot) override;
		virtual void PutRWUniformBufferInHeap(FUniformBufferPtr InBuffer, EResourceHeapType InHeapType, uint32 InHeapSlot) override;
		virtual void PutVertexBufferInHeap(FVertexBufferPtr InBuffer, EResourceHeapType InHeapType, int32 InVBHeapSlot) override;
		virtual void PutIndexBufferInHeap(FIndexBufferPtr InBuffer, EResourceHeapType InHeapType, int32 InIBHeapSlot) override;
		virtual void PutInstanceBufferInHeap(FInstanceBufferPtr InBuffer, EResourceHeapType InHeapType, uint32 InHeapSlot) override;
		virtual void PutRTColorInHeap(FTexturePtr InTexture, uint32 InHeapSlot) override;
		virtual void PutRTDepthInHeap(FTexturePtr InTexture, uint32 InHeapSlot) override;
		virtual void PutTopAccelerationStructureInHeap(FTopLevelAccelerationStructurePtr InTLAS, EResourceHeapType InHeapType, uint32 InHeapSlot) override;

		virtual void SetGPUBufferName(FGPUBufferPtr GPUBuffer, const TString& Name) override;
		virtual void SetGPUTextureName(FGPUTexturePtr GPUTexture, const TString& Name) override;


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

		ID3D12DescriptorHeap* GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE Type)
		{
			return DescriptorHeaps[Type].GetHeap();
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
	protected: 
		FRHIDx12();

		virtual void FeatureCheck() override;

	private:
		virtual void GetHardwareAdapter(IDXGIAdapter1** ppAdapter);
		virtual void CreateWindowsSizeDependentResources();
		virtual void MoveToNextFrame();

		FShaderBindingPtr CreateShaderBinding(const D3D12_ROOT_SIGNATURE_DESC& RSDesc);

		virtual void InitRHIRenderResourceHeap(EResourceHeapType Heap, uint32 HeapSize, uint32 HeapOffset);
		D3D12_CPU_DESCRIPTOR_HANDLE GetCpuDescriptorHandle(EResourceHeapType Heap, uint32 SlotIndex);
		D3D12_GPU_DESCRIPTOR_HANDLE GetGpuDescriptorHandle(EResourceHeapType Heap, uint32 SlotIndex);

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

		// Descriptor heaps
		FDescriptorHeapDx12 DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

		uint32 CurrentFrame;

		// Root Descriptor cache
		THMap<uint32, FShaderBindingPtr> ShaderBindingCache;

		// Raytracing component
		FRHIDXR* DXR;

		FRHICmdListDx12* CmdListDirectDx12Ref;

		// Asynchronized RHI for Async Compute
		TVector<FRHI*> AsyncRHIs;

		friend class FRHI;
		friend class FDescriptorHeapDx12;
	};
}
#endif	// COMPILE_WITH_RHI_DX12