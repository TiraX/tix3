/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once
#include "StreamingPageUploader.h"
#include "InitCandidateNodesCS.h"
#include "InitClusterBatchesCS.h"
#include "FakeInstanceCullCS.h"
#include "PersistentCullCS.h"
#include "InitArgsCS.h"
#include "ClearCandidateBufferCS.h"
#include "ClearVisBufferCS.h"

struct FTessedDataStruct
{
	FFloat3 P;
	uint32 N;
	uint32 UV;
};

class FNaniteLearningRenderer : public FDefaultRenderer
{
public:
	FNaniteLearningRenderer(FSceneInterface* Scene);
	virtual ~FNaniteLearningRenderer();

	virtual void InitInRenderThread() override;
	virtual void Render(FRHICmdList* RHICmdList) override;

	static void FreezeCulling();

private:
	void CalcLoopIndexData();

private:
	FArgumentBufferPtr AB_Result;

	FFullScreenRender FSRender;
	FRenderTargetPtr RT_BasePass;

	FUniformBufferPtr ClusterPageData;
	FUniformBufferPtr Hierarchy;
	FUniformBufferPtr View;
	FUniformBufferPtr QueueState;
	FUniformBufferPtr MainAndPostNodesAndClusterBatchesBuffer;
	FUniformBufferPtr MainAndPostCandididateClustersBuffer;
	FUniformBufferPtr VisibleClustersSWHW;
	FUniformBufferPtr VisibleClustersArgsSWHW;

	FUniformBufferPtr TessedData;

	FUniformBufferPtr CullingDebugInfo;
	FUniformBufferPtr TessDebugInfo;
	FUniformBufferPtr TessDebugTable;

	TNaniteMesh* NaniteMesh;
	FStreamingPageUploader StreamingManager;

	FInitCandidateNodesCSPtr InitCandidateNodesCS;
	FInitClusterBatchesCSPtr InitClusterBatchesCS;
	FInitArgsCSPtr InitArgsCS;
	FClearCandidateBufferCSPtr ClearCandidateBufferCS;
	FFakeInstanceCullCSPtr FakeInstanceCullCS;
	FPersistentCullCSPtr PersistentCullCS;
	FClearVisBufferCSPtr ClearVisBufferCS;


	enum
	{
		RC_DecodeInfo,
		RT_Table,
	};
	enum
	{
		SRV_ClusterPageData,
		SRV_HierarchyBuffer,
		SRV_VisibleClusterSWHW,
		SRV_Views,
		SRV_TexHeight,

		UAV_TessedData,
		UAV_VisBuffer,
		UAV_TessDebugInfo,
		UAV_TessDebugTable,

		NumHWRasterizeParams
	};
	FPipelinePtr PL_HWRasterizer;
	FTexturePtr VisBuffer;
	FRenderResourceTablePtr RT_HWRasterize;
	FGPUCommandSignaturePtr CmdSig_HWRasterize;
};
