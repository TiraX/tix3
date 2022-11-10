/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once
#include "NaniteMesh.h"

class FClearVisBufferCS : public FComputeTask
{
public:
	FClearVisBufferCS();
	virtual ~FClearVisBufferCS();

	void ApplyParameters(
		FRHICmdList* RHICmdList,
		const FInt2& InSize,
		FTexturePtr InVisBuffer
	);
	virtual void Run(FRHICmdList* RHICmdList) override;


private:

private:
	FRenderResourceTablePtr ResourceTable;
	FInt2 Size;
	FTexturePtr VisBuffer;

};
typedef TI_INTRUSIVE_PTR(FClearVisBufferCS) FClearVisBufferCSPtr;
