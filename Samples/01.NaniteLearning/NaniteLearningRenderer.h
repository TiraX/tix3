/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

class FNaniteLearningRenderer : public FDefaultRenderer
{
public:
	FNaniteLearningRenderer(FSceneInterface* Scene);
	virtual ~FNaniteLearningRenderer();

	virtual void InitInRenderThread() override;
	virtual void Render(FRHICmdList* RHICmdList) override;

private:

private:
	FArgumentBufferPtr AB_Result;

	FFullScreenRender FSRender;
	FRenderTargetPtr RT_BasePass;
};
