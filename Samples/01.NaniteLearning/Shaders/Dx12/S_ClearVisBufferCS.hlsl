/*
	TiX Engine v3.0 Copyright (C) 2018~2021
	By ZhaoShuai tirax.cn@gmail.com
*/

// NaniteLearning Sample


// Params
uint2 Size : register(b0);
RWTexture2D<uint2>	VisBuffer64 : register(u0);

#define ClearVisBufferRS \
	"RootConstants(num32BitConstants=2, b0)," \
    "DescriptorTable(UAV(u0, numDescriptors=1))" 

[RootSignature(ClearVisBufferRS)]
[numthreads(16, 16, 1)]
void ClearVisBufferCS( uint3 DispatchThreadId : SV_DispatchThreadID ) 
{
	uint2 Index = DispatchThreadId.xy;
	if (Index.x >= Size.x || Index.y >= Size.y)
		return;
	
	VisBuffer64[Index] = uint2(0, 0);
}