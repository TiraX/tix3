/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

#if COMPILE_WITH_RHI_DX12

#include "FRHIDx12.h"

using namespace Microsoft::WRL;

namespace tix
{
#define FORBID_FUNC(func) func { RuntimeFail(); }
#define FORBID_FUNCP(func) func { RuntimeFail(); return nullptr;}
#define FORBID_FUNCB(func) func { RuntimeFail(); return false;}
#define FORBID_FUNCI(func) func { RuntimeFail(); return 0;}
	// Render hardware interface use DirectX 12
	class FRHIAsyncDx12 : public FRHIDx12
	{
	public:
		virtual ~FRHIAsyncDx12();

		void InitAsyncRHI(ComPtr<ID3D12Device> InD3dDevice);

		// RHI common methods
		FORBID_FUNCP(virtual FRHI* CreateAsyncRHI(const TString& InRHIName) override);
		FORBID_FUNC(virtual void InitRHI() override);
		FORBID_FUNC(virtual void BeginFrame() override);
		FORBID_FUNC(virtual void EndFrame() override);
		FORBID_FUNC(virtual void BeginRenderToFrameBuffer() override);

		FORBID_FUNC(virtual void BeginEvent(const int8* InEventName) override);
		FORBID_FUNC(virtual void BeginEvent(const int8* InEventName, int32 Index) override);
		FORBID_FUNC(virtual void EndEvent() override);

		FORBID_FUNCI(virtual int32 GetCurrentEncodingFrameIndex() override);
		FORBID_FUNC(virtual void WaitingForGpu() override);

		// Create GPU Resource
		//virtual FGPUBufferPtr CreateGPUBuffer() override;
		//virtual FGPUTexturePtr CreateGPUTexture() override;

		// Create RTX related resources
		//virtual FShaderPtr CreateRtxShaderLib(const TString& ShaderLibName) override;
		//virtual FRtxPipelinePtr CreateRtxPipeline(FShaderPtr InShader) override;
		//virtual FTopLevelAccelerationStructurePtr CreateTopLevelAccelerationStructure() override;
		//virtual FBottomLevelAccelerationStructurePtr CreateBottomLevelAccelerationStructure() override;

		// Create Graphics and Compute related resources
		//virtual FPipelinePtr CreatePipeline(FShaderPtr InShader) override;
		//virtual FRenderTargetPtr CreateRenderTarget(int32 W, int32 H) override;
		//virtual FShaderPtr CreateShader(const TShaderNames& InNames, E_SHADER_TYPE Type) override;
		//virtual FShaderPtr CreateComputeShader(const TString& InComputeShaderName) override;
		//virtual FArgumentBufferPtr CreateArgumentBuffer(int32 ReservedSlots) override;
		//virtual FGPUCommandSignaturePtr CreateGPUCommandSignature(FPipelinePtr Pipeline, const TVector<E_GPU_COMMAND_TYPE>& CommandStructure) override;
		//virtual FGPUCommandBufferPtr CreateGPUCommandBuffer(FGPUCommandSignaturePtr GPUCommandSignature, uint32 CommandsCount, uint32 Flag = 0) override;

		// RTX
		FORBID_FUNCB(virtual bool UpdateHardwareResourceRtxPL(FRtxPipelinePtr Pipeline, TRtxPipelinePtr InPipelineDesc) override);
		FORBID_FUNC(virtual void SetRtxPipeline(FRtxPipelinePtr RtxPipeline) override);
		FORBID_FUNC(virtual void TraceRays(FRtxPipelinePtr RtxPipeline, const FInt3& Size) override);

		// Graphics and Compute
		FORBID_FUNCB(virtual bool UpdateHardwareResourceGraphicsPipeline(FPipelinePtr Pipeline, const TPipelineDesc& Desc) override);
		FORBID_FUNCB(virtual bool UpdateHardwareResourceComputePipeline(FPipelinePtr Pipeline) override);
		FORBID_FUNCB(virtual bool UpdateHardwareResourceTilePL(FPipelinePtr Pipeline, TTilePipelinePtr InTilePipelineDesc) override);
		FORBID_FUNCB(virtual bool UpdateHardwareResourceRT(FRenderTargetPtr RenderTarget) override);
		FORBID_FUNCB(virtual bool UpdateHardwareResourceShader(FShaderPtr ShaderResource, const TVector<TStreamPtr>& ShaderCodes) override);
		FORBID_FUNCB(virtual bool UpdateHardwareResourceAB(FArgumentBufferPtr ArgumentBuffer, FShaderPtr InShader, int32 SpecifiedBindingIndex = -1) override);
		FORBID_FUNCB(virtual bool UpdateHardwareResourceGPUCommandSig(FGPUCommandSignaturePtr GPUCommandSignature) override);

