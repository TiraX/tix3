/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "NaniteLearningRenderer.h"
#include "NaniteMesh.h"
#include "NaniteView.h"
#include "NaniteDebug.h"

#include "Shaders/Dx12/NaniteEnableMeshShader.h"


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

	FRHI* RHI = FRHI::Get();
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

	CullingDebugInfo = CreateCullingDebugInfoUniform(RHICmdList, FNaniteCullingDebug::MaxDebugInfo);
	TessDebugInfo = CreateTessDebugInfoUniform(RHICmdList, FNaniteTessDebug::MaxDebugInfo);
	TessDebugTable = CreateTessDebugTableUniform(RHICmdList, FNaniteTessDebugTable::MaxDebugInfo);

	// Make it big enough to hold all tessed data, for this case, make it cover max 128 un-tessed tris.
	const uint32 TessedPts = 128 * 1024;
	TessedData =
		FUniformBuffer::CreateBuffer(
			RHICmdList,
			"Nanite.TessedData",
			sizeof(FTessedDataStruct),
			TessedPts,
			(uint32)EGPUResourceFlag::Uav,
			nullptr,
			EGPUResourceState::UnorderedAccess);

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
	ClearVisBufferCS = ti_new FClearVisBufferCS();
	ClearVisBufferCS->Finalize();

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
	EShaderType ShaderType;
#if NANITE_MESH_SHADER
	ShaderType = EShaderType::AmpMesh;
#	if DO_TESS
	ShaderNames.ShaderNames[ESS_AMPLIFICATION_SHADER] = "S_RasterizerTessAS";
	ShaderNames.ShaderNames[ESS_MESH_SHADER] = "S_RasterizerTessMS";
	ShaderNames.ShaderNames[ESS_PIXEL_SHADER] = "S_RasterizerTessPS";
#	else
#		if USE_AS_SHADER
	ShaderNames.ShaderNames[ESS_AMPLIFICATION_SHADER] = "S_RasterizerAS";
#		endif
	ShaderNames.ShaderNames[ESS_MESH_SHADER] = "S_RasterizerMS";
	ShaderNames.ShaderNames[ESS_PIXEL_SHADER] = "S_RasterizerPS";
#	endif
#else
	ShaderType = EShaderType::Standard;
	ShaderNames.ShaderNames[ESS_VERTEX_SHADER] = "S_RasterizerVS";
	ShaderNames.ShaderNames[ESS_PIXEL_SHADER] = "S_RasterizerPS";
