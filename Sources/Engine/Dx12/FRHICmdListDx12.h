/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

#if COMPILE_WITH_RHI_DX12
#include <d3d12.h>
#include "d3dx12.h"
#include "FRHICmdList.h"

using namespace Microsoft::WRL;

namespace tix
{
	class FRHIDx12;
	class FFrameResourcesDx12;
	// Render hardware interface use DirectX 12
	class FRHICmdListDx12 : public FRHICmdList
	{
	public:
		virtual ~FRHICmdListDx12();

		virtual void BeginCmdList() override;
		virtual void EndCmdList() override;
		virtual void AddHeap(FRHIHeap* InHeap) override;
		virtual FRHIHeap* GetHeap(int32 Index) override;
		virtual void Close() override;
		virtual void Execute() override;
		virtual void WaitingForGpu() override;
		virtual void MoveToNextFrame() override;

		// Command list methods
		virtual void BeginEvent(const int8* InEventName) override;
		virtual void BeginEvent(const int8* InEventName, int32 Index) override;
		virtual void EndEvent() override;

		virtual void CopyTextureRegion(FGPUBufferPtr DstBuffer, FGPUTexturePtr SrcTexture, uint32 RowPitch) override;
		virtual void CopyGPUBuffer(FGPUBufferPtr DstBuffer, FGPUBufferPtr SrcBuffer) override;

		// Graphics
		virtual void SetGraphicsPipeline(FPipelinePtr InPipeline) override;
		virtual void SetVertexBuffer(FVertexBufferPtr InVertexBuffer, FInstanceBufferPtr InInstanceBuffer) override;
		virtual void SetIndexBuffer(FIndexBufferPtr InIndexBuffer) override;

		virtual void SetUniformBuffer(E_SHADER_STAGE ShaderStage, int32 BindIndex, FUniformBufferPtr InUniformBuffer) override;
		virtual void SetRenderResourceTable(int32 BindIndex, FRenderResourceTablePtr RenderResourceTable) override;
		virtual void SetArgumentBuffer(int32 InBindIndex, FArgumentBufferPtr InArgumentBuffer) override;

		virtual void SetGPUBufferState(FGPUBufferPtr GPUBuffer, EGPUResourceState NewState) override;
		virtual void SetGPUTextureState(FGPUTexturePtr GPUTexture, EGPUResourceState NewState) override;
		virtual void FlushBarriers() override;

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
		virtual void SetComputeConstant(int32 BindIndex, const void* ConstantData, int32 Size32Bit) override;
		virtual void SetComputeConstant(int32 BindIndex, const FUInt4& InValue) override;
		virtual void SetComputeConstant(int32 BindIndex, const FFloat4& InValue) override;
		virtual void SetComputeConstantBuffer(int32 BindIndex, FUniformBufferPtr InUniformBuffer, uint32 BufferOffset = 0) override;
		virtual void SetComputeUnorderedAccessResource(int32 BindIndex, FUniformBufferPtr InUniformBuffer, uint32 BufferOffset = 0) override;
		virtual void SetComputeShaderResource(int32 BindIndex, FUniformBufferPtr InUniformBuffer, uint32 BufferOffset = 0) override;
		virtual void SetComputeResourceTable(int32 BindIndex, FRenderResourceTablePtr RenderResourceTable) override;
		virtual void SetComputeArgumentBuffer(int32 BindIndex, FArgumentBufferPtr InArgumentBuffer) override;
		virtual void SetComputeTexture(int32 BindIndex, FTexturePtr InTexture) override;

		virtual void DispatchCompute(const FInt3& GroupSize, const FInt3& GroupCount) override;

		// RTX
		virtual void SetRtxPipeline(FRtxPipelinePtr RtxPipeline) override;
		virtual void TraceRays(FRtxPipelinePtr RtxPipeline, const FInt3& Size) override;

		// GPU Command buffer
		virtual void ExecuteGPUDrawCommands(FGPUCommandBufferPtr GPUCommandBuffer) override;
		virtual void ExecuteGPUComputeCommands(FGPUCommandBufferPtr GPUCommandBuffer) override;

		virtual void SetViewport(const FRecti& InViewport) override;
		virtual void SetScissorRect(const FRecti& InRect) override;
		virtual void BeginRenderToRenderTarget(FRenderTargetPtr RT, const int8* PassName = "UnnamedPass", uint32 MipLevel = 0) override;

		virtual void UAVBarrier(FBottomLevelAccelerationStructurePtr BLAS) override;

		virtual void UpdateD3D12Resource(
			_In_ ID3D12Resource* pDestinationResource,
			_In_ ID3D12Resource* pIntermediate,
			uint32 NumSubResources,
			const D3D12_SUBRESOURCE_DATA* pData
		);
		virtual void HoldResourceReference(FRenderResourcePtr InResource) override;
		virtual void HoldResourceReference(ComPtr<ID3D12Resource> InDxResource);
	private:
		FRHICmdListDx12(ERHICmdList InType);

		ID3D12CommandQueue* GetQueueForSwapChain()
		{
			return CommandQueue.Get();
		}

		// Dx12 specified
		void Init(FRHIDx12* RHIDx12, const TString& InNamePrefix, int32 BufferCount);
		void SetBackbufferTarget(D3D12_CPU_DESCRIPTOR_HANDLE InRTView, D3D12_CPU_DESCRIPTOR_HANDLE InDSView);

		void Transition(
			ID3D12Resource* pResource,
			D3D12_RESOURCE_STATES stateBefore,
			D3D12_RESOURCE_STATES stateAfter
		);
		void UAVBarrier(ID3D12Resource* pResource);
		
		void ReleaseAllResources();
		void ReleaseFrameResources(uint32 FrameIndex);

		uint64 UpdateSubresources(
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
			_In_ ID3D12Resource* pDestinationResource,
			_In_ ID3D12Resource* pIntermediate,
			uint64 IntermediateOffset,
			_In_range_(0, D3D12_REQ_SUBRESOURCES) uint32 FirstSubresource,
			_In_range_(0, D3D12_REQ_SUBRESOURCES - FirstSubresource) uint32 NumSubresources,
			_In_reads_(NumSubresources) const D3D12_SUBRESOURCE_DATA* pSrcData);

	protected:
		FRHIDx12* RHIDx12;

		TVector<ComPtr<ID3D12CommandAllocator>> CommandAllocators;
		ComPtr<ID3D12CommandQueue> CommandQueue;
		ComPtr<ID3D12GraphicsCommandList> CommandList;
		ComPtr<ID3D12Fence> CommandFence;

		uint64 ExecIndex;
		uint32 CurrentIndex;

		// Heaps Used
		TVector<FRHIHeap*> HeapsUsed;

		// CPU/GPU Synchronization.
		TVector<uint64> FenceValues;
		HANDLE FenceEvent;

		// Barriers
		TVector<D3D12_RESOURCE_BARRIER> Barriers;

		// Frame on the fly Resource holders
		TVector<FFrameResourcesDx12*> ResourceHolders;

		friend class FRHIDx12;
	};
}
#endif	// COMPILE_WITH_RHI_DX12