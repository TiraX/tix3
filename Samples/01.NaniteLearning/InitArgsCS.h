/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once
#include "NaniteMesh.h"

class FInitArgsCS : public FComputeTask
{
public:
	FInitArgsCS();
	virtual ~FInitArgsCS();

	void ApplyParameters(
		FRHICmdList* RHICmdList,
		const FDecodeInfo& DecodeInfo,
		FUniformBufferPtr InQueueState,
		FUniformBufferPtr InVisibleClustersArgsSWHW
	);
	virtual void Run(FRHICmdList* RHICmdList) override;

	enum
	{
		RC_DecodeInfo,
		RT_Table,
	};
	enum
	{
		UAV_QueueState,
		UAV_VisibleClustersArgsSWHW,

		NumParams,
	};



private:

private:
	FDecodeInfo DecodeInfo;
	FRenderResourceTablePtr ResourceTable;
	FUniformBufferPtr QueueState;
	FUniformBufferPtr VisibleClustersArgsSWHW;

};
typedef TI_INTRUSIVE_PTR(FInitArgsCS) FInitArgsCSPtr;
