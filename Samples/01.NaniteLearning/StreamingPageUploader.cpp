/*
	TiX Engine v3.0 Copyright (C) 2018~2021
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "StreamingPageUploader.h"
#include "NaniteMesh.h"
#include "NaniteEncode.h"

const int32 MaxPageInstallsPerUpdate = 128;

struct FPageInstallInfo
{
	uint32 SrcPageOffset;
	uint32 DstPageOffset;
	uint32 PageDependenciesStart;
	uint32 PageDependenciesNum;
};
enum { INDEX_NONE = -1 };
struct FPageKey
{
	uint32 RuntimeResourceID = INDEX_NONE;
	uint32 PageIndex = INDEX_NONE;
	
	inline bool operator < (const FPageKey& Other) const
	{
		return RuntimeResourceID != Other.RuntimeResourceID ? RuntimeResourceID < Other.RuntimeResourceID : PageIndex < Other.PageIndex;
	}
};
uint32 GPUPageIndexToGPUOffset(int32 PageIndex)
{
	return (TMath::Min(PageIndex, (int32)GetMaxStreamingPages()) << NANITE_STREAMING_PAGE_GPU_SIZE_BITS) + ((uint32)TMath::Max((int32)PageIndex - (int32)GetMaxStreamingPages(), 0) << NANITE_ROOT_PAGE_GPU_SIZE_BITS);
}

FStreamingPageUploader::FStreamingPageUploader()
{
}

FStreamingPageUploader::~FStreamingPageUploader()
{
}

void FStreamingPageUploader::Init(FRHICmdList* RHICmdList)
{
	FRHI* RHI = FRHI::Get();
	TI_ASSERT(TThread::AccquireId() == RHICmdList->WorkingThread);

	// Allcoate Buffers

	// PageDependenciesBuffer
	TI_ASSERT(PageDependenciesBuffer == nullptr);

}

FUniformBufferPtr FStreamingPageUploader::AllocateClusterPageBuffer(FRHICmdList* RHICmdList)
{
	const int32 NumAllocatedRootPages = 2048;
	const int32 NumAllocatedPages = GetMaxStreamingPages() + NumAllocatedRootPages;
	const uint32 AllocatedPagesSize = GPUPageIndexToGPUOffset(NumAllocatedPages);

	FUniformBufferPtr Buffer = FUniformBuffer::CreateBuffer(
		RHICmdList,
		"Nanite.StreamingManager.ClusterPageData",
		sizeof(uint32),
		AllocatedPagesSize / sizeof(uint32),
		(uint32)EGPUResourceFlag::Uav | (uint32)EGPUResourceFlag::ByteAddressBuffer
	);
	RHICmdList->SetGPUBufferState(Buffer->GetGPUBuffer(), EGPUResourceState::UnorderedAccess);

	return Buffer;
}

FUniformBufferPtr FStreamingPageUploader::AllocateHierarchyBuffer(FRHICmdList* RHICmdList, const TVector<FPackedHierarchyNode>& HierarchyNodes)
{
	const int32 MaxHierarchyNodes = TNaniteMesh::MaxHierarchyNodes;
	const uint32 AllocatedNodesSize = TMath::RoundUpToPowerOfTwo(MaxHierarchyNodes) * sizeof(FPackedHierarchyNode);

	TStreamPtr NodesData = nullptr;
	if (HierarchyNodes.size() > 0)
	{
		NodesData = ti_new TStream(HierarchyNodes.data(), (uint32)(HierarchyNodes.size() * sizeof(FPackedHierarchyNode)));
	}

	FUniformBufferPtr Buffer = FUniformBuffer::CreateBuffer(
		RHICmdList,
		"Nanite.StreamingManager.Hierarchy",
		sizeof(uint32),
		AllocatedNodesSize / sizeof(uint32),
		(uint32)EGPUResourceFlag::ByteAddressBuffer,
		NodesData
	);
	RHICmdList->SetGPUBufferState(Buffer->GetGPUBuffer(), EGPUResourceState::NonPixelShaderResource);

	return Buffer;
}

struct FAddedPageInfo
{
	FPageInstallInfo	InstallInfo;
	FPageKey			GPUPageKey;
	uint32				InstallPassIndex;
};

inline float asfloat(uint32 v)
{
	return *(float*)(&v);
}

inline FSpheref asfloat_sphere(const FUInt4& v)
{
	FSpheref S;
	S.Center.X = asfloat(v.X);
	S.Center.Y = asfloat(v.Y);
	S.Center.Z = asfloat(v.Z);
	S.W = asfloat(v.W);
	return S;
}

inline FFloat4 asfloat_float4(const FUInt4& v)
{
	FFloat4 F;
	F.X = asfloat(v.X);
	F.Y = asfloat(v.Y);
	F.Z = asfloat(v.Z);
	F.W = asfloat(v.W);
	return F;
}

inline float f16tof32(uint32 v)
{
	TI_ASSERT(v < (1 << 16));
	uint16 u16 = (uint16)v;
	half h = *(half*)(&u16);
	return h.operator float();
}

void UnpackClusterInstance(const TVector<FUInt4>& ClusterInstanceData)
{
	TI_ASSERT(ClusterInstanceData.size() == NANITE_NUM_PACKED_CLUSTER_INSTANCE_FLOAT4S);

	FClusterInstance ClusterInstance;

	ClusterInstance.TransformedLODBounds = asfloat_sphere(ClusterInstanceData[0]);

	//ClusterInstance.BoxBoundsCenter		= asfloat(ClusterInstanceData[3].xyz);
	// tix : BoxBoundsCenter is BoxBoundsCenter16_MipLevel; decode it
	FFloat3 BoxBoundsCenter;
	uint32 MipLevel;
	BoxBoundsCenter.X = f16tof32(ClusterInstanceData[1].X >> 16);
	BoxBoundsCenter.Y = f16tof32(ClusterInstanceData[1].X & 0xffffu);
	BoxBoundsCenter.Z = f16tof32(ClusterInstanceData[1].Y >> 16);
	MipLevel = ClusterInstanceData[1].Y & 0xffffu;

	uint32 PageIndex = ClusterInstanceData[1].Z >> NANITE_MAX_CLUSTERS_PER_PAGE_BITS;
	uint32 ClusterIndex = ClusterInstanceData[1].Z & NANITE_MAX_CLUSTERS_PER_PAGE_MASK;

	ClusterInstance.LODError = f16tof32(ClusterInstanceData[1].W & 0xffffu);
	ClusterInstance.TransformedEdgeLength = f16tof32(ClusterInstanceData[1].W >> 16);

	FFloat3 BoxBoundsExtent;
	BoxBoundsExtent.X = asfloat(ClusterInstanceData[2].X);
	BoxBoundsExtent.Y = asfloat(ClusterInstanceData[2].Y);
	BoxBoundsExtent.Z = asfloat(ClusterInstanceData[2].Z);

	uint32 Flags = ClusterInstanceData[2].W & 0xffffu;
	uint32 GroupIndex = ClusterInstanceData[2].W >> 16;			// Debug only

	// tix TODO : load transform
	FFloat4 t0 = asfloat_float4(ClusterInstanceData[3]);
	FFloat4 t1 = asfloat_float4(ClusterInstanceData[4]);
	FFloat4 t2 = asfloat_float4(ClusterInstanceData[5]);

	ClusterInstance.ClusterId = 0;

}

void FStreamingPageUploader::DoLoadTest(TNaniteMesh* NaniteMesh)
{
	const uint32 TargetPageIndex = 1;
	const uint32 TargetCIIndex = 27;

	const TVector< FPageStreamingState >& PageStreamingStates = NaniteMesh->PageStreamingStates;
	const FPageStreamingState& PageStreamingState = PageStreamingStates[TargetPageIndex];

	bool bIsRootPage = NaniteMesh->IsRootPage(TargetPageIndex);

	const TVector<uint8>& BulkData = bIsRootPage ? NaniteMesh->RootData : NaniteMesh->StreamablePages;
	TI_ASSERT(BulkData.size() > 0);
	const uint8* SrcPtr = BulkData.data() + PageStreamingState.BulkOffset;

	const uint32 FixupChunkSize = ((const FFixupChunk*)SrcPtr)->GetSize();
	TI_ASSERT(PageStreamingState.PageSize == PageStreamingState.BulkSize - FixupChunkSize);

	const uint8* PageData = SrcPtr + FixupChunkSize;
	FPageDiskHeader* Header = (FPageDiskHeader*)PageData;
	const uint32 NumPageClusters = Header->NumClustersAndInstances >> 16;
	const uint32 NumPageClusterInstances = Header->NumClustersAndInstances & 0xffff;

	PageData += sizeof(FPageDiskHeader);

	FClusterDiskHeader* ClusterDiskHeaders = (FClusterDiskHeader*)PageData;
	PageData += sizeof(FClusterDiskHeader) * NumPageClusters;

	FPageGPUHeader* GPUHeader = (FPageGPUHeader*)PageData;
	const uint8* GPUPageData = PageData;

	const uint32 ClusterInstanceSOAStride = (NumPageClusterInstances << 4);
	const uint32 ClusterInstanceBaseAddress = (TargetCIIndex << 4) + NumPageClusters * NANITE_NUM_PACKED_CLUSTER_FLOAT4S * 16;

	TVector<FUInt4> ClusterInstanceData;
	ClusterInstanceData.resize(NANITE_NUM_PACKED_CLUSTER_INSTANCE_FLOAT4S);
	uint8* DstAddress = (uint8*)ClusterInstanceData.data();
	const uint32 Float4Size = sizeof(FUInt4);

	for (int i = 0; i < NANITE_NUM_PACKED_CLUSTER_INSTANCE_FLOAT4S; i++)
	{
		uint32 TargetAddress = ClusterInstanceBaseAddress + i * ClusterInstanceSOAStride + NANITE_GPU_PAGE_HEADER_SIZE;
		memcpy(DstAddress + Float4Size * i, GPUPageData + TargetAddress, Float4Size);
	}
	
	UnpackClusterInstance(ClusterInstanceData);
}

void FStreamingPageUploader::ProcessNewResources(FRHICmdList* RHICmdList, TNaniteMesh* NaniteMesh, FUniformBufferPtr DstBuffer)
{
	// Tix:Process all pages in this case

	FRHI* RHI = FRHI::Get();
	TI_ASSERT(TThread::AccquireId() == RHICmdList->WorkingThread);
	
	// PageUploadBuffer
	TI_ASSERT(PageUploadBuffer == nullptr);
	const int32 MaxPageBytes = MaxPageInstallsPerUpdate * NANITE_MAX_PAGE_DISK_SIZE;
	PageUploadBuffer = FUniformBuffer::CreateBuffer(
		RHICmdList,
		"Nanite.PageUploadBuffer",
		sizeof(uint32),
		MaxPageBytes / sizeof(uint32),
		(uint32)EGPUResourceFlag::Intermediate | (uint32)EGPUResourceFlag::ByteAddressBuffer
	);

	// InstallInfoUploadBuffer
	TI_ASSERT(InstallInfoUploadBuffer == nullptr);
	// UE dynamically update this 'NumPages' to alloc a suitable buffer size.
	// We have totally 43 pages, load them all 
	const int32 NumAllocPages = 64;
	const int32 InstallInfoAllocationSize = TMath::RoundUpToPowerOfTwo(NumAllocPages * sizeof(FPageInstallInfo));
	InstallInfoUploadBuffer = FUniformBuffer::CreateBuffer(
		RHICmdList,
		"Nanite.InstallInfoUploadBuffer",
		sizeof(FPageInstallInfo),
		InstallInfoAllocationSize / sizeof(FPageInstallInfo),
		(uint32)EGPUResourceFlag::Intermediate
	);

	// PageDependenciesBuffer
	TI_ASSERT(PageDependenciesBuffer == nullptr);
	const int32 PageDependenciesAllocationSize = TMath::RoundUpToPowerOfTwo(4096 * sizeof(uint32));
	PageDependenciesBuffer = FUniformBuffer::CreateBuffer(
		RHICmdList,
		"Nanite.PageDependenciesBuffer",
		sizeof(uint32),
		PageDependenciesAllocationSize / sizeof(uint32),
		(uint32)EGPUResourceFlag::Intermediate
	);

	// We upload all pages at once
	const int32 MaxInstallPages = 32;
	ClusterFixupUploadCS = ti_new FScatterUploadCS;
	ClusterFixupUploadCS->Finalize();
	ClusterFixupUploadCS->Reset(MaxInstallPages * NANITE_MAX_CLUSTER_INSTANCES_PER_PAGE, DstBuffer);

	RedirectClusterIdForClusterInstances(NaniteMesh);

	//DoLoadTest(NaniteMesh);

	// Fill Src Buffer
	uint8* PageUploadBufferPtr = PageUploadBuffer->GetGPUBuffer()->Lock();
	// Install pages
	// Must be processed in PendingPages order so FFixupChunks are loaded when we need them.
	TVector<uint32> FlattenedPageDependencies;
	TVector<FAddedPageInfo> AddedPageInfos;
	FlattenedPageDependencies.reserve(1024);
	TMap<FPageKey, uint32>	GPUPageKeyToAddedIndex;
	AddedPageInfos.reserve(1024);
	{
		const uint32 NumReadyPages = (uint32)NaniteMesh->PageStreamingStates.size();
		uint32 NumInstalledPages = 0;
		int32 NextPageByteOffset = 0;
		// Tix: PageIndex 0 is the RootPage
		for (uint32 PageIndex = 0; PageIndex < NumReadyPages; PageIndex++)
		{
			//uint32 PendingPageIndex = (StartPendingPageIndex + TaskIndex) % MaxPendingPages;
			//FPendingPage& PendingPage = PendingPages[PendingPageIndex];

			//FUploadTask& UploadTask = UploadTasks[TaskIndex];
			//UploadTask.PendingPage = &PendingPage;

			//FResources** Resources = RuntimeResourceMap.Find(PendingPage.InstallKey.RuntimeResourceID);
			//uint32 LastPendingPageIndex = GPUPageToLastPendingPageIndex.FindChecked(PendingPages[PendingPageIndex].GPUPageIndex);
			//if (PendingPageIndex != LastPendingPageIndex || !Resources)
			//{
			//	continue;	// Skip resource install. Resource no longer exists or page has already been overwritten.
			//}

			const TVector< FPageStreamingState >& PageStreamingStates = NaniteMesh->PageStreamingStates;
			const FPageStreamingState& PageStreamingState = PageStreamingStates[PageIndex];
			//FStreamingPageInfo* StreamingPage = &StreamingPageInfos[PendingPage.GPUPageIndex];

			//CommittedStreamingPageMap.Add(PendingPage.InstallKey, StreamingPage);

			//ModifiedResources.Add(PendingPage.InstallKey.RuntimeResourceID);

			bool bIsRootPage = NaniteMesh->IsRootPage(PageIndex);

			const TVector<uint8>& BulkData = bIsRootPage ? NaniteMesh->RootData : NaniteMesh->StreamablePages;
			TI_ASSERT(BulkData.size() > 0);
			const uint8* SrcPtr = BulkData.data() + PageStreamingState.BulkOffset;
//#if WITH_EDITOR
//			const uint8* SrcPtr;
//			if ((*Resources)->ResourceFlags & NANITE_RESOURCE_FLAG_STREAMING_DATA_IN_DDC)
//			{
//				SrcPtr = (const uint8*)PendingPage.SharedBuffer.GetData();
//			}
//			else
//			{
//				// Make sure we only lock each resource BulkData once.
//				const uint8** BulkDataPtrPtr = ResourceToBulkPointer.Find(*Resources);
//				if (BulkDataPtrPtr)
//				{
//					SrcPtr = *BulkDataPtrPtr + PageStreamingState.BulkOffset;
//				}
//				else
//				{
//					FByteBulkData& BulkData = (*Resources)->StreamablePages;
//					check(BulkData.IsBulkDataLoaded() && BulkData.GetBulkDataSize() > 0);
//					const uint8* BulkDataPtr = (const uint8*)BulkData.LockReadOnly();
//					ResourceToBulkPointer.Add(*Resources, BulkDataPtr);
//					SrcPtr = BulkDataPtr + PageStreamingState.BulkOffset;
//				}
//			}
//#else
//			const uint8* SrcPtr = PendingPage.RequestBuffer.GetData();
//#endif

			//FFixupChunk* FixupChunk = (FFixupChunk*)FMemory::Realloc(StreamingPageFixupChunks[PendingPage.GPUPageIndex], FixupChunkSize, sizeof(uint16));
			//StreamingPageFixupChunks[PendingPage.GPUPageIndex] = FixupChunk;
			//FMemory::Memcpy(FixupChunk, SrcPtr, FixupChunkSize);

			// Build list of GPU page dependencies
			TVector<uint32> GPUPageDependencies;
			GPUPageDependencies.reserve(1024);
			if (PageStreamingState.Flags & NANITE_PAGE_FLAG_RELATIVE_ENCODING)
			{
				for (uint32 i = 0; i < PageStreamingState.DependenciesNum; i++)
				{
					const uint32 DependencyPageIndex = NaniteMesh->PageDependencies[PageStreamingState.DependenciesStart + i];
					if (NaniteMesh->IsRootPage(DependencyPageIndex))
					{
						// Tix:NaniteMesh->RootPageIndex always = ZERO in this case
						GPUPageDependencies.push_back(GetMaxStreamingPages() + 0/*NaniteMesh->RootPageIndex*/ + DependencyPageIndex);
					}
					else
					{
						// Tix: We install page in order, do not need a committed map, RootPage(Index is 0) always stay at index = MaxStreamingPages(4096)
						//FPageKey DependencyKey = { PendingPage.InstallKey.RuntimeResourceID, DependencyPageIndex };
						//FStreamingPageInfo** DependencyPagePtr = CommittedStreamingPageMap.Find(DependencyKey);
						//check(DependencyPagePtr != nullptr);
						//GPUPageDependencies.Add((*DependencyPagePtr)->GPUPageIndex);
						GPUPageDependencies.push_back(DependencyPageIndex - 1);
					}
				}
			}

			uint32 GPUPageIndex = bIsRootPage ? GetMaxStreamingPages() : PageIndex - 1;

			const uint32 FixupChunkSize = ((const FFixupChunk*)SrcPtr)->GetSize();
			const FFixupChunk* FixupChunkPtr = (const FFixupChunk*)SrcPtr;
			FixupMap[GPUPageIndex] = FixupChunkPtr;
			TI_ASSERT(PageStreamingState.PageSize == PageStreamingState.BulkSize - FixupChunkSize);

			uint32 PageOffset = GPUPageIndexToGPUOffset(GPUPageIndex);
			uint32 DataSize = PageStreamingState.BulkSize - FixupChunkSize;
			TI_ASSERT(NumInstalledPages < MaxPageInstallsPerUpdate);

			const FPageKey GPUPageKey = FPageKey{ 0, GPUPageIndex };
			const uint32 AddedPageIndex = (uint32)AddedPageInfos.size();

			const uint8* UploadSrc = SrcPtr + FixupChunkSize;
			uint8* UploadDst = PageUploadBufferPtr + NextPageByteOffset;
			FAddedPageInfo AddedPageInfo;
			AddedPageInfo.GPUPageKey = GPUPageKey;
			AddedPageInfo.InstallInfo.SrcPageOffset = NextPageByteOffset;
			AddedPageInfo.InstallInfo.DstPageOffset = PageOffset;
			AddedPageInfo.InstallInfo.PageDependenciesStart = (uint32)FlattenedPageDependencies.size();
			AddedPageInfo.InstallInfo.PageDependenciesNum = (uint32)GPUPageDependencies.size();
			AddedPageInfo.InstallPassIndex = 0xFFFFFFFFu;
			AddedPageInfos.push_back(AddedPageInfo);

			//FlattenedPageDependencies.Append(PageDependencies);
			FlattenedPageDependencies.insert(FlattenedPageDependencies.end(), GPUPageDependencies.begin(), GPUPageDependencies.end());
			GPUPageKeyToAddedIndex[GPUPageKey] = AddedPageIndex;

			NextPageByteOffset += DataSize;

			memcpy(UploadDst, UploadSrc, DataSize);
			//const FPageKey GPUPageKey = FPageKey{ PendingPage.InstallKey.RuntimeResourceID, PendingPage.GPUPageIndex };

			//UploadTask.PendingPage = &PendingPage;
			//UploadTask.Dst = PageUploader->Add_GetRef(DataSize, PageOffset, GPUPageKey, GPUPageDependencies);
			//UploadTask.Src = SrcPtr + FixupChunkSize;
			//UploadTask.SrcSize = DataSize;
			NumInstalledPages++;

			// Apply fixups to install page
			//StreamingPage->ResidentKey = PendingPage.InstallKey;
			ApplyFixups(*FixupChunkPtr, NaniteMesh, BulkData);

			//INC_DWORD_STAT(STAT_NaniteInstalledPages);
			//INC_DWORD_STAT(STAT_NanitePageInstalls);

		}
	}
	PageUploadBuffer->GetGPUBuffer()->Unlock();

	// Fill Dependency Data
	uint8* PageDependenciesPtr = PageDependenciesBuffer->GetGPUBuffer()->Lock();
	memcpy(PageDependenciesPtr, FlattenedPageDependencies.data(), FlattenedPageDependencies.size() * sizeof(uint32));
	PageDependenciesBuffer->GetGPUBuffer()->Unlock();

	// Fill Install Info
	// Split page installs into passes.
	// Every pass adds the pages that no longer have any unresolved dependency.
	// Essentially a naive multi-pass topology sort, but with a low number of passes in practice.
	FPageInstallInfo* InstallInfoPtr = (FPageInstallInfo*)InstallInfoUploadBuffer->GetGPUBuffer()->Lock();
	TVector<uint32> NumInstalledPagesPerPass;
	NumInstalledPagesPerPass.reserve(1024);
	const uint32 NumPages = (uint32)AddedPageInfos.size();
	uint32 NumRemainingPages = NumPages;
	while (NumRemainingPages > 0)
	{
		const uint32 CurrentPassIndex = (uint32)NumInstalledPagesPerPass.size();
		uint32 NumPassPages = 0;
		int32 PIndex = -1;
		for (FAddedPageInfo& PageInfo : AddedPageInfos)
		{
			PIndex++;
			if (PageInfo.InstallPassIndex < CurrentPassIndex)
				continue;	// Page already installed in an earlier pass

			bool bMissingDependency = false;
			for (uint32 i = 0; i < PageInfo.InstallInfo.PageDependenciesNum; i++)
			{
				const uint32 GPUPageIndex = FlattenedPageDependencies[PageInfo.InstallInfo.PageDependenciesStart + i];
				const FPageKey DependencyGPUPageKey = { PageInfo.GPUPageKey.RuntimeResourceID, GPUPageIndex };
				TMap<FPageKey, uint32>::const_iterator It = GPUPageKeyToAddedIndex.find(DependencyGPUPageKey);
				//const uint32* DependencyAddedIndexPtr = GPUPageKeyToAddedIndex.Find(DependencyGPUPageKey);

				// Check if a dependency has not yet been installed.
				// We only need to resolve dependencies in the current batch. Batches are already ordered.
				if (It != GPUPageKeyToAddedIndex.end() && AddedPageInfos[It->second].InstallPassIndex >= CurrentPassIndex)
				{
					bMissingDependency = true;
					break;
				}
			}

			if (!bMissingDependency)
			{
				*InstallInfoPtr++ = PageInfo.InstallInfo;
				PageInfo.InstallPassIndex = CurrentPassIndex;
				NumPassPages++;
			}
		}

		NumInstalledPagesPerPass.push_back(NumPassPages);
		NumRemainingPages -= NumPassPages;
	}
	InstallInfoUploadBuffer->GetGPUBuffer()->Unlock();

	// Create shader
	TranscodeCS = ti_new FTranscodeCS();
	TranscodeCS->Finalize();


	// Install at once
	const uint32 NumPasses = (uint32)NumInstalledPagesPerPass.size();
	FRenderResourceTablePtr ResourceTable = RHICmdList->GetHeap(0)->CreateRenderResourceTable(FTranscodeCS::PARAM_NUM);
	FRHI::Get()->PutUniformBufferInTable(ResourceTable, InstallInfoUploadBuffer, FTranscodeCS::SRV_InstallInfoBuffer);
	FRHI::Get()->PutUniformBufferInTable(ResourceTable, PageDependenciesBuffer, FTranscodeCS::SRV_PageDependenciesBuffer);
	FRHI::Get()->PutUniformBufferInTable(ResourceTable, PageUploadBuffer, FTranscodeCS::SRV_SrcPageBuffer);
	FRHI::Get()->PutRWUniformBufferInTable(ResourceTable, DstBuffer, FTranscodeCS::UAV_DstPageBuffer);

	TranscodeCS->BindComputePipeline(RHICmdList);

	uint32 StartPageIndex = 0;
	int8 EventName[128];
	for (uint32 PassIndex = 0; PassIndex < NumPasses; PassIndex++)
	{
		const uint32 NumPagesInPass = NumInstalledPagesPerPass[PassIndex];

		sprintf(EventName, "TranscodePageToGPU (PageOffset: %u, PageCount: %u)", StartPageIndex, NumPagesInPass);

		FDecodeInfo DecodeInfo;
		DecodeInfo.StartPageIndex = StartPageIndex;
		DecodeInfo.PageConstants.Y = GetMaxStreamingPages();

		RHICmdList->BeginEvent(EventName);
#define TRANSCODE_THREADS_PER_GROUP_BITS	7
#define TRANSCODE_THREADS_PER_GROUP			(1 << TRANSCODE_THREADS_PER_GROUP_BITS)
		RHICmdList->SetComputeConstant(FTranscodeCS::RC_DecodeInfo, &DecodeInfo, sizeof(FDecodeInfo) / sizeof(uint32));
		RHICmdList->SetComputeResourceTable(FTranscodeCS::RT_Table, ResourceTable); 
		RHICmdList->DispatchCompute(
			FInt3(TRANSCODE_THREADS_PER_GROUP, 1, 1), 
			FInt3(NANITE_MAX_TRANSCODE_GROUPS_PER_PAGE, NumPagesInPass, 1)
		);
		RHICmdList->EndEvent();

		StartPageIndex += NumPagesInPass;
	}

	// Copy fixups
	//ClusterFixupUploadCS->Run(RHICmdList);
}

