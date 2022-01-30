/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "FDefaultRenderer.h"
#include "FUniformBufferView.h"

namespace tix
{
	FDefaultRenderer::FDefaultRenderer(FSceneInterface* InScene)
		: FRendererInterface(InScene)
		, Scene(nullptr)
	{
		Scene = dynamic_cast<FDefaultScene*>(InScene);
		TI_ASSERT(Scene != nullptr);
	}

	FDefaultRenderer::~FDefaultRenderer()
	{
		TI_ASSERT(IsRenderThread());
	}

	// From UE4
	void SetupSkyIrradianceEnvironmentMapConstantsFromSkyIrradiance(FFloat4* OutSkyIrradianceEnvironmentMap, const FSHVectorRGB3& SkyIrradiance)
	{
		const float SqrtPI = TMath::Sqrt(PI);
		const float Coefficient0 = 1.0f / (2 * SqrtPI);
		const float Coefficient1 = TMath::Sqrt(3) / (3 * SqrtPI);
		const float Coefficient2 = TMath::Sqrt(15) / (8 * SqrtPI);
		const float Coefficient3 = TMath::Sqrt(5) / (16 * SqrtPI);
		const float Coefficient4 = .5f * Coefficient2;

		// Pack the SH coefficients in a way that makes applying the lighting use the least shader instructions
		// This has the diffuse convolution coefficients baked in
		// See "Stupid Spherical Harmonics (SH) Tricks"
		OutSkyIrradianceEnvironmentMap[0].X = -Coefficient1 * SkyIrradiance.R.V[3];
		OutSkyIrradianceEnvironmentMap[0].Y = -Coefficient1 * SkyIrradiance.R.V[1];
		OutSkyIrradianceEnvironmentMap[0].Z = Coefficient1 * SkyIrradiance.R.V[2];
		OutSkyIrradianceEnvironmentMap[0].W = Coefficient0 * SkyIrradiance.R.V[0] - Coefficient3 * SkyIrradiance.R.V[6];

		OutSkyIrradianceEnvironmentMap[1].X = -Coefficient1 * SkyIrradiance.G.V[3];
		OutSkyIrradianceEnvironmentMap[1].Y = -Coefficient1 * SkyIrradiance.G.V[1];
		OutSkyIrradianceEnvironmentMap[1].Z = Coefficient1 * SkyIrradiance.G.V[2];
		OutSkyIrradianceEnvironmentMap[1].W = Coefficient0 * SkyIrradiance.G.V[0] - Coefficient3 * SkyIrradiance.G.V[6];

		OutSkyIrradianceEnvironmentMap[2].X = -Coefficient1 * SkyIrradiance.B.V[3];
		OutSkyIrradianceEnvironmentMap[2].Y = -Coefficient1 * SkyIrradiance.B.V[1];
		OutSkyIrradianceEnvironmentMap[2].Z = Coefficient1 * SkyIrradiance.B.V[2];
		OutSkyIrradianceEnvironmentMap[2].W = Coefficient0 * SkyIrradiance.B.V[0] - Coefficient3 * SkyIrradiance.B.V[6];

		OutSkyIrradianceEnvironmentMap[3].X = Coefficient2 * SkyIrradiance.R.V[4];
		OutSkyIrradianceEnvironmentMap[3].Y = -Coefficient2 * SkyIrradiance.R.V[5];
		OutSkyIrradianceEnvironmentMap[3].Z = 3 * Coefficient3 * SkyIrradiance.R.V[6];
		OutSkyIrradianceEnvironmentMap[3].W = -Coefficient2 * SkyIrradiance.R.V[7];

		OutSkyIrradianceEnvironmentMap[4].X = Coefficient2 * SkyIrradiance.G.V[4];
		OutSkyIrradianceEnvironmentMap[4].Y = -Coefficient2 * SkyIrradiance.G.V[5];
		OutSkyIrradianceEnvironmentMap[4].Z = 3 * Coefficient3 * SkyIrradiance.G.V[6];
		OutSkyIrradianceEnvironmentMap[4].W = -Coefficient2 * SkyIrradiance.G.V[7];

		OutSkyIrradianceEnvironmentMap[5].X = Coefficient2 * SkyIrradiance.B.V[4];
		OutSkyIrradianceEnvironmentMap[5].Y = -Coefficient2 * SkyIrradiance.B.V[5];
		OutSkyIrradianceEnvironmentMap[5].Z = 3 * Coefficient3 * SkyIrradiance.B.V[6];
		OutSkyIrradianceEnvironmentMap[5].W = -Coefficient2 * SkyIrradiance.B.V[7];

		OutSkyIrradianceEnvironmentMap[6].X = Coefficient4 * SkyIrradiance.R.V[8];
		OutSkyIrradianceEnvironmentMap[6].Y = Coefficient4 * SkyIrradiance.G.V[8];
		OutSkyIrradianceEnvironmentMap[6].Z = Coefficient4 * SkyIrradiance.B.V[8];
		OutSkyIrradianceEnvironmentMap[6].W = 1;
	}

