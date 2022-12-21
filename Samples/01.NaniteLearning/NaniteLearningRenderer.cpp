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

	CreateTessellationTemplates(RHICmdList);

	// Resource table
	RT_HWRasterize = RHICmdList->GetHeap(0)->CreateRenderResourceTable(NumHWRasterizeParams);

	RHICmdList->SetGPUBufferState(ClusterPageData->GetGPUBuffer(), EGPUResourceState::NonPixelShaderResource);
	RHI->PutUniformBufferInTable(RT_HWRasterize, ClusterPageData, SRV_ClusterPageData);
	RHI->PutUniformBufferInTable(RT_HWRasterize, View, SRV_Views);
	RHI->PutUniformBufferInTable(RT_HWRasterize, VisibleClustersSWHW, SRV_VisibleClusterSWHW);
	RHI->PutUniformBufferInTable(RT_HWRasterize, TessTemplateData, SRV_TessTemplates);
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

}

inline uint32 CalcTessInsideTriCount(uint32 n)
{
	if ((n & 1) == 0)
	{
		// even
		return n * n * 3 / 2;
	}
	else
	{
		// odd
		return (n / 2) * 3 * (n + 1) + 1;
	}
}
inline uint32 CalcTessInsidePointCount(uint32 n)
{
	if ((n & 1) == 0)
	{
		// even
		uint32 nn = n / 2;
		return (nn * nn + nn) * 3 + 1;
	}
	else
	{
		// odd
		uint32 nn = n / 2 + 1;
		return (nn * nn) * 3;
	}
}

static const uint32 MaxTessFactor = 26;
void AddPoints(TVector<FFloat3>& OutBaryCoords)
{
	const FFloat3 B0(0, 1, 0);
	const FFloat3 B1(1, 0, 0);
	const FFloat3 B2(0, 0, 1);
	const FFloat3 BC = (B0 + B1 + B2) / 3.0f;
	const FFloat3 D0 = (B0 - BC).Normalize();
	const FFloat3 D1 = (B1 - BC).Normalize();
	const FFloat3 D2 = (B2 - BC).Normalize();
	const float EdgeLen = (B0 - B1).GetLength();
	const float Cos30Inv = 1.f / cos(TMath::DegToRad(30.f));

	auto AddLoop = [&OutBaryCoords](const FFloat3& p0, const FFloat3& p1, const FFloat3& p2, int32 segs)
	{
		const float SegsInv = 1.f / float(segs);
		// Points on p0 - p1
		FFloat3 D = (p1 - p0) * SegsInv;
		FFloat3 Start = p0;
		for (int s = 0; s < segs; s++)
		{
			FFloat3 P = Start + D * float(s);
			OutBaryCoords.push_back(P);
		}
		// Points on B1-B2
		D = (p2 - p1) * SegsInv;
		Start = p1;
		for (int s = 0; s < segs; s++)
		{
			FFloat3 P = Start + D * float(s);
			OutBaryCoords.push_back(P);
		}
		// Points on B2-B0
		D = (p0 - p2) * SegsInv;
		Start = p2;
		for (int s = 0; s < segs; s++)
		{
			FFloat3 P = Start + D * float(s);
			OutBaryCoords.push_back(P);
		}
	};

	for (uint32 TessFactor = 2; TessFactor <= MaxTessFactor; TessFactor++)
	{
		float SegLen = EdgeLen / TessFactor;
		int InsideSegs = TessFactor - 2;
		if ((InsideSegs & 1) == 0)
		{
			int Loops = InsideSegs / 2;
			for (int l = Loops - 1; l >= 0; l--)
			{
				int SegInLoop = l * 2 + 2;
				float Radius = SegLen * (SegInLoop / 2) * Cos30Inv;
				FFloat3 C0 = BC + D0 * Radius;    // Corner0
				FFloat3 C1 = BC + D1 * Radius;    // Corner1
				FFloat3 C2 = BC + D2 * Radius;    // Corner2
				AddLoop(C0, C1, C2, SegInLoop);
			}
			OutBaryCoords.push_back(BC);	// add center point for even tess
		}
		else
		{
			int Loops = InsideSegs / 2 + 1;
			for (int l = Loops - 1; l >= 0; l--)
			{
				int SegInLoop = l * 2 + 1;
				float Radius = SegLen * ((SegInLoop / 2) + 0.5f) * Cos30Inv;
				FFloat3 C0 = BC + D0 * Radius;    // Corner0
				FFloat3 C1 = BC + D1 * Radius;    // Corner1
				FFloat3 C2 = BC + D2 * Radius;    // Corner2
				AddLoop(C0, C1, C2, SegInLoop);
			}
		}
	}
}