		//virtual TStreamPtr ReadGPUBufferToCPU(FGPUBufferPtr GPUBuffer) override;

		//virtual void CopyTextureRegion(FGPUBufferPtr DstBuffer, FGPUTexturePtr SrcTexture, uint32 RowPitch) override;
		//virtual void CopyGPUBuffer(FGPUBufferPtr DstBuffer, FGPUBufferPtr SrcBuffer) override;

		// Be aware of race condition
		//virtual void PutConstantBufferInHeap(FUniformBufferPtr InUniformBuffer, EResourceHeapType InHeapType, uint32 InHeapSlot) override;
		//virtual void PutTextureInHeap(FTexturePtr InTexture, EResourceHeapType InHeapType, uint32 InHeapSlot) override;
		//virtual void PutRWTextureInHeap(FTexturePtr InTexture, uint32 InMipLevel, EResourceHeapType InHeapType, uint32 InHeapSlot) override;
		//virtual void PutUniformBufferInHeap(FUniformBufferPtr InBuffer, EResourceHeapType InHeapType, uint32 InHeapSlot) override;
		//virtual void PutRWUniformBufferInHeap(FUniformBufferPtr InBuffer, EResourceHeapType InHeapType, uint32 InHeapSlot) override;
		//virtual void PutVertexBufferInHeap(FVertexBufferPtr InBuffer, EResourceHeapType InHeapType, int32 InVBHeapSlot) override;
		//virtual void PutIndexBufferInHeap(FIndexBufferPtr InBuffer, EResourceHeapType InHeapType, int32 InIBHeapSlot) override;
		//virtual void PutInstanceBufferInHeap(FInstanceBufferPtr InBuffer, EResourceHeapType InHeapType, uint32 InHeapSlot) override;
		FORBID_FUNC(virtual void PutRTColorInHeap(FTexturePtr InTexture, uint32 InHeapSlot) override);
		FORBID_FUNC(virtual void PutRTDepthInHeap(FTexturePtr InTexture, uint32 InHeapSlot) override);
		FORBID_FUNC(virtual void PutTopAccelerationStructureInHeap(FTopLevelAccelerationStructurePtr InTLAS, EResourceHeapType InHeapType, uint32 InHeapSlot) override);

		// Graphics
		FORBID_FUNC(virtual void SetGraphicsPipeline(FPipelinePtr InPipeline) override);
		FORBID_FUNC(virtual void SetVertexBuffer(FVertexBufferPtr InVertexBuffer, FInstanceBufferPtr InInstanceBuffer) override);
		FORBID_FUNC(virtual void SetIndexBuffer(FIndexBufferPtr InIndexBuffer) override);

		FORBID_FUNC(virtual void SetUniformBuffer(E_SHADER_STAGE ShaderStage, int32 BindIndex, FUniformBufferPtr InUniformBuffer) override);
		FORBID_FUNC(virtual void SetRenderResourceTable(int32 BindIndex, FRenderResourceTablePtr RenderResourceTable) override);
		FORBID_FUNC(virtual void SetArgumentBuffer(int32 InBindIndex, FArgumentBufferPtr InArgumentBuffer) override);

		FORBID_FUNC(virtual void UAVBarrier(FBottomLevelAccelerationStructurePtr BLAS));
		//virtual void SetGPUBufferName(FGPUBufferPtr GPUBuffer, const TString& Name) override;
		//virtual void SetGPUTextureName(FGPUTexturePtr GPUTexture, const TString& Name) override;
		//virtual void SetGPUBufferState(FGPUBufferPtr GPUBuffer, EGPUResourceState NewState) override;
		//virtual void SetGPUTextureState(FGPUTexturePtr GPUTexture, EGPUResourceState NewState) override;
		//virtual void FlushResourceStateChange() override;

