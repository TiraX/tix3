/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once
#include "NaniteMesh.h"

class FFakeInstanceCullCS : public FComputeTask
{
public:
	FFakeInstanceCullCS();
	virtual ~FFakeInstanceCullCS();

	virtual void Run(FRHICmdList* RHICmdList) override;

	void ApplyParameters(
		FRHICmdList* RHICmdList,
		const FDecodeInfo& InDecodeInfo,
		FUniformBufferPtr InQueueState,
		FUniformBufferPtr InNodesAndClusterBatches
		);
private:
	enum
	{
		RC_DecodeInfo,
		RT_Table,
	};

	enum
	{
		UAV_QueueState,
		UAV_MainAndPostNodesAndClusterBatches,

		NumParams
	};

private:
	FDecodeInfo DecodeInfo;

	FRenderResourceTablePtr ResourceTable;

	FUniformBufferPtr QueueState;
	FUniformBufferPtr NodesAndClusterBatches;

};
typedef TI_INTRUSIVE_PTR(FFakeInstanceCullCS) FFakeInstanceCullCSPtr;
