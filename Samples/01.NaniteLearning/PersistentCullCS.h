/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once
#include "NaniteMesh.h"

class FPersistentCullCS : public FComputeTask
{
public:
	FPersistentCullCS();
	virtual ~FPersistentCullCS();

	virtual void Run(FRHICmdList* RHICmdList) override;

	void ApplyParameters(
		FRHICmdList* RHICmdList,
		const FDecodeInfo& InDecodeInfo,
		FUniformBufferPtr InClusterPageData,
		FUniformBufferPtr InHierachyBuffer,
		FUniformBufferPtr InBuffer1,
		FUniformBufferPtr InBuffer2,
		FUniformBufferPtr InQueueState,
		FUniformBufferPtr InClusterBatches,
		FUniformBufferPtr InCandididateClusters,
		FUniformBufferPtr InStreamingRequest,
		FUniformBufferPtr InVisibleClustersSWHW,
		FUniformBufferPtr InVisibleClustersArgsSWHW
		);
private:
	enum
	{
		RC_DecodeInfo,
		RT_Table,
	};

	enum
	{
		SRV_ClusterPageData,
		SRV_HierachyBuffer,
		SRV_UNKNOWN1,
		SRV_UNKNOWN2,
		
		UAV_QueueState,
		UAV_MainAndPostNodesAndClusterBatches,
		UAV_MainAndPostCandididateClusters,
		UAV_OutStreamingRequests,
		UAV_OutVisibleClustersSWHW,
		UAV_VisibleClustersArgsSWHW,

		NumParams
	};

private:
	FDecodeInfo DecodeInfo;

	FRenderResourceTablePtr ResourceTable;

	FUniformBufferPtr ClusterPageData;
	FUniformBufferPtr HierachyBuffer;
	FUniformBufferPtr Buffer1;
	FUniformBufferPtr Buffer2;
	FUniformBufferPtr QueueState;
	FUniformBufferPtr ClusterBatches;
	FUniformBufferPtr CandididateClusters;
	FUniformBufferPtr StreamingRequest;
	FUniformBufferPtr VisibleClustersSWHW;
	FUniformBufferPtr VisibleClustersArgsSWHW;

};
typedef TI_INTRUSIVE_PTR(FPersistentCullCS) FPersistentCullCSPtr;
