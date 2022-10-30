/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once
#include "TranscodeCS.h"

class TNaniteMesh;
class FStreamingPageUploader
{
public:
	FStreamingPageUploader();
	~FStreamingPageUploader();

	void Init(FRHICmdList* RHICmdList);
	void ProcessNewResources(FRHICmdList* RHICmdLists, TNaniteMesh* NaniteMesh, FUniformBufferPtr DstBuffer);

	static FUniformBufferPtr AllocateClusterPageBuffer(FRHICmdList* RHICmdList);
	static int32 GetMaxStreamingPages();
private:

private:
	FUniformBufferPtr InstallInfoUploadBuffer;
	FUniformBufferPtr PageDependenciesBuffer;
	FUniformBufferPtr PageUploadBuffer;

	FTranscodeCSPtr TranscodeCS;
};
