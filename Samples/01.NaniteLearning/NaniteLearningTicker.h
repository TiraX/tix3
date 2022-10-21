/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once
#include "NaniteMesh.h"

class TNaniteLearningTicker : public TTicker
{
public:
	TNaniteLearningTicker();
	virtual ~TNaniteLearningTicker();

	virtual void Tick(float Dt) override;

	void SetupScene();

protected:
	TNaniteMesh* NaniteMesh;
};