#endif

	// Move this TShader load to Game Thread. Or Make a gloal shader system.
	TShaderPtr Shader = ti_new TShader(ShaderNames);
	Shader->LoadShaderCode();
	Shader->ShaderResource = FRHI::Get()->CreateShader(ShaderNames, ShaderType);
	FRHI::Get()->UpdateHardwareResourceShader(Shader->ShaderResource, Shader->GetShaderCodes());
	HWRasterizerMat->SetShader(Shader);
	FShaderPtr RTShader = Shader->ShaderResource;

	HWRasterizerMat->EnableTwoSides(true);
	HWRasterizerMat->SetShaderVsFormat(0);
	HWRasterizerMat->SetRTColor(EPF_RGBA16F, ERTC_COLOR0);
	bool EnableDepth = true;
	HWRasterizerMat->EnableDepthWrite(EnableDepth);
	HWRasterizerMat->EnableDepthTest(EnableDepth);
	if (EnableDepth)
	{
		HWRasterizerMat->SetRTDepth(EPF_DEPTH24_STENCIL8);
	}

	// Pipeline
	PL_HWRasterizer = FRHI::Get()->CreatePipeline(RTShader);
	PL_HWRasterizer->SetResourceName("PL_HWRasterizer");
	FRHI::Get()->UpdateHardwareResourceGraphicsPipeline(PL_HWRasterizer, HWRasterizerMat->GetDesc());
	HWRasterizerMat = nullptr;

	// Visbuffer
	TTextureDesc VisBufferDesc;
	VisBufferDesc.Format = EPF_RG32U;
	VisBufferDesc.Width = RTWidth;
	VisBufferDesc.Height = RTHeight;
	VisBufferDesc.AddressMode = ETC_CLAMP_TO_EDGE;
	VisBuffer = FTexture::CreateTexture(VisBufferDesc, (uint32)EGPUResourceFlag::Uav);
	VisBuffer->SetResourceName("VisBuffer");
	VisBuffer->CreateGPUTexture(RHICmdList, TVector<TImagePtr>(), EGPUResourceState::UnorderedAccess);

	// Load displacement texture
	TFile FileImage;
	FileImage.Open("T_Height.hdr", EFA_READ);
	TImagePtr ImgHeight = TImage::LoadImageHDR(FileImage);
	TImagePtr ImgHR16 = ti_new TImage(EPF_R16F, 1024, 1024);
	TI_ASSERT(ImgHeight != nullptr && ImgHeight->GetFormat() == EPF_RGBA16F);
	for (int32 y = 0; y < 1024; y++)
	{
		for (int32 x = 0; x < 1024; x++)
		{
			SColorf C = ImgHeight->GetPixelFloat(x, y);
			ImgHR16->SetPixel(x, y, C);
		}
	}
	TTextureDesc TexDesc;
	TexDesc.Width = 1024;
	TexDesc.Height = 1024;
	TexDesc.Format = EPF_R16F;
	FTexturePtr TexHeight = FTexture::CreateTexture(TexDesc);
	TVector<TImagePtr> SourceImg;
	SourceImg.push_back(ImgHR16);
	TexHeight->CreateGPUTexture(RHICmdList, SourceImg, EGPUResourceState::NonPixelShaderResource);

	// Resource table
	RT_HWRasterize = RHICmdList->GetHeap(0)->CreateRenderResourceTable(NumHWRasterizeParams);

	RHICmdList->SetGPUBufferState(ClusterPageData->GetGPUBuffer(), EGPUResourceState::NonPixelShaderResource);
	RHI->PutUniformBufferInTable(RT_HWRasterize, ClusterPageData, SRV_ClusterPageData);
	RHI->PutUniformBufferInTable(RT_HWRasterize, View, SRV_Views);
	RHI->PutUniformBufferInTable(RT_HWRasterize, VisibleClustersSWHW, SRV_VisibleClusterSWHW);
	RHI->PutTextureInTable(RT_HWRasterize, TexHeight, SRV_TexHeight);
	RHI->PutRWUniformBufferInTable(RT_HWRasterize, TessedData, UAV_TessedData);
	RHI->PutRWTextureInTable(RT_HWRasterize, VisBuffer, 0, UAV_VisBuffer);
	RHI->PutRWUniformBufferInTable(RT_HWRasterize, TessDebugInfo, UAV_TessDebugInfo);
	RHI->PutRWUniformBufferInTable(RT_HWRasterize, TessDebugTable, UAV_TessDebugTable);

	// Indirect command signature
	TVector<E_GPU_COMMAND_TYPE> CommandStructure;
	CommandStructure.resize(1);
#if NANITE_MESH_SHADER
	CommandStructure[0] = GPU_COMMAND_DISPATCH_MESH;
#else
	CommandStructure[0] = GPU_COMMAND_DRAW;
#endif
	CmdSig_HWRasterize = RHI->CreateGPUCommandSignature(PL_HWRasterizer, CommandStructure);
	CmdSig_HWRasterize->SetResourceName("CmdSig_HWRasterize");
	RHI->UpdateHardwareResourceGPUCommandSig(CmdSig_HWRasterize);

	// Calc look up table data
	//CalcLoopIndexData();
}