void FStreamingPageUploader::ApplyFixups(const FFixupChunk& FixupChunk, TNaniteMesh* NaniteMesh, const TVector<uint8>& BulkData)
{
	// tix : since we upload all pages at once. 
	// we can apply Cluster and Hierarchy fixup here for easy case

	bool bUninstall = false;
	uint32 Flags = bUninstall ? NANITE_CLUSTER_FLAG_LEAF : 0;
	// Fixup clusters
	for (uint32 i = 0; i < FixupChunk.Header.NumClusterFixups; i++)
	{
		const FClusterFixup& Fixup = FixupChunk.GetClusterFixup(i);

		//bool bPageDependenciesCommitted = bUninstall || ArePageDependenciesCommitted(RuntimeResourceID, Fixup.GetPageDependencyStart(), Fixup.GetPageDependencyNum());
		//if (!bPageDependenciesCommitted)
		//	continue;

		uint32 TargetPageIndex = Fixup.GetPageIndex();
		uint32 TargetGPUPageIndex = 0xffffffffu;
		uint32 NumTargetPageClusterInstances = 0;

		if (NaniteMesh->IsRootPage(TargetPageIndex))
		{
			TargetGPUPageIndex = GetMaxStreamingPages() + 0/*Resources.RootPageIndex*/ + TargetPageIndex;
			NumTargetPageClusterInstances = NaniteMesh->NumRootPageClusterInstances;
				//RootPageInfos[Resources.RootPageIndex + TargetPageIndex].NumClusters;
		}
		else
		{
			TI_ASSERT(TargetPageIndex > 0);
			uint32 TargetGPUIndex = TargetPageIndex - 1;
			//FPageKey TargetKey = { RuntimeResourceID, TargetPageIndex };
			//FStreamingPageInfo** TargetPagePtr = CommittedStreamingPageMap.Find(TargetKey);

			//check(bUninstall || TargetPagePtr);
			//if (TargetPagePtr)
			//{
				THMap<uint32, const FFixupChunk*>::const_iterator It = FixupMap.find(TargetGPUIndex);
				TI_ASSERT(It != FixupMap.end());
				const FFixupChunk* TargetFixupChunk = It->second;
				//FStreamingPageInfo* TargetPage = *TargetPagePtr;
				//FFixupChunk& TargetFixupChunk = *StreamingPageFixupChunks[TargetPage->GPUPageIndex];
				//check(StreamingPageInfos[TargetPage->GPUPageIndex].ResidentKey == TargetKey);

				NumTargetPageClusterInstances = TargetFixupChunk->Header.NumClusterInstances;
				TI_ASSERT(Fixup.GetClusterInstanceIndex() < NumTargetPageClusterInstances);

				TargetGPUPageIndex = TargetGPUIndex;
			//}
		}

		//if (TargetGPUPageIndex != INVALID_PAGE_INDEX)
		{
			const FPageStreamingState& PageStreamingState = NaniteMesh->PageStreamingStates[TargetPageIndex];
			const uint8* PageDataPtr = BulkData.data() + PageStreamingState.BulkOffset;
			const uint32 FixupChunkSize = ((const FFixupChunk*)PageDataPtr)->GetSize();
			FPageDiskHeader* Header = (FPageDiskHeader*)(PageDataPtr + FixupChunkSize);
			const uint32 NumClusters = Header->NumClustersAndInstances >> 16;
			const uint32 NumClusterInstances = Header->NumClustersAndInstances & 0xffff;
			const uint32 PackedClusterInstanceOffset =
				FixupChunkSize +
				sizeof(FPageDiskHeader) +
				sizeof(FClusterDiskHeader) * NumClusters +
				sizeof(FPageGPUHeader) +
				sizeof(FPackedCluster) * NumClusters +
				32 * NumClusterInstances;	// in SOA layout

			uint32 ClusterInstanceIndex = Fixup.GetClusterInstanceIndex();

			TI_ASSERT(PackedClusterInstanceOffset + 16 * ClusterInstanceIndex + sizeof(FFloat3) < PageStreamingState.PageSize);
			const uint32 Flags_GroupIndex = *(uint32*)(PageDataPtr + PackedClusterInstanceOffset + 16 * ClusterInstanceIndex + sizeof(FFloat3));
			const uint32 GroupIndex = Flags_GroupIndex >> 16;

			const uint32 FlagsOffset = offsetof(FPackedClusterInstance, Flags_GroupIndex);
			const uint32 Offset = 
				GPUPageIndexToGPUOffset(TargetGPUPageIndex) + 
				NANITE_GPU_PAGE_HEADER_SIZE + 
				FixupChunk.Header.NumClusters * NANITE_NUM_PACKED_CLUSTER_FLOAT4S * 16 +
				((FlagsOffset >> 4) * NumTargetPageClusterInstances + ClusterInstanceIndex) * 16 + (FlagsOffset & 15);
			// TIX: todo: fixup here
			const uint32 FlagsFixup = (GroupIndex << 16) | Flags;
			ClusterFixupUploadCS->Add(Offset / sizeof(uint32), FlagsFixup);
		}
	}



	// Fixup hierarchy
	for (uint32 i = 0; i < FixupChunk.Header.NumHierachyFixups; i++)
	{
		const FHierarchyFixup& Fixup = FixupChunk.GetHierarchyFixup(i);

		//bool bPageDependenciesCommitted = true;// bUninstall || ArePageDependenciesCommitted(RuntimeResourceID, Fixup.GetPageDependencyStart(), Fixup.GetPageDependencyNum());
		//if (!bPageDependenciesCommitted)
		//	continue;

		uint32 Target_PageIndex = Fixup.GetPageIndex();
		//FPageKey TargetKey = { RuntimeResourceID, Fixup.GetPageIndex() };
		uint32 TargetGPUPageIndex = 0xFFFFFFFFu;
		//if (!bUninstall)
		{
			if (NaniteMesh->IsRootPage(Target_PageIndex))
			{
				TargetGPUPageIndex = GetMaxStreamingPages() + 0 /*Resources.RootPageIndex*/ + Target_PageIndex;
			}
			else
			{
				TI_ASSERT(Target_PageIndex > 0);
				uint32 TargetGPUIndex = Target_PageIndex - 1;
				//FStreamingPageInfo** TargetPagePtr = CommittedStreamingPageMap.Find(TargetKey);
				//check(TargetPagePtr);
				//check((*TargetPagePtr)->ResidentKey == TargetKey);
				TargetGPUPageIndex = TargetGPUIndex;
			}
		}

		// Uninstalls are unconditional. The same uninstall might happen more than once.
		// If this page is getting uninstalled it also means it wont be reinstalled and any split groups can't be satisfied, so we can safely uninstall them.	

		uint32 HierarchyNodeIndex = Fixup.GetNodeIndex();
		TI_ASSERT(HierarchyNodeIndex < (uint32)NaniteMesh->HierarchyNodes.size());
		uint32 ChildIndex = Fixup.GetChildIndex();
		uint32 ChildStartReference = false ? 0xFFFFFFFFu : ((TargetGPUPageIndex << NANITE_MAX_CLUSTER_INSTANCES_PER_PAGE_BITS) | Fixup.GetClusterGroupPartStartIndex());
		//uint32 Offset = (size_t) & (((FPackedHierarchyNode*)0)[0/*HierarchyOffset*/ + HierarchyNodeIndex].Misc1[ChildIndex].ChildStartReference);
		//Hierarchy.UploadBuffer.Add(Offset / sizeof(uint32), &ChildStartReference);
		NaniteMesh->HierarchyNodes[HierarchyNodeIndex].Misc1[ChildIndex].ChildStartReference = ChildStartReference;
	}
}