		FORBID_FUNC(virtual void SetStencilRef(uint32 InRefValue) override);
		FORBID_FUNC(virtual void DrawPrimitiveInstanced(uint32 VertexCount, uint32 InstanceCount, uint32 InstanceOffset) override);
		FORBID_FUNC(virtual void DrawPrimitiveIndexedInstanced(uint32 IndexCount, uint32 InstanceCount, uint32 InstanceOffset) override);
		FORBID_FUNC(virtual void DrawPrimitiveIndexedInstanced(uint32 IndexCount, uint32 InstanceCount, uint32 IndexOffset, uint32 InstanceOffset) override);

		// Tile, For Metal, dx12 has empty implementation
		FORBID_FUNC(virtual void SetTilePipeline(FPipelinePtr InPipeline) override);
		FORBID_FUNC(virtual void SetTileBuffer(int32 BindIndex, FUniformBufferPtr InUniformBuffer) override);
		FORBID_FUNC(virtual void DispatchTile(const FInt3& GroupSize) override);

		// Compute
		//virtual void SetComputePipeline(FPipelinePtr InPipeline) override;
		//virtual void SetComputeConstant(int32 BindIndex, const FUInt4& InValue) override;
		//virtual void SetComputeConstant(int32 BindIndex, const FFloat4& InValue) override;
		//virtual void SetComputeConstantBuffer(int32 BindIndex, FUniformBufferPtr InUniformBuffer, uint32 BufferOffset = 0) override;
		//virtual void SetComputeShaderResource(int32 BindIndex, FUniformBufferPtr InUniformBuffer, uint32 BufferOffset = 0) override;
		//virtual void SetComputeResourceTable(int32 BindIndex, FRenderResourceTablePtr RenderResourceTable) override;
		//virtual void SetComputeArgumentBuffer(int32 BindIndex, FArgumentBufferPtr InArgumentBuffer) override;
		//virtual void SetComputeTexture(int32 BindIndex, FTexturePtr InTexture) override;

		//virtual void DispatchCompute(const FInt3& GroupSize, const FInt3& GroupCount) override;

		// GPU Command buffer
		FORBID_FUNC(virtual void ExecuteGPUDrawCommands(FGPUCommandBufferPtr GPUCommandBuffer) override);
		FORBID_FUNC(virtual void ExecuteGPUComputeCommands(FGPUCommandBufferPtr GPUCommandBuffer) override);

		FORBID_FUNC(virtual void SetViewport(const FViewport& InViewport) override);
		FORBID_FUNC(virtual void BeginRenderToRenderTarget(FRenderTargetPtr RT, const int8* PassName = "UnnamedPass", uint32 MipLevel = 0) override);


		//// Direct12 Specified Functions
		//void CreateD3D12Resource(
		//	D3D12_HEAP_TYPE HeapType,
		//	const D3D12_RESOURCE_DESC* pDesc,
		//	D3D12_RESOURCE_STATES InitState,
		//	const D3D12_CLEAR_VALUE* pOptimizedClearValue,
		//	ComPtr<ID3D12Resource>& OutResource
		//);

		//virtual void UpdateD3D12Resource(
		//	_In_ ID3D12Resource* pDestinationResource,
		//	_In_ ID3D12Resource* pIntermediate,
		//	uint32 NumSubResources,
		//	const D3D12_SUBRESOURCE_DATA* pData
		//) override;

		//uint64 GetRequiredIntermediateSize(
		//	const D3D12_RESOURCE_DESC& Desc,
		//	uint32 FirstSubresource,
		//	uint32 NumSubresources);

		//ID3D12DescriptorHeap* GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE Type)
		//{
		//	return DescriptorHeaps[Type].GetHeap();
		//}

		//ID3D12Device5* GetDXRDevice()
		//{
		//	return DXR->DXRDevice.Get();
		//}

		//ID3D12GraphicsCommandList4* GetDXRCommandList()
		//{
		//	return DXR->DXRCommandList.Get();
		//}
		//void HoldResourceReference(FRenderResourcePtr InResource);
		//void HoldResourceReference(ComPtr<ID3D12Resource> InDxResource);

		//static uint32 GetUavCounterOffset(uint32 InBufferSize);
		//static uint32 GetUavSizeWithCounter(uint32 InBufferSize);

