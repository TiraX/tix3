/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "FFullScreenRender.h"

namespace tix
{
	FFullScreenRender::FFullScreenRender()
		: bInited(false)
	{}

	FFullScreenRender::~FFullScreenRender()
	{
		FullScreenVB = nullptr;
		FullScreenIB = nullptr;
		FullScreenPipeline = nullptr;
		FullScreenShader = nullptr;
	}

	void FFullScreenRender::InitCommonResources(FRHI* RHI)
	{
		if (bInited)
			return;

		// Create full screen quad
		const float16 half0 = float16(0.f);
		const float16 half1 = float16(1.f);
		static const FullScreenVertex FullScreenQuadVertices[4] = {
			{FFloat3(-1.f, -1.f, 0.f), FHalf2(half0, half1)},
			{FFloat3(1.f, -1.f, 0.f), FHalf2(half1, half1)},
			{FFloat3(-1.f, 1.f, 0.f), FHalf2(half0, half0)},
			{FFloat3(1.f, 1.f, 0.f), FHalf2(half1, half0)}
		};
		static const uint16 FullScreenQuadIndices[6] = {
			0, 2, 1, 1, 2, 3
		};

		TVertexBufferPtr VBData = ti_new TVertexBuffer();
		VBData->SetResourceName("FullScreenVB");
		VBData->SetVertexData(EVSSEG_POSITION | EVSSEG_TEXCOORD0, FullScreenQuadVertices, 4, FBox());
		TIndexBufferPtr IBData = ti_new TIndexBuffer();
		IBData->SetResourceName("FullScreenIB");
		IBData->SetIndexData(EIT_16BIT, FullScreenQuadIndices, 6);
		FullScreenVB = ti_new FVertexBuffer(VBData->GetDesc());
		FullScreenIB = ti_new FIndexBuffer(IBData->GetDesc());
		FullScreenVB->CreateGPUBuffer(VBData->GetVertexBufferData());
		FullScreenIB->CreateGPUBuffer(IBData->GetIndexBufferData());
		VBData = nullptr;
		IBData = nullptr;

		// Create full screen render pipeline
		TMaterialPtr FSMaterial = ti_new TMaterial();
		FSMaterial->SetResourceName("FullScreenMaterial");

		TShaderNames ShaderNames;
		ShaderNames.ShaderNames[ESS_VERTEX_SHADER] = "FullScreenVS";
		ShaderNames.ShaderNames[ESS_PIXEL_SHADER] = "FullScreenPS";

		// Move this TShader load to Game Thread. Or Make a gloal shader system.
		TShaderPtr Shader = ti_new TShader(ShaderNames);
		Shader->LoadShaderCode();
		Shader->ShaderResource = RHI->CreateShader(ShaderNames, EST_RENDER);
		RHI->UpdateHardwareResourceShader(Shader->ShaderResource, Shader);
		FSMaterial->SetShader(Shader);
		FullScreenShader = Shader->ShaderResource;

        FSMaterial->EnableTwoSides(true);
		FSMaterial->EnableDepthWrite(false);
		FSMaterial->EnableDepthTest(false);
		FSMaterial->SetShaderVsFormat(FullScreenVB->GetDesc().VsFormat);
		FSMaterial->SetRTColor(FRHIConfig::DefaultBackBufferFormat, ERTC_COLOR0);

		// Pipeline
		FullScreenPipeline = RHI->CreatePipeline(FullScreenShader);
		RHI->UpdateHardwareResourcePL(FullScreenPipeline, FSMaterial);
		FSMaterial = nullptr;

		bInited = true;
	}

	void FFullScreenRender::DrawFullScreenTexture(FRHI* RHI, FTexturePtr Texture)
	{
		TI_ASSERT(0);
	}

	void FFullScreenRender::DrawFullScreenTexture(FRHI* RHI, FRenderResourceTablePtr TextureTable)
	{
		TI_ASSERT(bInited);
		RHI->SetVertexBuffer(FullScreenVB, nullptr);
		RHI->SetIndexBuffer(FullScreenIB);
		RHI->SetGraphicsPipeline(FullScreenPipeline);
		RHI->SetRenderResourceTable(0, TextureTable);

		RHI->DrawPrimitiveIndexedInstanced(FullScreenIB->GetDesc().IndexCount, 1, 0);
	}

	void FFullScreenRender::DrawFullScreenTexture(FRHI* RHI, FArgumentBufferPtr ArgumentBuffer)
	{
		TI_ASSERT(bInited);
        RHI->SetGraphicsPipeline(FullScreenPipeline);
		RHI->SetVertexBuffer(FullScreenVB, nullptr);
		RHI->SetIndexBuffer(FullScreenIB);
		RHI->SetArgumentBuffer(0, ArgumentBuffer);

		RHI->DrawPrimitiveIndexedInstanced(FullScreenIB->GetDesc().IndexCount, 1, 0);
	}

	void FFullScreenRender::DrawFullScreenQuad(FRHI* RHI)
	{
		TI_ASSERT(bInited);
		RHI->SetVertexBuffer(FullScreenVB, nullptr);
		RHI->SetIndexBuffer(FullScreenIB);
		RHI->DrawPrimitiveIndexedInstanced(FullScreenIB->GetDesc().IndexCount, 1, 0);
	}
}
