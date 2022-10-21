/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

class FTranscode : public FComputeTask
{
public:
	FTranscode();
	virtual ~FTranscode();

	virtual void Run(FRHICmdList* RHICmdList) override;

private:

private:

};
