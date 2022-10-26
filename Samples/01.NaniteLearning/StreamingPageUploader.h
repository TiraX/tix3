/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

class TNaniteMesh;
class FStreamingPageUploader
{
public:
	FStreamingPageUploader();
	~FStreamingPageUploader();

	void Init(FRHICmdList* RHICmdList);
	void ProcessNewResources(FRHICmdList* RHICmdLists, TNaniteMesh* NaniteMesh);
private:

private:
	FUniformBufferPtr InstallInfoUploadBuffer;
	FUniformBufferPtr PageDependenciesBuffer;
	FGPUBufferPtr PageUploadBuffer;

};
