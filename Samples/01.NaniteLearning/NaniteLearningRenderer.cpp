/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "NaniteLearningRenderer.h"
#include "NaniteMesh.h"
#include "NaniteView.h"
#include "NaniteDebug.h"


FNaniteLearningRenderer::FNaniteLearningRenderer(FSceneInterface* Scene)
	: FDefaultRenderer(Scene)
	, NaniteMesh(nullptr)
{
}

FNaniteLearningRenderer::~FNaniteLearningRenderer()
{
	ti_delete NaniteMesh;
}

void FNaniteLearningRenderer::InitInRenderThread()
{
	FDefaultRenderer::InitInRenderThread();

	FRHI * RHI = FRHI::Get();
	FRHICmdList* RHICmdList = RHI->GetDefaultCmdList();
	FSRender.InitCommonResources(RHICmdList);

	const int32 RTWidth = TEngine::GetAppInfo().Width;
	const int32 RTHeight = TEngine::GetAppInfo().Height;

	TStreamPtr ArgumentValues = ti_new TStream;
	TVector<FTexturePtr> ArgumentTextures;

	// Setup base pass render target
	{
		RT_BasePass = FRenderTarget::Create(RTWidth, RTHeight);
		RT_BasePass->SetResourceName("RT_BasePass");
		TTextureDesc BasePassDesc;
		BasePassDesc.Format = EPF_RGBA16F;
		BasePassDesc.Width = RTWidth;
		BasePassDesc.Height = RTHeight;
		BasePassDesc.AddressMode = ETC_CLAMP_TO_EDGE;
		BasePassDesc.Mips = 1;
		BasePassDesc.ClearColor = SColorf(0, 0, 1, 0);
		FTexturePtr SceneColor = FTexture::CreateTexture(BasePassDesc);
		RT_BasePass->AddColorBuffer(SceneColor, ERTC_COLOR0, ERenderTargetLoadAction::Clear, ERenderTargetStoreAction::Store);

		RT_BasePass->AddDepthStencilBuffer(EPF_DEPTH24_STENCIL8, 1, ERenderTargetLoadAction::Clear, ERenderTargetStoreAction::DontCare);
		RT_BasePass->Compile();
	}

	// Setup EnvLight
	const TString IBLAsset = "EnvIBL.tasset";
	TResourcePtr IBLRes = TAssetLibrary::Get()->LoadAsset(IBLAsset)->GetResourcePtr();
	TTexturePtr EnvTexture = static_cast<TTexture*>(IBLRes.get());
	FTexturePtr IBL = EnvTexture->TextureResource;
	FEnvLightPtr EnvLight = ti_new FEnvLight(IBL, FFloat3());
	Scene->SetEnvLight(EnvLight);

	// Load default pipeline
	//const TString DefaultMaterial = "M_Debug.tasset";
	//TAssetPtr DebugMaterialAsset = TAssetLibrary::Get()->LoadAsset(DefaultMaterial);
	//TResourcePtr DebugMaterialResource = DebugMaterialAsset->GetResourcePtr();
	//TMaterialPtr DebugMaterial = static_cast<TMaterial*>(DebugMaterialResource.get());
	//FPipelinePtr DebugPipeline = DebugMaterial->PipelineResource;


	AB_Result = RHI->CreateArgumentBuffer(1);
	{
		AB_Result->SetTexture(0, RT_BasePass->GetColorBuffer(0).Texture);
		RHI->UpdateHardwareResourceAB(AB_Result, FSRender.GetFullScreenShader(), 0);
	}

	// For fast debug, load nanite mesh in render thread here.
	NaniteMesh = TNaniteMesh::LoadMesh();

	// Setup nanite related
	TI_ASSERT(ClusterPageData == nullptr);
	ClusterPageData = FStreamingPageUploader::AllocateClusterPageBuffer(RHICmdList);

	View = FUniformBuffer::CreateBuffer(
		RHICmdList,
		"Nanite.Views",
		sizeof(FPackedView),
		1,
		(uint32)EGPUResourceFlag::Intermediate
	);
	const uint32 QueueStateSize = (6 * 2 + 1) * sizeof(uint32);
	TStreamPtr ZeroData = ti_new TStream(1024);
	ZeroData->FillWithZero(1024);
	QueueState = FUniformBuffer::CreateUavBuffer(RHICmdList, "Nanite.QueueState", QueueStateSize, 1, ZeroData, EGPUResourceState::UnorderedAccess);
	const uint32 MaxNodes = GetMaxNodes();
 	const uint32 MaxCullingBatches = GetMaxClusterBatches();
	MainAndPostNodesAndClusterBatchesBuffer = 
		FUniformBuffer::CreateBuffer(
			RHICmdList, 
			"Nanite.MainAndPostNodesAndClusterBatchesBuffer", 
			4, 
			MaxCullingBatches * 2 + MaxNodes * (2 + 3), 
			(uint32)EGPUResourceFlag::Uav | (uint32)EGPUResourceFlag::ByteAddressBuffer,
			nullptr,
			EGPUResourceState::UnorderedAccess);

	MainAndPostCandididateClustersBuffer = 
		FUniformBuffer::CreateBuffer(
			RHICmdList,
			"Nanite.MainAndPostCandididateClustersBuffer",
			4,
			GetMaxCandidateClusters() * 2,
			(uint32)EGPUResourceFlag::Uav | (uint32)EGPUResourceFlag::ByteAddressBuffer,
			nullptr,
			EGPUResourceState::UnorderedAccess);
	VisibleClustersSWHW =
		FUniformBuffer::CreateBuffer(
			RHICmdList,
			"Nanite.VisibleClustersSWHW",
			4,
			3 * GetMaxVisibleClusters(),
			(uint32)EGPUResourceFlag::Uav | (uint32)EGPUResourceFlag::ByteAddressBuffer,
			nullptr,
			EGPUResourceState::UnorderedAccess);
	// TODO: VisibleClustersArgsSWHW also will be indirect command buffer
	VisibleClustersArgsSWHW =
		FUniformBuffer::CreateBuffer(
			RHICmdList,
			"Nanite.MainRasterizeArgsSWHW",
			4,
			NANITE_RASTERIZER_ARG_COUNT,
			(uint32)EGPUResourceFlag::Uav | (uint32)EGPUResourceFlag::ByteAddressBuffer,
			nullptr,
			EGPUResourceState::UnorderedAccess);

	DebugInfo = CreateDebugInfoUniform(RHICmdList, FNaniteDebug::MaxDebugInfo);

	StreamingManager.ProcessNewResources(RHICmdList, NaniteMesh, ClusterPageData);

	Hierarchy = FStreamingPageUploader::AllocateHierarchyBuffer(RHICmdList, NaniteMesh->HierarchyNodes);

	// Create shaders
	InitCandidateNodesCS = ti_new FInitCandidateNodesCS;
	InitCandidateNodesCS->Finalize();
	InitClusterBatchesCS = ti_new FInitClusterBatchesCS;
	InitClusterBatchesCS->Finalize();
	InitArgsCS = ti_new FInitArgsCS;
	InitArgsCS->Finalize();
	ClearCandidateBufferCS = ti_new FClearCandidateBufferCS;
	ClearCandidateBufferCS->Finalize();
	FakeInstanceCullCS = ti_new FFakeInstanceCullCS;
	FakeInstanceCullCS->Finalize();
	PersistentCullCS = ti_new FPersistentCullCS();
	PersistentCullCS->Finalize();

	FDecodeInfo DecodeInfo;
	DecodeInfo.StartPageIndex = 0;
	DecodeInfo.PageConstants.Y = GetMaxStreamingPages();
	DecodeInfo.MaxNodes = GetMaxNodes();
	DecodeInfo.MaxVisibleClusters = GetMaxVisibleClusters();
	DecodeInfo.MaxCandidateClusters = GetMaxCandidateClusters();

	InitCandidateNodesCS->ApplyParameters(RHICmdList, DecodeInfo, MainAndPostNodesAndClusterBatchesBuffer);
	InitCandidateNodesCS->Run(RHICmdList);
	InitClusterBatchesCS->ApplyParameters(RHICmdList, DecodeInfo, MainAndPostNodesAndClusterBatchesBuffer);
	InitClusterBatchesCS->Run(RHICmdList);



	// Create hardware rasterizer pipeline
	TMaterialPtr HWRasterizerMat = ti_new TMaterial();
	HWRasterizerMat->SetResourceName("HWRasterizer");

	TShaderNames ShaderNames;
	ShaderNames.ShaderNames[ESS_VERTEX_SHADER] = "S_RasterizerVS";
	ShaderNames.ShaderNames[ESS_PIXEL_SHADER] = "S_RasterizerPS";

	// Move this TShader load to Game Thread. Or Make a gloal shader system.
	TShaderPtr Shader = ti_new TShader(ShaderNames);
	Shader->LoadShaderCode();
	Shader->ShaderResource = FRHI::Get()->CreateShader(ShaderNames, EST_RENDER);
	FRHI::Get()->UpdateHardwareResourceShader(Shader->ShaderResource, Shader->GetShaderCodes());
	HWRasterizerMat->SetShader(Shader);
	FShaderPtr RTShader = Shader->ShaderResource;

	HWRasterizerMat->EnableTwoSides(true);
	HWRasterizerMat->EnableDepthWrite(false);
	HWRasterizerMat->EnableDepthTest(false);
	HWRasterizerMat->SetShaderVsFormat(0);
	HWRasterizerMat->SetRTColor(EPF_RGBA16F, ERTC_COLOR0);

	// Pipeline
	HWRasterizerPL = FRHI::Get()->CreatePipeline(RTShader);
	HWRasterizerPL->SetResourceName("HWRasterizerPL");
	FRHI::Get()->UpdateHardwareResourceGraphicsPipeline(HWRasterizerPL, HWRasterizerMat->GetDesc());
	HWRasterizerMat = nullptr;

	// Visbuffer
	TTextureDesc VisBufferDesc;
	VisBufferDesc.Format = EPF_RG32U;
	VisBufferDesc.Width = RTWidth;
	VisBufferDesc.Height = RTHeight;
	VisBufferDesc.AddressMode = ETC_CLAMP_TO_EDGE;
	VisBuffer = FTexture::CreateTexture(VisBufferDesc, (uint32)EGPUResourceFlag::Uav);
	VisBuffer->CreateGPUTexture(RHICmdList, TVector<TImagePtr>(), EGPUResourceState::UnorderedAccess);

	// Resource table
	RT_HWRasterize = RHICmdList->GetHeap(0)->CreateRenderResourceTable(NumHWRasterizeParams);

	RHICmdList->SetGPUBufferState(ClusterPageData->GetGPUBuffer(), EGPUResourceState::NonPixelShaderResource);
	RHI->PutUniformBufferInTable(RT_HWRasterize, ClusterPageData, SRV_ClusterPageData);
	RHI->PutUniformBufferInTable(RT_HWRasterize, View, SRV_Views);
	RHI->PutUniformBufferInTable(RT_HWRasterize, VisibleClustersSWHW, SRV_VisibleClusterSWHW);
	RHI->PutRWTextureInTable(RT_HWRasterize, VisBuffer, 0, UAV_VisBuffer);
}

