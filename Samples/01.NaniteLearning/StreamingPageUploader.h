/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once
#include "TranscodeCS.h"
#include "NaniteMesh.h"
#include "ScatterUploadCS.h"

class FStreamingPageUploader
{
public:
	FStreamingPageUploader();
	~FStreamingPageUploader();

	void Init(FRHICmdList* RHICmdList);
	void ProcessNewResources(FRHICmdList* RHICmdLists, TNaniteMesh* NaniteMesh, FUniformBufferPtr DstBuffer);

	static FUniformBufferPtr AllocateClusterPageBuffer(FRHICmdList* RHICmdList);
	static FUniformBufferPtr AllocateHierarchyBuffer(FRHICmdList* RHICmdList, const TVector<FPackedHierarchyNode>& HierarchyNodes);
private:
	void ApplyFixups(const FFixupChunk& FixupChunk, TNaniteMesh* NaniteMesh, const TVector<uint8>& BulkData);
	void RedirectClusterIdForClusterInstances(TNaniteMesh* NaniteMesh);

private:
	FUniformBufferPtr InstallInfoUploadBuffer;
	FUniformBufferPtr PageDependenciesBuffer;
	FUniformBufferPtr PageUploadBuffer;

	THMap<uint32, const FFixupChunk*> FixupMap;	// key = GPUPageIndex, value = FixupPtr

	FTranscodeCSPtr TranscodeCS;
	FScatterUploadCSPtr ClusterFixupUploadCS;
};
