/*
	TiX Engine v3.0 Copyright (C) 2018~2021
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "NaniteEnableMeshShader.h"

#if NANITE_MESH_SHADER
#include "S_RasterizerTess.hlsli"
#else
float4 HWRasterizePS() : SV_Target0
{
	return float4(0.5, 0.5, 0.0, 1.0);
}
#endif