void FNaniteLearningRenderer::Render(FRHICmdList* RHICmdList)
{
	FDecodeInfo DecodeInfo;
	DecodeInfo.StartPageIndex = 0;
	DecodeInfo.PageConstants.Y = GetMaxStreamingPages();
	DecodeInfo.MaxNodes = GetMaxNodes();
	DecodeInfo.MaxVisibleClusters = GetMaxVisibleClusters();
	DecodeInfo.MaxCandidateClusters = GetMaxCandidateClusters();

	RHICmdList->BeginEvent("Nanite.VisBuffer");
	// Init args
	{
		InitArgsCS->ApplyParameters(RHICmdList, DecodeInfo, QueueState, VisibleClustersArgsSWHW);
		InitArgsCS->Run(RHICmdList);
		ClearCandidateBufferCS->ApplyParameters(RHICmdList, MainAndPostCandididateClustersBuffer);
		ClearCandidateBufferCS->Run(RHICmdList);
	}

	// Fake instance cull
	{
		FakeInstanceCullCS->ApplyParameters(RHICmdList, DecodeInfo, QueueState, MainAndPostNodesAndClusterBatchesBuffer);
		FakeInstanceCullCS->Run(RHICmdList);
	}

	// node and cluster cull
	{
		RHICmdList->SetGPUBufferState(VisibleClustersSWHW->GetGPUBuffer(), EGPUResourceState::UnorderedAccess);
		FPackedView PackedView = CreatePackedViewFromViewInfo(
			Scene->GetViewProjection(),
			FInt2(TEngine::GetAppInfo().Width, TEngine::GetAppInfo().Height),
			NANITE_VIEW_FLAG_HZBTEST | NANITE_VIEW_FLAG_NEAR_CLIP,
			/* StreamingPriorityCategory = */ 3,
			/* MinBoundsRadius = */ 0.0f,
			1.0
		);
		// Update View uniform buffer
		uint8* ViewDataPtr = View->GetGPUBuffer()->Lock();
		memcpy(ViewDataPtr, &PackedView, sizeof(FPackedView));
		View->GetGPUBuffer()->Unlock();

		PersistentCullCS->ApplyParameters(
			RHICmdList,
			DecodeInfo,
			ClusterPageData,
			Hierarchy,
			View,
			QueueState,
			MainAndPostNodesAndClusterBatchesBuffer,
			MainAndPostCandididateClustersBuffer,
			nullptr,
			VisibleClustersSWHW,
			VisibleClustersArgsSWHW,
			DebugInfo
			);
		PersistentCullCS->Run(RHICmdList);
	}
	RHICmdList->EndEvent();

	RHICmdList->BeginRenderToRenderTarget(RT_BasePass, "BasePass", 0);
	//DrawPrimitives(RHICmdList);

	// Do Hardware rasterize
	RHICmdList->SetGPUBufferState(VisibleClustersSWHW->GetGPUBuffer(), EGPUResourceState::NonPixelShaderResource);
	RHICmdList->SetGraphicsPipeline(HWRasterizerPL);
	RHICmdList->SetPrimitiveTopology(EPrimitiveType::TriangleList);
	RHICmdList->SetGraphicsConstant(RC_DecodeInfo, &DecodeInfo, sizeof(FDecodeInfo) / sizeof(uint32));
	RHICmdList->SetGraphicsResourceTable(RT_Table, RT_HWRasterize);
	RHICmdList->DrawPrimitiveInstanced(
		384,
		4000,
		0);

	FRHI::Get()->BeginRenderToFrameBuffer();
	FSRender.DrawFullScreenTexture(RHICmdList, AB_Result);
}
