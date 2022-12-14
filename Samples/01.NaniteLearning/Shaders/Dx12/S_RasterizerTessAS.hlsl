/*
	TiX Engine v3.0 Copyright (C) 2018~2021
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "NaniteEnableMeshShader.h"

#if (NANITE_MESH_SHADER)
#include "S_RasterizerTess.hlsli"
#else
// Give something to compile, or else will get an compile error.
struct Payload
{
	uint Data;
};
[numthreads(32, 1, 1)]
void HWRasterizeAS(
	uint GroupThreadID : SV_GroupThreadID,
	uint GroupID : SV_GroupID
)
{
	Payload P;
	DispatchMesh(0, 1, 1, P);
}
#endif