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

	void SetupParams(FRHICmdList* RHICmdList);
private:
	enum
	{
		SRV_InstallInfoBuffer,
		SRV_PageDependenciesBuffer,
		SRV_SrcPageBuffer,

		UAV_DstPageBuffer,

		PARAM_NUM,
	};

	enum
	{
		RC_StartPageIndex,
		RT_Table,
	};

private:
	FRenderResourceTablePtr ResourceTable;

};