		//D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView(FVertexBufferPtr VB);
		//D3D12_VERTEX_BUFFER_VIEW GetInstanceBufferView(FInstanceBufferPtr IB);
		//D3D12_INDEX_BUFFER_VIEW GetIndexBufferView(FIndexBufferPtr IB);
		//D3D12_CONSTANT_BUFFER_VIEW_DESC GetConstantBufferView(FUniformBufferPtr UB);

		//D3D12_GPU_VIRTUAL_ADDRESS GetGPUBufferGPUAddress(FGPUResourcePtr GPUBuffer);
		//D3D12_GPU_VIRTUAL_ADDRESS GetGPUTextureGPUAddress(FGPUResourcePtr GPUTexture);
	protected: 
		FRHIAsyncDx12(const TString& InRHIName);

		FORBID_FUNC(virtual void FeatureCheck() override);

	private:
		FORBID_FUNC(virtual void GetHardwareAdapter(IDXGIAdapter1** ppAdapter) override);
		FORBID_FUNC(virtual void CreateWindowsSizeDependentResources() override);
		FORBID_FUNC(virtual void MoveToNextFrame() override);

		//FShaderBindingPtr CreateShaderBinding(const D3D12_ROOT_SIGNATURE_DESC& RSDesc);

		//uint64 UpdateSubresources(
		//	_In_ ID3D12GraphicsCommandList* pCmdList,
		//	_In_ ID3D12Resource* pDestinationResource,
		//	_In_ ID3D12Resource* pIntermediate,
		//	_In_range_(0, D3D12_REQ_SUBRESOURCES) uint32 FirstSubresource,
		//	_In_range_(0, D3D12_REQ_SUBRESOURCES - FirstSubresource) uint32 NumSubresources,
		//	uint64 RequiredSize,
		//	_In_reads_(NumSubresources) const D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts,
		//	_In_reads_(NumSubresources) const uint32* pNumRows,
		//	_In_reads_(NumSubresources) const uint64* pRowSizesInBytes,
		//	_In_reads_(NumSubresources) const D3D12_SUBRESOURCE_DATA* pSrcData);
		//uint64 UpdateSubresources(
		//	_In_ ID3D12GraphicsCommandList* pCmdList,
		//	_In_ ID3D12Resource* pDestinationResource,
		//	_In_ ID3D12Resource* pIntermediate,
		//	uint64 IntermediateOffset,
		//	_In_range_(0, D3D12_REQ_SUBRESOURCES) uint32 FirstSubresource,
		//	_In_range_(0, D3D12_REQ_SUBRESOURCES - FirstSubresource) uint32 NumSubresources,
		//	_In_reads_(NumSubresources) const D3D12_SUBRESOURCE_DATA* pSrcData);

		//virtual void _Transition(
		//	_In_ ID3D12Resource* pResource,
		//	D3D12_RESOURCE_STATES stateBefore,
		//	D3D12_RESOURCE_STATES stateAfter,
		//	uint32 subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
		//	D3D12_RESOURCE_BARRIER_FLAGS flags = D3D12_RESOURCE_BARRIER_FLAG_NONE) override;

		FORBID_FUNC(virtual void UAVBarrier(_In_ ID3D12Resource* pResource) override);

		FORBID_FUNC(virtual void FlushBarriers(_In_ ID3D12GraphicsCommandList* pCmdList) override);

		FORBID_FUNC(virtual void SetRenderTarget(FRenderTargetPtr RT, uint32 MipLevel) override);

		FORBID_FUNC(virtual void InitRHIRenderResourceHeap(EResourceHeapType Heap, uint32 HeapSize, uint32 HeapOffset));
		//D3D12_CPU_DESCRIPTOR_HANDLE GetCpuDescriptorHandle(EResourceHeapType Heap, uint32 SlotIndex);
		//D3D12_GPU_DESCRIPTOR_HANDLE GetGpuDescriptorHandle(EResourceHeapType Heap, uint32 SlotIndex);

		FORBID_FUNCB(virtual bool InitRaytracing() override);
	private:
		friend class FRHIDx12;
	};

#undef FORBID_FUNC

}
#endif	// COMPILE_WITH_RHI_DX12