/*
	TiX Engine v3.0 Copyright (C) 2018~2021
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "NaniteEnableMeshShader.h"

#include "S_Rasterizer.hlsli"

#if !(NANITE_MESH_SHADER)
// Give something to compile, or else will get an compile error.
[numthreads(32, 1, 1)]
void HWRasterizeAS(
	uint GroupThreadID : SV_GroupThreadID,
	uint GroupID : SV_GroupID
)
{
}
#endif