void AddTriangles(TVector<FUInt3>& OutTriangles)
{
	for (uint32 TessFactor = 2; TessFactor <= MaxTessFactor; TessFactor++)
	{
		uint32 InsideSegs = TessFactor - 2;

		if ((InsideSegs & 1) == 0)
		{
			int Loops = InsideSegs / 2;
			for (int side = 0; side < 3; side++)
			{
				int line_offset = 0;
				for (int l = Loops - 1; l >= 0; l--)
				{
					int SegInCurrLoop = l * 2 + 2;
					int SegInNextLoop = (l - 1) * 2 + 2;
					int curr_loop_ptnum = SegInCurrLoop * 3;
					int next_loop_ptnum = SegInNextLoop * 3;
					int curr_loop_start = line_offset + SegInCurrLoop * side;
					int next_loop_start = line_offset + curr_loop_ptnum + SegInNextLoop * side;

					for (int seg = 0; seg < SegInCurrLoop; seg++)
					{
						int curr0 = curr_loop_start + seg;
						int curr1 = curr_loop_start + seg + 1;
						if (curr1 >= line_offset + curr_loop_ptnum) curr1 -= curr_loop_ptnum;
						int next0 = next_loop_start + seg;
						int next_1 = next_loop_start + seg - 1;
						if (next0 >= line_offset + curr_loop_ptnum + next_loop_ptnum) next0 -= next_loop_ptnum;
						if (next_1 >= line_offset + curr_loop_ptnum + next_loop_ptnum) next_1 -= next_loop_ptnum;

						if ((seg & 1) == 0)
						{
							OutTriangles.push_back(FUInt3(curr0, curr1, next0));
						}
						else
						{
							OutTriangles.push_back(FUInt3(curr0, curr1, next_1));
						}
						if (seg > 0 && seg < SegInCurrLoop - 1)
						{
							if ((seg & 1) == 0)
							{
								OutTriangles.push_back(FUInt3(curr0, next0, next_1));
							}
							else
							{
								OutTriangles.push_back(FUInt3(next_1, curr1, next0));
							}
						}
					}
					line_offset += curr_loop_ptnum;
				}
			}
		}
		else
		{
			int Loops = InsideSegs / 2;
			for (int side = 0; side < 3; side++)
			{
				int line_offset = 0;
				for (int l = Loops - 1; l >= 0; l--)
				{
					int SegInCurrLoop = l * 2 + 3;
					int SegInNextLoop = (l - 1) * 2 + 3;
					int curr_loop_ptnum = SegInCurrLoop * 3;
					int next_loop_ptnum = SegInNextLoop * 3;
					int curr_loop_start = line_offset + SegInCurrLoop * side;
					int next_loop_start = line_offset + curr_loop_ptnum + SegInNextLoop * side;

					for (int seg = 0; seg < SegInCurrLoop; seg++)
					{
						int curr0 = curr_loop_start + seg;
						int curr1 = curr_loop_start + seg + 1;
						if (curr1 >= line_offset + curr_loop_ptnum) curr1 -= curr_loop_ptnum;
						int next0 = next_loop_start + seg;
						int next_1 = next_loop_start + seg - 1;
						if (next0 >= line_offset + curr_loop_ptnum + next_loop_ptnum) next0 -= next_loop_ptnum;
						if (next_1 >= line_offset + curr_loop_ptnum + next_loop_ptnum) next_1 -= next_loop_ptnum;

						if ((seg & 1) == 0)
						{
							if (seg != SegInCurrLoop - 1)
								OutTriangles.push_back(FUInt3(curr0, curr1, next0));
							else
								OutTriangles.push_back(FUInt3(curr0, curr1, next_1));
						}
						else
						{
							OutTriangles.push_back(FUInt3(curr0, curr1, next_1));
						}
						if (seg > 0 && seg < SegInCurrLoop - 1)
						{
							if ((seg & 1) == 0)
							{
								OutTriangles.push_back(FUInt3(curr0, next0, next_1));
							}
							else
							{
								OutTriangles.push_back(FUInt3(next_1, curr1, next0));
							}
						}
					}
					line_offset += curr_loop_ptnum;
				}
			}
			int total_pts = CalcTessInsidePointCount(InsideSegs);
			OutTriangles.push_back(FUInt3(total_pts - 3, total_pts - 2, total_pts - 1));
		}
	}
}

