/*
	TiX Engine v3.0 Copyright (C) 2018~2021
	By ZhaoShuai tirax.cn@gmail.com
*/

// NaniteLearning Sample
struct FScatterInfo
{
	uint Count;
};

// Params
FScatterInfo ScatterInfo : register(b0);

StructuredBuffer<uint> Addresses : register(t0);
StructuredBuffer<uint> Values : register(t1);

RWByteAddressBuffer DstBuffer : register(u0);



#define TranscodeRS \
	"RootConstants(num32BitConstants=1, b0)," \
	"SRV(t0)," \
	"SRV(t1)," \
	"UAV(u0)," 

[RootSignature(TranscodeRS)]
[numthreads(64, 1, 1)]
void ScatterCopyCS( uint DispatchThreadId : SV_DispatchThreadID ) 
{
	uint ThreadId = DispatchThreadId;
	if (ThreadId < ScatterInfo.Count)
	{
		uint address = Addresses[ThreadId];
		uint value = Values[ThreadId];

		DstBuffer.Store(address * 4, value);
	}
}