	void FDefaultRenderer::PrepareViewUniforms()
	{
		if (Scene->HasSceneFlag(FDefaultScene::ViewUniformDirty))
		{
			// Always make a new View uniform buffer for on-the-fly rendering
			ViewUniformBuffer = ti_new FViewUniformBuffer();

			const FViewProjectionInfo& VPInfo = Scene->GetViewProjection();
			const FEnvironmentInfo& EnvInfo = Scene->GetEnvironmentInfo();
			ViewUniformBuffer->UniformBufferData[0].VP = (VPInfo.MatProj * VPInfo.MatView).GetTransposed();
			ViewUniformBuffer->UniformBufferData[0].ViewDir = VPInfo.CamDir;
			ViewUniformBuffer->UniformBufferData[0].ViewPos = VPInfo.CamPos;

			ViewUniformBuffer->UniformBufferData[0].MainLightDirection = -EnvInfo.MainLightDirection;
			ViewUniformBuffer->UniformBufferData[0].MainLightColor = EnvInfo.MainLightColor * EnvInfo.MainLightIntensity;

			SetupSkyIrradianceEnvironmentMapConstantsFromSkyIrradiance(ViewUniformBuffer->UniformBufferData[0].SkyIrradiance, EnvInfo.SkyIrradiance);

			ViewUniformBuffer->InitUniformBuffer((uint32)EGPUResourceFlag::Intermediate);
		}
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

	void FDefaultRenderer::InitRenderFrame()
	{
		// Prepare frame view uniform buffer
		PrepareViewUniforms();

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

	void FDefaultRenderer::EndRenderFrame()
	{
		// Clear the flags in this frame.
		Scene->ClearSceneFlags();
	}

	void FDefaultRenderer::Render(FRHI* RHI)
	{
		RHI->BeginRenderToFrameBuffer();
		DrawPrimitives(RHI);
	}

	void FDefaultRenderer::DrawPrimitives(FRHI* RHI)
	{
		const TVector<FPrimitivePtr>& Prims = Scene->Primitives;
		for (uint32 PIndex = 0; PIndex < (uint32)Prims.size(); ++PIndex)
		{
			FPrimitivePtr Primitive = Prims[PIndex];

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
					ApplyShaderParameter(RHI, Primitive, S);
					RHI->DrawPrimitiveIndexedInstanced(
						Section.Triangles * 3,
						InstanceBuffer == nullptr ? 1 : Primitive->GetInstanceCount(),
						Section.IndexStart,
						Primitive->GetInstanceOffset());
				}
			}
		}
	}

	void FDefaultRenderer::BindEngineBuffer(FRHI * RHI, E_SHADER_STAGE ShaderStage, const FShaderBinding::FShaderArgument& Argument, FPrimitivePtr Primitive, int32 SectionIndex)
	{
		switch (Argument.ArgumentType)
		{
		case ARGUMENT_UNKNOWN:
			// A custom argument, do nothing here.
			break;
		case ARGUMENT_EB_VIEW:
			RHI->SetUniformBuffer(ShaderStage, Argument.BindingIndex, ViewUniformBuffer->UniformBuffer);
			break;
		case ARGUMENT_EB_PRIMITIVE:
			TI_ASSERT(Primitive != nullptr);
			RHI->SetUniformBuffer(ShaderStage, Argument.BindingIndex, Primitive->GetPrimitiveUniform()->UniformBuffer);
			break;
		case ARGUMENT_EB_BONES:
			TI_ASSERT(Primitive->GetSection(SectionIndex).SkeletonResourceRef != nullptr);
			RHI->SetUniformBuffer(ShaderStage, Argument.BindingIndex, Primitive->GetSection(SectionIndex).SkeletonResourceRef);
			break;
		case ARGUMENT_EB_ENV_CUBE:
		{
			FEnvLightPtr EnvLight = Scene->GetEnvLight();
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
			RuntimeFail();
			break;
		}
	}

	void FDefaultRenderer::ApplyShaderParameter(FRHI * RHI, FPrimitivePtr Primitive, int32 SectionIndex)
	{
		FShaderBindingPtr ShaderBinding = Primitive->GetSection(SectionIndex).Pipeline->GetShader()->GetShaderBinding();

		// bind vertex arguments
		const TVector<FShaderBinding::FShaderArgument>& VSArguments = ShaderBinding->GetVertexComputeShaderArguments();
		for (const auto& Arg : VSArguments)
		{
			BindEngineBuffer(RHI, ESS_VERTEX_SHADER, Arg, Primitive, SectionIndex);
		}

		// bind pixel arguments
		const TVector<FShaderBinding::FShaderArgument>& PSArguments = ShaderBinding->GetPixelShaderArguments();
		for (const auto& Arg : PSArguments)
		{
			BindEngineBuffer(RHI, ESS_PIXEL_SHADER, Arg, Primitive, SectionIndex);
		}
	}

}
