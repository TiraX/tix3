/*
	TiX Engine v2.0 Copyright (C) 2018~2021
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

class TNaniteMesh
{
public:
	~TNaniteMesh() = default;
	static TNaniteMesh* LoadMesh();

protected:
	TNaniteMesh();
	static bool ConvertNanieMesh();
};