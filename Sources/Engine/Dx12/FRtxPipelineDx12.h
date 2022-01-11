/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

#if COMPILE_WITH_RHI_DX12

namespace tix
{
	class FRtxPipelineDx12 : public FRtxPipeline
	{
	public:
		FRtxPipelineDx12(FShaderPtr InShader);
		virtual ~FRtxPipelineDx12();
	protected:

	private:
		ComPtr<ID3D12StateObject> StateObject;
		FUniformBufferPtr ShaderTable;
		FInt2 RayGenShaderOffsetAndSize;
		FInt2 MissShaderOffsetAndSize;
		FInt2 HitGroupOffsetAndSize;

		friend class FRHIDx12;
	};
}

#endif	// COMPILE_WITH_RHI_DX12
