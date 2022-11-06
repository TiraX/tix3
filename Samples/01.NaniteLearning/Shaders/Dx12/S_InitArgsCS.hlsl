#define CULLING_PASS CULLING_PASS_OCCLUSION_MAIN
#define CHECK_AND_TRIM_CLUSTER_COUNT 1

#include "Common.hlsli"
#include "WaveOpUtil.h"
#include "NaniteDataDecode.h"

#define NANITE_HIERARCHY_TRAVERSAL 1

#include "NaniteCulling.h"
#include "NaniteDebugInfo.h"

RWStructuredBuffer<FQueueState> OutQueueState : register(u0);
RWBuffer< uint > InOutMainPassRasterizeArgsSWHW : register(u1);

#define InitArgsRS \
	"RootConstants(num32BitConstants=10, b0)," \
    "DescriptorTable(UAV(u0, numDescriptors=2))" 

[RootSignature(InitArgsRS)]
[numthreads(1, 1, 1)]
void InitArgs()
{
	OutQueueState[0].TotalClusters = 0;
	for (uint i = 0; i < 2; i++)
	{
		OutQueueState[0].PassState[i].ClusterBatchReadOffset = 0;
		OutQueueState[0].PassState[i].ClusterWriteOffset = 0;
		OutQueueState[0].PassState[i].NodeReadOffset = 0;
		OutQueueState[0].PassState[i].NodeWriteOffset = 0;
		OutQueueState[0].PassState[i].NodePrevWriteOffset = 0;
		OutQueueState[0].PassState[i].NodeCount = 0;
	}

	const uint ArgsOffset = 0u;
	WriteRasterizerArgsSWHW(InOutMainPassRasterizeArgsSWHW, ArgsOffset, 0, 0);

}