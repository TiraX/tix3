/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	// Define in a single file for reference in different project.
	const int32 TVertexBuffer::SemanticSize[ESSI_TOTAL] =
	{
		12,	// ESSI_POSITION,
		4,	// ESSI_NORMAL,
		4,	// ESSI_COLOR,
		4,	// ESSI_TEXCOORD0,
		4,	// ESSI_TEXCOORD1,
		4,	// ESSI_TANGENT,
		4,	// ESSI_BLENDINDEX,
		4,	// ESSI_BLENDWEIGHT,
	};

	const int8* TVertexBuffer::SemanticName[ESSI_TOTAL] =
	{
		"POSITION",		// ESSI_POSITION,
		"NORMAL",		// ESSI_NORMAL,
		"COLOR",		// ESSI_COLOR,
		"TEXCOORD",		// ESSI_TEXCOORD0,
		"TEXCOORD",		// ESSI_TEXCOORD1,
		"TANGENT",		// ESSI_TANGENT,
		"BLENDINDICES",	// ESSI_BLENDINDEX,
		"BLENDWEIGHT",	// ESSI_BLENDWEIGHT,
	};

	const int32 TVertexBuffer::SemanticIndex[ESSI_TOTAL] =
	{
		0,		// ESSI_POSITION,
		0,		// ESSI_NORMAL,
		0,		// ESSI_COLOR,
		0,		// ESSI_TEXCOORD0,
		0,		// ESSI_TEXCOORD1,
		0,		// ESSI_TANGENT,
		0,		// ESSI_BLENDINDEX,
		0,		// ESSI_BLENDWEIGHT,
	};

	/////////////////////////////////////////////////////////////
	const int32 TInstanceBuffer::SemanticSize[EISI_TOTAL] =
	{
		16,	// EISI_TRANSFORM0,
		16,	// EISI_TRANSFORM1,
		16,	// EISI_TRANSFORM2,
		16,	// EISI_CUSTOM0,
	};

	const int8* TInstanceBuffer::SemanticName[EISI_TOTAL] =
	{
		"INS_TRANSFORM",	// EISI_TRANSFORM0,
		"INS_TRANSFORM",	// EISI_TRANSFORM1,
		"INS_TRANSFORM",	// EISI_TRANSFORM2,
		"INS_CUSTOM",	// EISI_CUSTOM0,
	};

	const int32 TInstanceBuffer::SemanticIndex[EISI_TOTAL] =
	{
		0,		// EISI_TRANSFORM0,
		1,		// EISI_TRANSFORM1,
		2,		// EISI_TRANSFORM2,
		0,	// EISI_CUSTOM0,
	};
}
