/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

class FTranscodeCS : public FComputeTask
{
public:
	FTranscodeCS();
	virtual ~FTranscodeCS();

	virtual void Run(FRHICmdList* RHICmdList) override;

	void SetupParams(FRHICmdList* RHICmdList);

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

private:
	FRenderResourceTablePtr ResourceTable;

};
typedef TI_INTRUSIVE_PTR(FTranscodeCS) FTranscodeCSPtr;
