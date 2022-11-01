/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

struct FNaniteDebug
{
	static const int32 MaxDebugInfo = 16;
	FUInt4 NodeData[NANITE_MAX_BVH_NODES_PER_GROUP];
	uint32 NodeIndex[NANITE_MAX_BVH_NODES_PER_GROUP];
	uint32 GroupNodeBatchStartIndex;

	FNaniteDebug()
		: GroupNodeBatchStartIndex(0)
	{
		memset(NodeIndex, 0, sizeof(NodeIndex));
	}
};


FUniformBufferPtr CreateDebugInfoUniform(FRHICmdList* RHICmdList, int32 Capcity = 1);
