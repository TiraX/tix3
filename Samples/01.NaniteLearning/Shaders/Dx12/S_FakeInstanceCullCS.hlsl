#define CULLING_PASS CULLING_PASS_OCCLUSION_MAIN
#define CHECK_AND_TRIM_CLUSTER_COUNT 1

#include "Common.hlsli"
#include "WaveOpUtil.h"
#include "NaniteDataDecode.h"

#define NANITE_HIERARCHY_TRAVERSAL 1

#include "NaniteCulling.h"
#include "NaniteDebugInfo.h"

RWStructuredBuffer<FQueueState> OutQueueState : register(u0);
RWByteAddressBuffer	MainAndPostNodesAndClusterBatches : register(u1);




#define InitClusterBatchesRS \
	"RootConstants(num32BitConstants=10, b0)," \
    "DescriptorTable(UAV(u0, numDescriptors=2))" 

[RootSignature(InitClusterBatchesRS)]
[numthreads(64, 1, 1)]
void FakeInstanceCull(uint GroupIndex : SV_GroupIndex, uint3 GroupId : SV_GroupID)
{
	const uint DispatchIndex = GroupId.x * 64 + GroupIndex;
	const bool bIsPostPass = false;

	{
		uint InstanceId = DispatchIndex;
		uint ViewId = 0;
		{

			// tix : only 1 instance, always visible
			bool bIsVisible = GroupIndex == 0;
			if (bIsVisible)
			{

				uint NodeOffset = 0;
				uint QueueStateIndex = 0;//(CULLING_PASS == CULLING_PASS_OCCLUSION_POST);
				WaveInterlockedAddScalar_(OutQueueState[0].PassState[QueueStateIndex].NodeWriteOffset, 1, NodeOffset);
				WaveInterlockedAddScalar(OutQueueState[0].PassState[QueueStateIndex].NodeCount, 1);


				if (NodeOffset < DecodeInfo.MaxNodes)
				{
					uint Flags = NANITE_CULLING_FLAG_TEST_LOD;
//#if CULLING_PASS == CULLING_PASS_OCCLUSION_POST
//					Flags |= NANITE_CULLING_FLAG_FROM_DISOCCLUDED_INSTANCE;
//#endif
					//if (bEnableWPO)
					//{
					//	Flags |= NANITE_CULLING_FLAG_ENABLE_WPO;
					//}

					FCandidateNode Node;
					Node.Flags = Flags;
					Node.ViewId = ViewId;
					Node.InstanceId = InstanceId;
					Node.NodeIndex = 0;
					Node.EnabledBitmask = NANITE_BVH_NODE_ENABLE_MASK;
					StoreCandidateNode(MainAndPostNodesAndClusterBatches, NodeOffset, Node, bIsPostPass);
				}
			}
		}
	}
}
