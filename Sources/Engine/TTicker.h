/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	class TTicker
	{
	public:
        TTicker() {}
        virtual ~TTicker() {}
        
		virtual void Tick(float Dt) = 0;
	};
}