static const int32 MaxTessellator = 26;
void FNaniteLearningRenderer::CalcLoopIndexData()
{
	const int32 MaxLoop = (MaxTessellator - 1) / 2;
	TVector<int32> VIndexInLoop0, VIndexInLoop1;
	VIndexInLoop0.reserve(1024);
	VIndexInLoop1.reserve(1024);

	VIndexInLoop0.push_back(0);
	int32 PtInLoop0 = 6;
	int32 PtInLoop1 = 3;
	for (int32 loop = 0; loop < MaxLoop; loop++)
	{
		for (int32 pt = 0; pt < PtInLoop0; pt++)
		{
			VIndexInLoop0.push_back(loop + 1);
		}
		PtInLoop0 += 6;
		for (int32 pt = 0; pt < PtInLoop1; pt++)
		{
			VIndexInLoop1.push_back(loop);
		}
		PtInLoop1 += 6;
	}

	TVector<int32> TIndexInLoop0, TIndexInLoop1;
	TIndexInLoop0.reserve(1024);
	TIndexInLoop1.reserve(1024);

	int32 TriInLoop0 = 6;
	int32 TriInLoop1 = 12;
	TIndexInLoop1.push_back(0);
	for (int32 loop = 0; loop < MaxLoop; loop++)
	{
		for (int32 Tri = 0; Tri < TriInLoop0; Tri++)
		{
			TIndexInLoop0.push_back(loop + 1);
		}
		TriInLoop0 += 12;
		for (int32 Tri = 0; Tri < TriInLoop1; Tri++)
		{
			TIndexInLoop1.push_back(loop + 1);
		}
		TriInLoop1 += 12;
	}

	// Packit
	TVector<uint32> PackedVIndex0, PackedVIndex1, PackedTIndex0, PackedTIndex1;
	PackedVIndex0.resize(TMath::DivideAndRoundUp((uint32)VIndexInLoop0.size(), 8u));
	PackedVIndex1.resize(TMath::DivideAndRoundUp((uint32)VIndexInLoop1.size(), 8u));
	PackedTIndex0.resize(TMath::DivideAndRoundUp((uint32)TIndexInLoop0.size(), 8u));
	PackedTIndex1.resize(TMath::DivideAndRoundUp((uint32)TIndexInLoop1.size(), 8u));

	auto PackData = [](TVector<uint32>& Data, int32 Index, int32 Value)
	{
		TI_ASSERT(Value < (1 << 4));
		int32 Element = Index / 8;
		int32 Bit = (Index % 8) * 4;
		Data[Element] |= Value << Bit;
	};
	auto PrintFormated = [](const TVector<uint32>& Data, const TString& Name)
	{
		TStringStream SS;
		SS << "static const uint " << Name << "[" << Data.size() << "] = {" << endl;
		SS << "\t";
		for (int32 i = 0; i < (int32)Data.size(); i++)
		{
			SS << Data[i] << ",";
			if ((i + 1) % 8 == 0)
				SS << endl << "\t";
		}
		SS << endl << "};" << endl;
		_LOG(ELog::Log, SS.str().c_str());
	};

	for (int32 i = 0; i < (int32)VIndexInLoop0.size(); i++)
	{
		PackData(PackedVIndex0, i, VIndexInLoop0[i]);
	}
	for (int32 i = 0; i < (int32)VIndexInLoop1.size(); i++)
	{
		PackData(PackedVIndex1, i, VIndexInLoop1[i]);
	}
	for (int32 i = 0; i < (int32)TIndexInLoop0.size(); i++)
	{
		PackData(PackedTIndex0, i, TIndexInLoop0[i]);
	}
	for (int32 i = 0; i < (int32)TIndexInLoop1.size(); i++)
	{
		PackData(PackedTIndex1, i, TIndexInLoop1[i]);
	}
	PrintFormated(PackedVIndex0, "PackedVIndex0");
	PrintFormated(PackedVIndex1, "PackedVIndex1");
	PrintFormated(PackedTIndex0, "PackedTIndex0");
	PrintFormated(PackedTIndex1, "PackedTIndex1");
}