TStreamPtr GroupTrianglesAndVerts(
	const TVector<FFloat3>& BaryCoords,
	const TVector<FUInt3>& Triangles
)
{
	uint32 TotalTrisOffset = 0;
	uint32 TotalPointsOffset = 0;

	THMap<uint32, uint32> VertMap;
	VertMap.reserve(256);
	TVector<FFloat3> NewBaryCoords;
	TVector<uint8> NewTriangles;
	NewBaryCoords.reserve(96);
	NewTriangles.reserve(96);

	struct FTemplateDesc
	{
		uint32 Offset;
		uint16 Verts;
		uint16 Tris;
	};
	struct FGroupOffAndCount
	{
		int32 Offset;
		int32 Count;
		int32 TrisAfterGroup;
		int32 TrisBeforeGroup;
	};

	auto NewVertsAdded = [&VertMap](uint32 IX, uint32 IY, uint32 IZ)
	{
		uint32 Added = 0;
		if (VertMap.find(IX) == VertMap.end())
			Added++;
		if (VertMap.find(IY) == VertMap.end())
			Added++;
		if (VertMap.find(IZ) == VertMap.end())
			Added++;
		return Added;
	};

	auto InsertVert = [&VertMap, &NewBaryCoords, &NewTriangles](uint32 VertIndex, const FFloat3& BCoord)
	{
		uint32 NewIndex = uint32(-1);
		THMap<uint32, uint32>::const_iterator It = VertMap.find(VertIndex);
		if (It == VertMap.end())
		{
			NewIndex = (uint32)VertMap.size();
			VertMap[VertIndex] = NewIndex;
			NewBaryCoords.push_back(BCoord);
			TI_ASSERT(VertMap.size() == NewBaryCoords.size());
		}
		else
		{
			NewIndex = It->second;
		}
		NewTriangles.push_back(NewIndex);
	};

	uint32 MaxDataSize = TMath::Align4((uint32)Triangles.size() * 3) + (uint32)BaryCoords.size() * sizeof(FFloat3) * 2;
	TStreamPtr Data = ti_new TStream(MaxDataSize);

	TVector<FTemplateDesc> Descs;
	Descs.reserve(1024);

	TVector<FGroupOffAndCount> GroupOffAndCounts;
	GroupOffAndCounts.reserve(18);
	GroupOffAndCounts.push_back({ 0, 0 });	// tess_factor 1 = 0

	for (uint32 i = 2; i <= MaxTessFactor; i++)
	{
		uint32 TessGroupOffset = (uint32)Descs.size();

		uint32 Segs = i - 2;
		int32 TriCount = CalcTessInsideTriCount(Segs);
		int32 PtCount = CalcTessInsidePointCount(Segs);

		FTemplateDesc Desc;

		VertMap.clear();
		NewBaryCoords.clear();
		NewTriangles.clear();

		auto AddGroup = [&]()
		{
			Desc.Offset = Data->GetLength();
			Desc.Tris = (uint16)NewTriangles.size() / 3;
			Desc.Verts = (uint16)NewBaryCoords.size();
			Descs.push_back(Desc);

			Data->Put(NewBaryCoords.data(), (uint32)NewBaryCoords.size() * sizeof(FFloat3));
			Data->Put(NewTriangles.data(), (uint32)NewTriangles.size());
			Data->FillZeroToAlign(4);

			VertMap.clear();
			NewBaryCoords.clear();
			NewTriangles.clear();
		};

		int32 TriOffset = 0;
		while (TriCount - TriOffset > 0)
		{
			uint32 IX, IY, IZ;
			IX = Triangles[TotalTrisOffset + TriOffset].X;
			IY = Triangles[TotalTrisOffset + TriOffset].Y;
			IZ = Triangles[TotalTrisOffset + TriOffset].Z;

			uint32 _Added = NewVertsAdded(IX, IY, IZ);
			if (NewBaryCoords.size() + _Added > 32 || NewTriangles.size() >= 32 * 3)
			{
				AddGroup();
			}

			InsertVert(IX, BaryCoords[TotalPointsOffset + IX]);
			InsertVert(IY, BaryCoords[TotalPointsOffset + IY]);
			InsertVert(IZ, BaryCoords[TotalPointsOffset + IZ]);

			TriOffset++;
		}
		TI_ASSERT((NewBaryCoords.size() > 0 && NewTriangles.size() > 0) ||
			(NewBaryCoords.size() == 0 && NewTriangles.size() == 0));
		uint32 MaxThreadNeedToFinishInside = (uint32)TMath::Max(NewBaryCoords.size(), NewTriangles.size() / 3);
		if (NewBaryCoords.size() > 0)
		{
			TI_ASSERT(NewBaryCoords.size() <= 32 && NewTriangles.size() <= 32 * 3);
			AddGroup();
		}

		FGroupOffAndCount OC;
		OC.Offset = TessGroupOffset;
		OC.Count = (uint32)Descs.size() - TessGroupOffset;
		OC.TrisAfterGroup = TMath::Max(OC.Count - 1, 0) * 32 + MaxThreadNeedToFinishInside;
		OC.TrisBeforeGroup = TriCount;
		GroupOffAndCounts.push_back(OC);

		TotalTrisOffset += TriCount;
		TotalPointsOffset += PtCount;
	}

	TStreamPtr UniformData = ti_new TStream(sizeof(uint32) * MaxTessFactor + sizeof(FTemplateDesc) * (uint32)Descs.size() + Data->GetLength());

	// Add Group Infos
	TVector<uint32> TessGroupInfos;
	TessGroupInfos.reserve(MaxTessFactor);
	for (const auto& OC : GroupOffAndCounts)
	{
		TI_ASSERT(OC.Count < (1 << 6));
		TI_ASSERT(OC.Offset < (1 << 10));
		TI_ASSERT(OC.TrisAfterGroup < (1 << 16));
		uint32 TessGroupInfo = (OC.Offset << 6) | (OC.Count);
		TessGroupInfo |= (OC.TrisAfterGroup << 16);
		TessGroupInfos.push_back(TessGroupInfo);
	}
	UniformData->Put(TessGroupInfos.data(), sizeof(uint32) * (uint32)TessGroupInfos.size());
	uint32 DataOffset = sizeof(uint32) * (uint32)TessGroupInfos.size() + sizeof(FTemplateDesc) * (uint32)Descs.size();
	for (auto& D : Descs)
	{
		D.Offset += DataOffset;
	}
	UniformData->Put(Descs.data(), sizeof(FTemplateDesc) * (uint32)Descs.size());
	// Add Data
	UniformData->Put(Data->GetBuffer(), Data->GetLength());

	const bool ExportToJson = false;
	if (ExportToJson)
	{
		TJSONWriter JRoot;
		JRoot.AddMember("num_tess", MaxTessFactor);
		TVector<int32> _GroupOffs, _GroupCounts;
		_GroupOffs.reserve(GroupOffAndCounts.size());
		_GroupCounts.reserve(GroupOffAndCounts.size());
		for (const auto& OC : GroupOffAndCounts)
		{
			TI_ASSERT(OC.Count < (1 << 6));
			TI_ASSERT(OC.Offset < (1 << 10));
			TI_ASSERT(OC.TrisAfterGroup < (1 << 16));
			_GroupOffs.push_back(OC.Offset);
			_GroupCounts.push_back(OC.Count);
		}
		JRoot.AddMember("group_offsets", _GroupOffs);
		JRoot.AddMember("group_counts", _GroupCounts);
		JRoot.AddMember("num_groups", (uint32)Descs.size());
		TVector<int32> _DataOffs, _GroupVerts, _GroupTris;
		_DataOffs.reserve(Descs.size());
		_GroupVerts.reserve(Descs.size());
		_GroupTris.reserve(Descs.size());
		for (const auto& D : Descs)
		{
			_DataOffs.push_back(D.Offset);
			_GroupVerts.push_back(D.Verts);
			_GroupTris.push_back(D.Tris);
		}
		JRoot.AddMember("data_offsets", _DataOffs);
		JRoot.AddMember("group_verts", _GroupVerts);
		JRoot.AddMember("group_tris", _GroupTris);

		TJSONNode JTessDatas = JRoot.AddArray("tess_datas");
		for (int32 i = 0; i < MaxTessFactor; i++)
		{
			int32 GroupOff = GroupOffAndCounts[i].Offset;
			int32 GroupCount = GroupOffAndCounts[i].Count;

			TJSONNode JTessLevel = JTessDatas.InsertEmptyObjectToArray();
			JTessLevel.AddMember("tess", i);

			TJSONNode JGroupDatas = JTessLevel.AddArray("group_datas");
			for (int32 g = 0; g < GroupCount; g++)
			{
				int32 DOffset = Descs[GroupOff + g].Offset;
				int32 NumVerts = Descs[GroupOff + g].Verts;
				int32 NumTris = Descs[GroupOff + g].Tris;
				TVector<float> Verts;
				Verts.reserve(NumVerts * 3);
				TVector<int32> Tris;
				Tris.reserve(NumTris * 3);

				TJSONNode JGroupData = JGroupDatas.InsertEmptyObjectToArray();
				JGroupData.AddMember("num_verts", NumVerts);
				JGroupData.AddMember("num_tris", NumTris);
				auto LoadByte = [](TStreamPtr _Data, uint32 _Addr)
				{
					uint32 addr4 = _Addr / 4 * 4;
					uint32 v = _Data->GetU32(addr4);
					uint32 offset = _Addr - addr4;
					offset *= 8;
					return (v >> offset) & ((1u << 8) - 1u);
				};

				for (int32 t = 0; t < NumTris; t++)
				{
					int32 Offset = DOffset + NumVerts * 12 + t * 3;
					FUInt3 Tri;
					Tri.X = LoadByte(UniformData, Offset);
					Tri.Y = LoadByte(UniformData, Offset + 1);
					Tri.Z = LoadByte(UniformData, Offset + 2);
					Tris.push_back(Tri.X);
					Tris.push_back(Tri.Y);
					Tris.push_back(Tri.Z);
				}
				JGroupData.AddMember("tris", Tris);
				for (int32 v = 0; v < NumVerts; v++)
				{
					int32 Offset = DOffset + v * 12;
					FUInt3 VertI;
					VertI.X = UniformData->GetU32(Offset);
					VertI.Y = UniformData->GetU32(Offset + 4);
					VertI.Z = UniformData->GetU32(Offset + 8);
					FFloat3 Vert = *(FFloat3*)(&VertI);
					Verts.push_back(Vert.X);
					Verts.push_back(Vert.Y);
					Verts.push_back(Vert.Z);
				}
				JGroupData.AddMember("verts", Verts);
			}
		}
		TString JsonString;
		JRoot.Dump(JsonString);

		TFile F;
		if (F.Open("tess_template.json", EFA_CREATEWRITE))
		{
			F.Write(JsonString.c_str(), (int32)JsonString.length());
			F.Close();
		}
	}

	return UniformData;
}

