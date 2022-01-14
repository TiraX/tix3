/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "FDefaultRenderer.h"
#include "FUniformBufferView.h"

namespace tix
{
	FDefaultRenderer::FDefaultRenderer()
	{
	}

	FDefaultRenderer::~FDefaultRenderer()
	{
		TI_ASSERT(IsRenderThread());
	}

	void FDefaultRenderer::InitInRenderThread()
	{
		// Create Zero reset command buffer
		CounterResetUniformBuffer = ti_new FUniformBuffer(sizeof(uint32) * 4, 1, (uint32)EGPUResourceFlag::Intermediate);
		uint32 ZeroData[4];
		memset(ZeroData, 0, sizeof(ZeroData));
		TStreamPtr Data = ti_new TStream(ZeroData, sizeof(ZeroData));
		CounterResetUniformBuffer->CreateGPUBuffer(Data);
	}

	void FDefaultRenderer::InitRenderFrame(FScene* Scene)
	{
		// Prepare frame view uniform buffer
		Scene->InitRenderFrame();

		TI_TODO("Remove draw list , use new gpu driven pipeline.");
		// Prepare frame primitive uniform buffers
		//for (int32 List = 0; List < LIST_COUNT; ++List)
		//{
		//	const TVector<FPrimitivePtr>& Primitives = Scene->GetStaticDrawList((E_DRAWLIST_TYPE)List);
		//	for (auto& Primitive : Primitives)
		//	{
		//		if (Primitive->IsPrimitiveBufferDirty())
		//		{
		//			Primitive->UpdatePrimitiveBuffer_RenderThread();
		//		}
		//	}
		//}
	}

	void FDefaultRenderer::EndRenderFrame(FScene* Scene)
	{
		// Clear the flags in this frame.
		Scene->ClearSceneFlags();
	}

	void FDefaultRenderer::Render(FRHI* RHI, FScene* Scene)
	{
		RHI->BeginRenderToFrameBuffer();
		DrawSceneTiles(RHI, Scene);
	}

	void FDefaultRenderer::DrawSceneTiles(FRHI* RHI, FScene* Scene)
	{
		const THMap<FInt2, FSceneTileResourcePtr>& SceneTileResources = Scene->GetSceneTiles();
		for (auto& TileIter : SceneTileResources)
		{
			const FInt2& TilePos = TileIter.first;
			FSceneTileResourcePtr TileRes = TileIter.second;

			const TVector<FPrimitivePtr>& TilePrimitives = TileRes->GetPrimitives();
			for (uint32 PIndex = 0; PIndex < (uint32)TilePrimitives.size(); ++PIndex)
			{
				FPrimitivePtr Primitive = TilePrimitives[PIndex];

				if (Primitive != nullptr)
				{
					FInstanceBufferPtr InstanceBuffer = Primitive->GetInstanceBuffer();
					FVertexBufferPtr VB = Primitive->GetVertexBuffer();
					FIndexBufferPtr IB = Primitive->GetIndexBuffer();
					for (int32 S = 0; S < Primitive->GetNumSections(); S++)
					{
						const FPrimitive::FSection& Section = Primitive->GetSection(S);
						RHI->SetGraphicsPipeline(Section.Pipeline);
						RHI->SetVertexBuffer(VB, InstanceBuffer);
						RHI->SetIndexBuffer(IB);
						ApplyShaderParameter(RHI, Scene, Primitive, S);
						RHI->DrawPrimitiveIndexedInstanced(
							Section.Triangles * 3,
							InstanceBuffer == nullptr ? 1 : Primitive->GetInstanceCount(),
							Section.IndexStart,
							Primitive->GetInstanceOffset());
					}
				}
			}
		}
	}

	void FDefaultRenderer::BindEngineBuffer(FRHI * RHI, E_SHADER_STAGE ShaderStage, const FShaderBinding::FShaderArgument& Argument, FScene * Scene, FPrimitivePtr Primitive, int32 SectionIndex)
	{
		switch (Argument.ArgumentType)
		{
		case ARGUMENT_UNKNOWN:
			// A custom argument, do nothing here.
			break;
		case ARGUMENT_EB_VIEW:
			RHI->SetUniformBuffer(ShaderStage, Argument.BindingIndex, Scene->GetViewUniformBuffer()->UniformBuffer);
			break;
		case ARGUMENT_EB_PRIMITIVE:
			TI_ASSERT(Primitive != nullptr);
			RHI->SetUniformBuffer(ShaderStage, Argument.BindingIndex, Primitive->GetPrimitiveUniform()->UniformBuffer);
			break;
		case ARGUMENT_EB_BONES:
			TI_ASSERT(Primitive->GetSection(SectionIndex).SkeletonResourceRef != nullptr);
			RHI->SetUniformBuffer(ShaderStage, Argument.BindingIndex, Primitive->GetSection(SectionIndex).SkeletonResourceRef);
			break;
		case ARGUMENT_EB_LIGHTS:
			RHI->SetUniformBuffer(ShaderStage, Argument.BindingIndex, Scene->GetSceneLights()->GetSceneLightsUniform()->UniformBuffer);
			break;
		case ARGUMENT_EB_ENV_CUBE:
		{
			TI_TODO("Env Light should use the nearest with Primitive, associate with Primitive Buffer. Remove ARGUMENT_EB_ENV_CUBE in future");
			FEnvLightPtr EnvLight = Scene->FindNearestEnvLight(FFloat3());
			RHI->SetRenderResourceTable(Argument.BindingIndex, EnvLight->GetResourceTable());
		}
			break;
#if (COMPILE_WITH_RHI_METAL)
        case ARGUMENT_EB_VT_INDIRECT:
            RHI->SetShaderTexture(Argument.BindingIndex, FVTSystem::Get()->GetVTIndirectTexture());
            break;
        case ARGUMENT_EB_VT_PHYSIC:
            RHI->SetShaderTexture(Argument.BindingIndex, FVTSystem::Get()->GetVTPhysicTexture());
            break;
#else
		case ARGUMENT_EB_VT_INDIRECT_AND_PHYSIC:
			TI_ASSERT(0);
			//RHI->SetArgumentBuffer(Argument.BindingIndex, FVTSystem::Get()->GetVTResource());
			break;
#endif
		case ARGUMENT_MI_ARGUMENTS:
			RHI->SetArgumentBuffer(Argument.BindingIndex, Primitive->GetSection(SectionIndex).Argument);
			break;
		default:
			TI_ASSERT(0);
			break;
		}
	}

	void FDefaultRenderer::ApplyShaderParameter(FRHI * RHI, FScene * Scene, FPrimitivePtr Primitive, int32 SectionIndex)
	{
		FShaderBindingPtr ShaderBinding = Primitive->GetSection(SectionIndex).Pipeline->GetShader()->GetShaderBinding();

		// bind vertex arguments
		const TVector<FShaderBinding::FShaderArgument>& VSArguments = ShaderBinding->GetVertexComputeShaderArguments();
		for (const auto& Arg : VSArguments)
		{
			BindEngineBuffer(RHI, ESS_VERTEX_SHADER, Arg, Scene, Primitive, SectionIndex);
		}

		// bind pixel arguments
		const TVector<FShaderBinding::FShaderArgument>& PSArguments = ShaderBinding->GetPixelShaderArguments();
		for (const auto& Arg : PSArguments)
		{
			BindEngineBuffer(RHI, ESS_PIXEL_SHADER, Arg, Scene, Primitive, SectionIndex);
		}
	}
}