static bool bFreezeCulling = false;
void FNaniteLearningRenderer::FreezeCulling()
{
	bFreezeCulling = !bFreezeCulling;
}

void FNaniteLearningRenderer::Render(FRHICmdList * RHICmdList)
{
	//TI_ASSERT(0);	// Stop rendering, or else will crash GPU

	FDecodeInfo DecodeInfo;
	DecodeInfo.StartPageIndex = 0;
	DecodeInfo.PageConstants.Y = GetMaxStreamingPages();
	DecodeInfo.MaxNodes = GetMaxNodes();
	DecodeInfo.MaxVisibleClusters = GetMaxVisibleClusters();
	DecodeInfo.MaxCandidateClusters = GetMaxCandidateClusters();
	DecodeInfo.RenderFlags |= NANITE_RENDER_FLAG_FORCE_HW_RASTER;
#if NANITE_MESH_SHADER
	DecodeInfo.RenderFlags |= NANITE_RENDER_FLAG_MESH_SHADER;
#endif

	RHICmdList->BeginEvent("Nanite.VisBuffer");
	// Init Context, clear vis buffer uav
	if (!bFreezeCulling)
	{
		const int32 RTWidth = TEngine::GetAppInfo().Width;
		const int32 RTHeight = TEngine::GetAppInfo().Height;
		ClearVisBufferCS->ApplyParameters(RHICmdList, FInt2(RTWidth, RTHeight), VisBuffer);
		ClearVisBufferCS->Run(RHICmdList);
	}
	// Init args
	if (!bFreezeCulling)
	{
		InitArgsCS->ApplyParameters(RHICmdList, DecodeInfo, QueueState, VisibleClustersArgsSWHW);
		InitArgsCS->Run(RHICmdList);
		ClearCandidateBufferCS->ApplyParameters(RHICmdList, MainAndPostCandididateClustersBuffer);
		ClearCandidateBufferCS->Run(RHICmdList);
	}

	// Fake instance cull
	if (!bFreezeCulling)
	{
		FakeInstanceCullCS->ApplyParameters(RHICmdList, DecodeInfo, QueueState, MainAndPostNodesAndClusterBatchesBuffer);
		FakeInstanceCullCS->Run(RHICmdList);
	}

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

	// node and cluster cull
	if (!bFreezeCulling)
	{
		RHICmdList->SetGPUBufferState(VisibleClustersSWHW->GetGPUBuffer(), EGPUResourceState::UnorderedAccess);
		RHICmdList->SetGPUBufferState(VisibleClustersArgsSWHW->GetGPUBuffer(), EGPUResourceState::UnorderedAccess);

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
			CullingDebugInfo
		);
		PersistentCullCS->Run(RHICmdList);
	}
	RHICmdList->EndEvent();

	RHICmdList->BeginRenderToRenderTarget(RT_BasePass, "BasePass", 0);
	//DrawPrimitives(RHICmdList);

	// Do Hardware rasterize
	RHICmdList->SetGPUBufferState(VisibleClustersSWHW->GetGPUBuffer(), EGPUResourceState::NonPixelShaderResource);
	RHICmdList->SetGPUBufferState(VisibleClustersArgsSWHW->GetGPUBuffer(), EGPUResourceState::IndirectArgument);

	RHICmdList->SetGraphicsPipeline(PL_HWRasterizer);
	RHICmdList->SetPrimitiveTopology(EPrimitiveType::TriangleList);
	RHICmdList->SetGraphicsConstant(RC_DecodeInfo, &DecodeInfo, sizeof(FDecodeInfo) / sizeof(uint32));
	RHICmdList->SetGraphicsResourceTable(RT_Table, RT_HWRasterize);
	RHICmdList->ExecuteIndirect(CmdSig_HWRasterize, VisibleClustersArgsSWHW, 1, 16);

	FRHI::Get()->BeginRenderToFrameBuffer();
	FSRender.DrawFullScreenTexture(RHICmdList, AB_Result);
}
