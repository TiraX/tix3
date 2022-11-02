/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once
#include "NaniteMesh.h"

class FInitClusterBatchesCS : public FComputeTask
{
public:
	FInitClusterBatchesCS();
	virtual ~FInitClusterBatchesCS();

	virtual void Run(FRHICmdList* RHICmdList) override;

	void ApplyParameters(
		FRHICmdList* RHICmdList,
		const FDecodeInfo& InDecodeInfo,
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
		UAV_MainAndPostNodesAndClusterBatches,

		NumParams
	};

private:
	FDecodeInfo DecodeInfo;

	FRenderResourceTablePtr ResourceTable;

	FUniformBufferPtr NodesAndClusterBatches;

};
typedef TI_INTRUSIVE_PTR(FInitClusterBatchesCS) FInitClusterBatchesCSPtr;
