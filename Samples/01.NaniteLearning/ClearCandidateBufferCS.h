/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once
#include "NaniteMesh.h"

class FClearCandidateBufferCS : public FComputeTask
{
public:
	FClearCandidateBufferCS();
	virtual ~FClearCandidateBufferCS();

	void ApplyParameters(
		FRHICmdList* RHICmdList,
		FUniformBufferPtr InCandididateClusters
	);
	virtual void Run(FRHICmdList* RHICmdList) override;

	enum
	{
		UAV_CandididateClusters,

		NumParams,
	};



private:

private:
	FUniformBufferPtr CandididateClusters;

};
typedef TI_INTRUSIVE_PTR(FClearCandidateBufferCS) FClearCandidateBufferCSPtr;
