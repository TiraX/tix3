/*
	TiX Engine v2.0 Copyright (C) 2018~2021
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

#define NANITE_MAX_CLUSTER_TRIANGLES						128

#define NANITE_MAX_CLUSTERS_PER_GROUP_TARGET				128

struct RawVertex
{
	FFloat3 Pos;
	FFloat3 Nor;
	FFloat2 UV;
};