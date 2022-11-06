#define CULLING_PASS CULLING_PASS_OCCLUSION_MAIN
#define CHECK_AND_TRIM_CLUSTER_COUNT 1

#include "Common.hlsli"
#include "WaveOpUtil.h"
#include "NaniteDataDecode.h"

RWByteAddressBuffer	MainAndPostCandididateClusters : register(u0);


#define ClearCandidateBufferRS \
	"UAV(u0)" 

[RootSignature(ClearCandidateBufferRS)]
[numthreads(128, 1, 1)]
void ClearCandidateBuffer(uint DispatchThreadId : SV_DispatchThreadID)
{
	uint address = DispatchThreadId * 16 * 4;
	uint4 value = 0;
	MainAndPostCandididateClusters.Store4(address, value);
	MainAndPostCandididateClusters.Store4(address + 16, value);
	MainAndPostCandididateClusters.Store4(address + 16 * 2, value);
	MainAndPostCandididateClusters.Store4(address + 16 * 3, value);
}