void FNaniteLearningRenderer::CreateTessellationTemplates(FRHICmdList* RHICmdList)
{

	uint32 TotalTris = 0;
	uint32 TotalPoints = 0;
	for (uint32 i = 2; i <= MaxTessFactor; i++)
	{
		uint32 Segs = i - 2;
		uint32 TriCount = CalcTessInsideTriCount(Segs);
		TotalTris += TriCount;
		uint32 PtCount = CalcTessInsidePointCount(Segs);
		TotalPoints += PtCount;
	}

	// USELESS
	TVector<int32> TriCounts, TriTotal, TriGroups;
	TVector<int32> PtCounts, PtTotal, PtGroups;
	TriCounts.reserve(64);
	TriTotal.reserve(64);
	TriGroups.reserve(64);
	PtCounts.reserve(64);
	PtTotal.reserve(64);
	PtGroups.reserve(64);
	for (int32 i = 2; i <= 64; i++)
	{
		int32 TriPerTess = CalcTessInsideTriCount(i);
		TriCounts.push_back(TriPerTess);
		TriTotal.push_back(TriPerTess * 64);
		TriGroups.push_back(TMath::DivideAndRoundUp(TriPerTess * 64, 32));
		int32 PtPerTess = CalcTessInsidePointCount(i);
		PtCounts.push_back(PtPerTess);
		PtTotal.push_back(PtPerTess * 64);
		PtGroups.push_back(TMath::DivideAndRoundUp(PtPerTess * 64, 32));
	}
	// USELESS


	// Calc Templates
	TVector<FFloat3> BaryCoords;
	BaryCoords.reserve(TotalPoints);
	AddPoints(BaryCoords);

	TVector<FUInt3> Triangles;
	Triangles.reserve(TotalTris);
	AddTriangles(Triangles);

	// Group them into 32 triangles per group
	TVector<uint32> BCOffsets;
	TVector<uint32> TriOffsets;
	// Structure:
	// |tess_group_offset + group_count| - |offset + tris + verts| - |verts_data + tris_data|
	TStreamPtr UniformData = GroupTrianglesAndVerts(BaryCoords, Triangles);


	TI_ASSERT(TessTemplateData == nullptr);
	TessTemplateData =
		FUniformBuffer::CreateBuffer(
			RHICmdList,
			"Nanite.TessTemplateData",
			4,
			TMath::DivideAndRoundUp(UniformData->GetLength(), 4u),
			(uint32)EGPUResourceFlag::Uav | (uint32)EGPUResourceFlag::ByteAddressBuffer,
			UniformData,
			EGPUResourceState::UnorderedAccess);
}

static bool bFreezeCulling = false;
void FNaniteLearningRenderer::FreezeCulling()
{
	bFreezeCulling = !bFreezeCulling;
}

void FNaniteLearningRenderer::Render(FRHICmdList * RHICmdList)
{
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
