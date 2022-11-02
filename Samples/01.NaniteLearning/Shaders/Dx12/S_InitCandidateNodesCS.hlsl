#define CULLING_PASS CULLING_PASS_OCCLUSION_MAIN
#define CHECK_AND_TRIM_CLUSTER_COUNT 1

#include "Common.hlsli"
#include "WaveOpUtil.h"
#include "NaniteDataDecode.h"

#define NANITE_HIERARCHY_TRAVERSAL 1

#include "NaniteCulling.h"
#include "NaniteDebugInfo.h"

RWByteAddressBuffer	MainAndPostNodesAndClusterBatches : register(u0);




#define InitCandidateNodesRS \
	"RootConstants(num32BitConstants=10, b0)," \
    "DescriptorTable(UAV(u0, numDescriptors=1))" 

[RootSignature(InitCandidateNodesRS)]
[numthreads(64, 1, 1)]
void InitCandidateNodes(uint GroupIndex : SV_GroupIndex, uint3 GroupId : SV_GroupID)
{
	const uint Index = GroupId.x * 64 + GroupIndex;
	// GetUnWrappedDispatchThreadId(GroupId, GroupIndex, 64);
	if(Index < DecodeInfo.MaxNodes)
	{
		ClearCandidateNode(MainAndPostNodesAndClusterBatches, Index, false);
		ClearCandidateNode(MainAndPostNodesAndClusterBatches, Index, true);
	}
}