void FStreamingPageUploader::RedirectClusterIdForClusterInstances(TNaniteMesh* NaniteMesh)
{
	const uint32 NumReadyPages = (uint32)NaniteMesh->PageStreamingStates.size();
	TVector<uint32> GPUPages;
	GPUPages.resize(NumReadyPages);

	for (uint32 PageIndex = 0; PageIndex < NumReadyPages; PageIndex++)
	{
		if (NaniteMesh->IsRootPage(PageIndex))
		{
			// Tix:NaniteMesh->RootPageIndex always = ZERO in this case
			GPUPages[PageIndex] = GetMaxStreamingPages() + 0/*NaniteMesh->RootPageIndex*/ + PageIndex;
		}
		else
		{
			GPUPages[PageIndex] = PageIndex - 1;
		}
	}

	for (uint32 PageIndex = 0; PageIndex < NumReadyPages; PageIndex++)
	{
		const TVector< FPageStreamingState >& PageStreamingStates = NaniteMesh->PageStreamingStates;
		const FPageStreamingState& PageStreamingState = PageStreamingStates[PageIndex];

		bool bIsRootPage = NaniteMesh->IsRootPage(PageIndex);
		const TVector<uint8>& BulkData = bIsRootPage ? NaniteMesh->RootData : NaniteMesh->StreamablePages;
		TI_ASSERT(BulkData.size() > 0);
		const uint8* PageDataPtr = BulkData.data() + PageStreamingState.BulkOffset;
		const uint32 FixupChunkSize = ((const FFixupChunk*)PageDataPtr)->GetSize();
		FPageDiskHeader* Header = (FPageDiskHeader*)(PageDataPtr + FixupChunkSize);
		const uint32 NumClusters = Header->NumClustersAndInstances >> 16;
		const uint32 NumClusterInstances = Header->NumClustersAndInstances & 0xffff;
		const uint32 PackedClusterInstanceOffset = 
			FixupChunkSize + 
			sizeof(FPageDiskHeader) +
			sizeof(FClusterDiskHeader) * NumClusters +
			sizeof(FPageGPUHeader) +
			sizeof(FPackedCluster) * NumClusters +
			16 * NumClusterInstances;	// in SOA layout

		for (uint32 CIIndex = 0; CIIndex < NumClusterInstances; CIIndex++)
		{
			TI_ASSERT(PackedClusterInstanceOffset + 16 * CIIndex + sizeof(FUInt2) < PageStreamingState.PageSize);
			uint32& PageAndCluster = *(uint32*)(PageDataPtr + PackedClusterInstanceOffset + 16 * CIIndex + sizeof(FUInt2));

			uint32 Page = PageAndCluster >> NANITE_MAX_CLUSTERS_PER_PAGE_BITS;
			uint32 Cluster = PageAndCluster & NANITE_MAX_CLUSTERS_PER_PAGE_MASK;
			Page = GPUPages[Page];

			PageAndCluster = (Page << NANITE_MAX_CLUSTERS_PER_PAGE_BITS) | Cluster;
		}
	}
}
