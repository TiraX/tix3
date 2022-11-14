/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

class TNaniteLearningTicker : public TTicker, public TEventHandler
{
public:
	TNaniteLearningTicker();
	virtual ~TNaniteLearningTicker();

	virtual void Tick(float Dt) override;

	virtual bool OnEvent(const TEvent& E) override;

	void SetupScene();

protected:
};