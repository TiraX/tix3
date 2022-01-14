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
	// Render hardware interface use DirectX 12
	class FRHIDx12 : public FRHI
	{
	public:
		virtual ~FRHIDx12();

		// RHI common methods
		virtual void InitRHI() override;
		virtual void BeginFrame() override;
		virtual void EndFrame() override;
		virtual void BeginRenderToFrameBuffer() override;

		virtual void BeginEvent(const int8* InEventName) override;
		virtual void BeginEvent(const int8* InEventName, int32 Index) override;
		virtual void EndEvent() override;

		virtual int32 GetCurrentEncodingFrameIndex() override;
		virtual void WaitingForGpu() override;

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
		virtual void SetRtxPipeline(FRtxPipelinePtr RtxPipeline) override;
		virtual void TraceRays(FRtxPipelinePtr RtxPipeline, const FInt3& Size) override;

		// Graphics and Compute
		virtual bool UpdateHardwareResourcePL(FPipelinePtr Pipeline, TPipelinePtr InPipelineDesc) override;
		virtual bool UpdateHardwareResourceTilePL(FPipelinePtr Pipeline, TTilePipelinePtr InTilePipelineDesc) override;
		virtual bool UpdateHardwareResourceRT(FRenderTargetPtr RenderTarget) override;
		virtual bool UpdateHardwareResourceShader(FShaderPtr ShaderResource, TShaderPtr InShaderSource) override;
		virtual bool UpdateHardwareResourceAB(FArgumentBufferPtr ArgumentBuffer, FShaderPtr InShader, int32 SpecifiedBindingIndex = -1) override;
		virtual bool UpdateHardwareResourceGPUCommandSig(FGPUCommandSignaturePtr GPUCommandSignature) override;

		virtual TStreamPtr ReadGPUBufferToCPU(FGPUBufferPtr GPUBuffer) override;

		virtual void CopyTextureRegion(FGPUBufferPtr DstBuffer, FGPUTexturePtr SrcTexture, uint32 RowPitch) override;
		virtual void CopyGPUBuffer(FGPUBufferPtr DstBuffer, FGPUBufferPtr SrcBuffer) override;

		virtual void PutConstantBufferInHeap(FUniformBufferPtr InUniformBuffer, E_RENDER_RESOURCE_HEAP_TYPE InHeapType, uint32 InHeapSlot) override;
		virtual void PutTextureInHeap(FTexturePtr InTexture, E_RENDER_RESOURCE_HEAP_TYPE InHeapType, uint32 InHeapSlot) override;
		virtual void PutRWTextureInHeap(FTexturePtr InTexture, uint32 InMipLevel, E_RENDER_RESOURCE_HEAP_TYPE InHeapType, uint32 InHeapSlot) override;
		virtual void PutUniformBufferInHeap(FUniformBufferPtr InBuffer, E_RENDER_RESOURCE_HEAP_TYPE InHeapType, uint32 InHeapSlot) override;
		virtual void PutRWUniformBufferInHeap(FUniformBufferPtr InBuffer, E_RENDER_RESOURCE_HEAP_TYPE InHeapType, uint32 InHeapSlot) override;
		virtual void PutVertexBufferInHeap(FVertexBufferPtr InBuffer, E_RENDER_RESOURCE_HEAP_TYPE InHeapType, int32 InVBHeapSlot) override;
		virtual void PutIndexBufferInHeap(FIndexBufferPtr InBuffer, E_RENDER_RESOURCE_HEAP_TYPE InHeapType, int32 InIBHeapSlot) override;
		virtual void PutInstanceBufferInHeap(FInstanceBufferPtr InBuffer, E_RENDER_RESOURCE_HEAP_TYPE InHeapType, uint32 InHeapSlot) override;
		virtual void PutRTColorInHeap(FTexturePtr InTexture, uint32 InHeapSlot) override;
		virtual void PutRTDepthInHeap(FTexturePtr InTexture, uint32 InHeapSlot) override;
		virtual void PutTopAccelerationStructureInHeap(FTopLevelAccelerationStructurePtr InTLAS, E_RENDER_RESOURCE_HEAP_TYPE InHeapType, uint32 InHeapSlot) override;

		// Graphics
		virtual void SetGraphicsPipeline(FPipelinePtr InPipeline) override;
		virtual void SetVertexBuffer(FVertexBufferPtr InVertexBuffer, FInstanceBufferPtr InInstanceBuffer) override;
		virtual void SetIndexBuffer(FIndexBufferPtr InIndexBuffer) override;

		virtual void SetUniformBuffer(E_SHADER_STAGE ShaderStage, int32 BindIndex, FUniformBufferPtr InUniformBuffer) override;
		virtual void SetRenderResourceTable(int32 BindIndex, FRenderResourceTablePtr RenderResourceTable) override;
		virtual void SetArgumentBuffer(int32 InBindIndex, FArgumentBufferPtr InArgumentBuffer) override;

				void UAVBarrier(FBottomLevelAccelerationStructurePtr BLAS);
		virtual void SetGPUBufferName(FGPUBufferPtr GPUBuffer, const TString& Name) override;
		virtual void SetGPUTextureName(FGPUTexturePtr GPUTexture, const TString& Name) override;
		virtual void SetGPUBufferState(FGPUBufferPtr GPUBuffer, EGPUResourceState NewState) override;
		virtual void SetGPUTextureState(FGPUTexturePtr GPUTexture, EGPUResourceState NewState) override;
		virtual void FlushResourceStateChange() override;

		virtual void SetStencilRef(uint32 InRefValue) override;
		virtual void DrawPrimitiveInstanced(uint32 VertexCount, uint32 InstanceCount, uint32 InstanceOffset) override;
		virtual void DrawPrimitiveIndexedInstanced(uint32 IndexCount, uint32 InstanceCount, uint32 InstanceOffset) override;
		virtual void DrawPrimitiveIndexedInstanced(uint32 IndexCount, uint32 InstanceCount, uint32 IndexOffset, uint32 InstanceOffset) override;

		// Tile, For Metal, dx12 has empty implementation
		virtual void SetTilePipeline(FPipelinePtr InPipeline) override;
		virtual void SetTileBuffer(int32 BindIndex, FUniformBufferPtr InUniformBuffer) override;
		virtual void DispatchTile(const FInt3& GroupSize) override;

		// Compute
		virtual void SetComputePipeline(FPipelinePtr InPipeline) override;
		virtual void SetComputeConstant(int32 BindIndex, const FUInt4& InValue) override;
		virtual void SetComputeConstant(int32 BindIndex, const FFloat4& InValue) override;
		virtual void SetComputeConstantBuffer(int32 BindIndex, FUniformBufferPtr InUniformBuffer, uint32 BufferOffset = 0) override;
		virtual void SetComputeShaderResource(int32 BindIndex, FUniformBufferPtr InUniformBuffer, uint32 BufferOffset = 0) override;
		virtual void SetComputeResourceTable(int32 BindIndex, FRenderResourceTablePtr RenderResourceTable) override;
		virtual void SetComputeArgumentBuffer(int32 BindIndex, FArgumentBufferPtr InArgumentBuffer) override;
		virtual void SetComputeTexture(int32 BindIndex, FTexturePtr InTexture) override;

		virtual void DispatchCompute(const FInt3& GroupSize, const FInt3& GroupCount) override;

		// GPU Command buffer
		virtual void ExecuteGPUDrawCommands(FGPUCommandBufferPtr GPUCommandBuffer) override;
		virtual void ExecuteGPUComputeCommands(FGPUCommandBufferPtr GPUCommandBuffer) override;

		virtual void SetViewport(const FViewport& InViewport) override;
		virtual void BeginRenderToRenderTarget(FRenderTargetPtr RT, const int8* PassName = "UnnamedPass", uint32 MipLevel = 0, const SColor& ClearColor = SColor(0, 0, 0, 0)) override;


		// Direct12 Specified Functions
		void CreateD3D12Resource(
			D3D12_HEAP_TYPE HeapType,
			const D3D12_RESOURCE_DESC* pDesc,
			D3D12_RESOURCE_STATES InitState,
			const D3D12_CLEAR_VALUE* pOptimizedClearValue,
			ComPtr<ID3D12Resource>& OutResource
		);

		void UpdateD3D12Resource(
			_In_ ID3D12Resource* pDestinationResource,
			_In_ ID3D12Resource* pIntermediate,
			uint32 NumSubResources,
			const D3D12_SUBRESOURCE_DATA* pData
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
		void HoldResourceReference(FRenderResourcePtr InResource);
		void HoldResourceReference(ComPtr<ID3D12Resource> InDxResource);

		static uint32 GetUavCounterOffset(uint32 InBufferSize);
		static uint32 GetUavSizeWithCounter(uint32 InBufferSize);

		D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView(FVertexBufferPtr VB);
		D3D12_VERTEX_BUFFER_VIEW GetInstanceBufferView(FInstanceBufferPtr IB);
		D3D12_INDEX_BUFFER_VIEW GetIndexBufferView(FIndexBufferPtr IB);
		D3D12_CONSTANT_BUFFER_VIEW_DESC GetConstantBufferView(FUniformBufferPtr UB);
	protected: 
		FRHIDx12();

		virtual void FeatureCheck() override;

	private:
		void GetHardwareAdapter(IDXGIAdapter1** ppAdapter);
		void CreateWindowsSizeDependentResources();
		void MoveToNextFrame();

		FShaderBindingPtr CreateShaderBinding(const D3D12_ROOT_SIGNATURE_DESC& RSDesc);

		uint64 UpdateSubresources(
			_In_ ID3D12GraphicsCommandList* pCmdList,
			_In_ ID3D12Resource* pDestinationResource,
			_In_ ID3D12Resource* pIntermediate,
			_In_range_(0, D3D12_REQ_SUBRESOURCES) uint32 FirstSubresource,
			_In_range_(0, D3D12_REQ_SUBRESOURCES - FirstSubresource) uint32 NumSubresources,
			uint64 RequiredSize,
			_In_reads_(NumSubresources) const D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts,
			_In_reads_(NumSubresources) const uint32* pNumRows,
			_In_reads_(NumSubresources) const uint64* pRowSizesInBytes,
			_In_reads_(NumSubresources) const D3D12_SUBRESOURCE_DATA* pSrcData);
		uint64 UpdateSubresources(
			_In_ ID3D12GraphicsCommandList* pCmdList,
			_In_ ID3D12Resource* pDestinationResource,
			_In_ ID3D12Resource* pIntermediate,
			uint64 IntermediateOffset,
			_In_range_(0, D3D12_REQ_SUBRESOURCES) uint32 FirstSubresource,
			_In_range_(0, D3D12_REQ_SUBRESOURCES - FirstSubresource) uint32 NumSubresources,
			_In_reads_(NumSubresources) const D3D12_SUBRESOURCE_DATA* pSrcData);

		void _Transition(
			_In_ ID3D12Resource* pResource,
			D3D12_RESOURCE_STATES stateBefore,
			D3D12_RESOURCE_STATES stateAfter,
			uint32 subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
			D3D12_RESOURCE_BARRIER_FLAGS flags = D3D12_RESOURCE_BARRIER_FLAG_NONE);

		void UAVBarrier(_In_ ID3D12Resource* pResource);

		void FlushGraphicsBarriers(
			_In_ ID3D12GraphicsCommandList* pCmdList);

		void SetRenderTarget(FRenderTargetPtr RT, uint32 MipLevel, const SColor& ClearColor);

		void InitRHIRenderResourceHeap(E_RENDER_RESOURCE_HEAP_TYPE Heap, uint32 HeapSize, uint32 HeapOffset);
		D3D12_CPU_DESCRIPTOR_HANDLE GetCpuDescriptorHandle(E_RENDER_RESOURCE_HEAP_TYPE Heap, uint32 SlotIndex);
		D3D12_GPU_DESCRIPTOR_HANDLE GetGpuDescriptorHandle(E_RENDER_RESOURCE_HEAP_TYPE Heap, uint32 SlotIndex);

		bool InitRaytracing();

		D3D12_GPU_VIRTUAL_ADDRESS GetGPUBufferGPUAddress(FGPUResourcePtr GPUBuffer);
		D3D12_GPU_VIRTUAL_ADDRESS GetGPUTextureGPUAddress(FGPUResourcePtr GPUTexture);
	private:
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

		// Commands
		ComPtr<ID3D12CommandAllocator> DirectCommandAllocators[FRHIConfig::FrameBufferNum];
		ComPtr<ID3D12CommandQueue> DirectCommandQueue;
		ComPtr<ID3D12GraphicsCommandList> DirectCommandList;
		ComPtr<ID3D12Fence> DirectCommandFence;

		// Descriptor heaps
		FDescriptorHeapDx12 DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

		// CPU/GPU Synchronization.
		uint64 FenceValues[FRHIConfig::FrameBufferNum];
		HANDLE FenceEvent;
		uint32 CurrentFrame;

		// Barriers
		D3D12_RESOURCE_BARRIER GraphicsBarrierBuffers[FRHIConfig::MaxResourceBarrierBuffers];
		uint32 GraphicsNumBarriersToFlush;
		D3D12_RESOURCE_BARRIER ComputeBarrierBuffers[FRHIConfig::MaxResourceBarrierBuffers];
		uint32 ComputeNumBarriersToFlush;

		// Frame on the fly Resource holders
		FFrameResourcesDx12 * ResHolders[FRHIConfig::FrameBufferNum];

		// Root Descriptor cache
		THMap<uint32, FShaderBindingPtr> ShaderBindingCache;

		// Raytracing component
		FRHIDXR* DXR;

		friend class FRHI;
		friend class FDescriptorHeapDx12;
	};
}
#endif	// COMPILE_WITH_RHI_DX12