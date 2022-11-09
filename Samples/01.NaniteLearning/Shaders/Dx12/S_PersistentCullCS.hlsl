#define CULLING_PASS CULLING_PASS_OCCLUSION_MAIN
#define CHECK_AND_TRIM_CLUSTER_COUNT 1

#include "Common.hlsli"
#include "WaveOpUtil.h"
#include "NaniteDataDecode.h"

#define NANITE_HIERARCHY_TRAVERSAL 1

#include "NaniteCulling.h"
#include "NaniteDebugInfo.h"

#define GROUP_NODE_SIZE 2


#if GROUP_NODE_SIZE == 2

groupshared uint2 GroupNodeData[NANITE_MAX_BVH_NODES_PER_GROUP];
uint4 GetGroupNodeData(uint NodeIndex) { return uint4(GroupNodeData[NodeIndex], 0, 0); }
void SetGroupNodeData(uint NodeIndex, uint4 Data) { GroupNodeData[NodeIndex] = Data.xy; }

#elif GROUP_NODE_SIZE == 3

groupshared uint3 GroupNodeData[NANITE_MAX_BVH_NODES_PER_GROUP];
uint4 GetGroupNodeData(uint NodeIndex) { return uint4(GroupNodeData[NodeIndex], 0); }
void SetGroupNodeData(uint NodeIndex, uint4 Data) { GroupNodeData[NodeIndex] = Data.xyz; }

#else
#	error "Unexpected Group Node Size."
#endif

#define NANITE_HIERARCHY_TRAVERSAL_TYPE (CULLING_TYPE)

//#if CULLING_PASS == CULLING_PASS_OCCLUSION_POST
//static const bool bIsPostPass = true;
//static const uint QueueStateIndex = 1;
//#else
static const bool bIsPostPass = false;
static const uint QueueStateIndex = 0;
//#endif


#define DEBUG_MODE 0
#define DEBUG_INFO 1

groupshared uint GroupOccludedBitmask[NANITE_MAX_BVH_NODES_PER_GROUP];

RWCoherentStructuredBuffer(FQueueState)	QueueState : register(u0);

RWCoherentByteAddressBuffer	MainAndPostNodesAndClusterBatches : register(u1);
RWCoherentByteAddressBuffer	MainAndPostCandididateClusters : register(u2);


struct FStreamingRequest
{
	uint RuntimeResourceID_Magic;
	uint PageIndex_NumPages_Magic;
	uint Priority_Magic;
};

Buffer<uint>							OffsetClustersArgsSWHW;
StructuredBuffer<uint2>					InTotalPrevDrawClusters;

RWStructuredBuffer<FStreamingRequest>	OutStreamingRequests : register(u3);			// First entry holds count

RWByteAddressBuffer						OutVisibleClustersSWHW : register(u4);
RWBuffer<uint>							VisibleClustersArgsSWHW : register(u5);
#if DEBUG_INFO
RWStructuredBuffer<FNaniteDebugInfo>	DebugInfo : register(u6);
#endif

#if DEBUG_FLAGS
RWStructuredBuffer<FNaniteStats>		OutStatsBuffer;
#endif

//float									DisocclusionLodScaleFactor;	// TODO: HACK: Force LOD down first frame an instance is visible to mitigate disocclusion spikes.
//uint									LargePageRectThreshold;
//uint									StreamingRequestsBufferVersion;

RWStructuredBuffer<uint>				OutDirtyPageFlags;


// Get the area of an "inclusive" rect (which means that the max is inside the rect), also guards against negative area (where min > max)
uint GetInclusiveRectArea(uint4 Rect)
{
	if (all(Rect.zw >= Rect.xy))
	{
		uint2 Size = Rect.zw - Rect.xy;
		return (Size.x  + 1) * (Size.y + 1);
	}
	return 0;
}

float2 GetProjectedEdgeScales(FNaniteView NaniteView, float4 Bounds)	// float2(min, max)
{
	if( NaniteView.ViewToClip[ 3 ][ 3 ] >= 1.0f )
	{
		// Ortho
		return float2( 1, 1 );
	}
	float3 Center = Bounds.xyz - NaniteView.WorldCameraOrigin.xyz;// mul(float4(Bounds.xyz, 1.0f), DynamicData.LocalToTranslatedWorld).xyz;
	float Radius = Bounds.w;// Bounds.w* InstanceData.NonUniformScale.w;

	//Center *= 100.0;
	//Radius *= 100.0;

	float ZNear = NaniteView.NearPlane;
	float DistToClusterSq = length2( Center );	// camera origin in (0,0,0)
	
	float Z = dot(NaniteView.ViewForward.xyz, Center);
	float XSq = DistToClusterSq - Z * Z;
	float X = sqrt( max(0.0f, XSq) );
	float DistToTSq = DistToClusterSq - Radius * Radius;
	float DistToT = sqrt( max(0.0f, DistToTSq) );
	float ScaledCosTheta = DistToT;
	float ScaledSinTheta = Radius;
	float ScaleToUnit = rcp( DistToClusterSq );
	float By = (  ScaledSinTheta * X + ScaledCosTheta * Z ) * ScaleToUnit;
	float Ty = ( -ScaledSinTheta * X + ScaledCosTheta * Z ) * ScaleToUnit;
	
	float H = ZNear - Z;
	if( DistToTSq < 0.0f || By * DistToT < ZNear )
	{
		float Bx = max( X - sqrt( Radius * Radius - H * H ), 0.0f );
		By = ZNear * rsqrt( Bx * Bx + ZNear * ZNear );
	}

	if( DistToTSq < 0.0f || Ty * DistToT < ZNear )
	{	
		float Tx = X + sqrt( Radius * Radius - H * H );
		Ty = ZNear * rsqrt( Tx * Tx + ZNear * ZNear );
	}

	float MinZ = max( Z - Radius, ZNear );
	float MaxZ = max( Z + Radius, ZNear );
	float MinCosAngle = Ty;
	float MaxCosAngle = By;

	if(Z + Radius > ZNear)
		return float2( MinZ * MinCosAngle, MaxZ * MaxCosAngle );
	else
		return float2( 0.0f, 0.0f );
}

bool ShouldVisitChildInternal( FNaniteView NaniteView, float4 LODBounds, float MinLODError, float MaxParentLODError, inout float Priority, out float4 DebugValue )
{
	float2 ProjectedEdgeScales = GetProjectedEdgeScales(NaniteView, LODBounds);// GetProjectedEdgeScales(NaniteView, InstanceData, DynamicData, LODBounds);
	float UniformScale = 1.0;// min3(InstanceData.NonUniformScale.x, InstanceData.NonUniformScale.y, InstanceData.NonUniformScale.z);
	float Threshold = NaniteView.LODScale * UniformScale * MaxParentLODError;

	DebugValue.x = ProjectedEdgeScales.x;
	DebugValue.y = Threshold;

	if( ProjectedEdgeScales.x <= Threshold )
	{
		Priority = Threshold / ProjectedEdgeScales.x;	// TODO: Experiment with better priority
		// return (ProjectedEdgeScales.y >= NaniteView.LODScale * UniformScale * MinLODError); //TODO: Doesn't currently work with streaming. MinLODError needs to also reflect leafness caused by streaming cut.
		return true;
	}
	else
	{
		return false;
	}
}

bool SmallEnoughToDraw( FNaniteView NaniteView, float4 LODBounds, float LODError, float EdgeLength, inout bool bUseHWRaster )
{
	float ProjectedEdgeScale = GetProjectedEdgeScales(NaniteView, LODBounds).x; ;// GetProjectedEdgeScales(NaniteView, InstanceData, DynamicData, LODBounds).x;
	float UniformScale = 1.0;// min3(InstanceData.NonUniformScale.x, InstanceData.NonUniformScale.y, InstanceData.NonUniformScale.z);
	bool bVisible = ProjectedEdgeScale > UniformScale * LODError * NaniteView.LODScale;

	if (DecodeInfo.RenderFlags & NANITE_RENDER_FLAG_FORCE_HW_RASTER)
	{
		bUseHWRaster = true;
	}
	else
	{
		// https://zhuanlan.zhihu.com/p/377652639
		// InstanceData.NonUniformScale.w is the maximum of NonuniformScale, in GetPrimitiveUniformShaderParameters()
		// Result.NonUniformScale = FVector4(ScaleX, ScaleY, ScaleZ, FMath::Max3(FMath::Abs(ScaleX), FMath::Abs(ScaleY), FMath::Abs(ScaleZ));
		bUseHWRaster = ProjectedEdgeScale < abs(EdgeLength)* NaniteView.LODScaleHW; //ProjectedEdgeScale < InstanceData.NonUniformScale.w * abs( EdgeLength ) * NaniteView.LODScaleHW; // TODO: EdgeLength shouldn't have sign
	}

	return bVisible;
}

//void RequestPageRange( uint RuntimeResourceID, uint StartPageIndex, uint NumPages, uint PriorityCategory, float Priority )
//{
//	if ((DecodeInfo.RenderFlags & NANITE_RENDER_FLAG_OUTPUT_STREAMING_REQUESTS) && NumPages > 0)
//	{
//		uint Index;
//		WaveInterlockedAddScalar_(OutStreamingRequests[0].RuntimeResourceID_Magic, 1, Index);	// HACK: Store count in RuntimeResourceID_Magic of first request.
//		if (Index < NANITE_MAX_STREAMING_REQUESTS - 1)
//		{
//			FStreamingRequest Request;
//			Request.RuntimeResourceID_Magic		= (RuntimeResourceID << NANITE_STREAMING_REQUEST_MAGIC_BITS);
//			Request.PageIndex_NumPages_Magic	= (((StartPageIndex << NANITE_MAX_GROUP_PARTS_BITS) | NumPages) << NANITE_STREAMING_REQUEST_MAGIC_BITS);
//			const uint UIntPriority				= (PriorityCategory << 30) | (asuint(Priority) >> 2);
//			Request.Priority_Magic				= UIntPriority & ~NANITE_STREAMING_REQUEST_MAGIC_MASK;		// Mask off low bits to leave space for magic
//#if NANITE_SANITY_CHECK_STREAMING_REQUESTS
//			const uint FrameNibble = StreamingRequestsBufferVersion & 0xF;
//			Request.RuntimeResourceID_Magic		|= 0xA0 | FrameNibble;
//			Request.PageIndex_NumPages_Magic	|= 0xB0 | FrameNibble;
//			Request.Priority_Magic				|= 0xC0 | FrameNibble;
//#endif
//			OutStreamingRequests[Index + 1]		= Request;
//		}
//	}
//}

struct FNaniteTraversalClusterCullCallback
{
	uint ChildIndex;
	uint LocalNodeIndex;

	FCandidateNode CandidateNode;
	FNaniteView NaniteView;
	//FInstanceSceneData InstanceData;

	bool bVisible;

	float StreamingPriority;

	void Init(uint InChildIndex, uint InLocalNodeIndex, uint GroupNodeFetchIndex, uint GroupID, uint LoopIndex)
	{
		ChildIndex = InChildIndex;
		LocalNodeIndex = InLocalNodeIndex;
		
		const uint4 NodeData = GetGroupNodeData(GroupNodeFetchIndex);

		CandidateNode = UnpackCandidateNode(NodeData, bIsPostPass);

		NaniteView = GetNaniteView(CandidateNode.ViewId);
#if CULLING_PASS == CULLING_PASS_OCCLUSION_POST
		if (CandidateNode.Flags & NANITE_CULLING_FLAG_FROM_DISOCCLUDED_INSTANCE)
			NaniteView.LODScale *= DisocclusionLodScaleFactor;
#endif

		//InstanceData = GetInstanceSceneData(CandidateNode.InstanceId, false);
	}

	int GetHierarchyNodeIndex()
	{
		//return InstanceData.NaniteHierarchyOffset + CandidateNode.NodeIndex;
		return CandidateNode.NodeIndex;
	}

	bool ShouldVisitChild(FHierarchyNodeSlice HierarchyNodeSlice, bool bInVisible, out float4 DebugValue)
	{
		bVisible = bInVisible;
		bool bWasOccluded = false;

		//FInstanceDynamicData DynamicData = CalculateInstanceDynamicData(NaniteView, InstanceData);

//#if VIRTUAL_TEXTURE_TARGET
//		FPrimitiveSceneData PrimitiveData = GetPrimitiveData(InstanceData.PrimitiveId);
//		const bool bMaterialInvalidates = (PrimitiveData.Flags & PRIMITIVE_SCENE_DATA_FLAG_EVALUATE_WORLD_POSITION_OFFSET) != 0u;
//		const bool bHasMoved = DynamicData.bHasMoved || bMaterialInvalidates;
//		const uint PageFlagMask = GetPageFlagMaskForRendering(InstanceData, NaniteView, bHasMoved);
//#endif

		// Depth clipping should only be disabled with orthographic projections
		//const bool bIsOrtho = IsOrthoProjection(NaniteView.ViewToClip);
		//const bool bNearClip = (NaniteView.Flags & NANITE_VIEW_FLAG_NEAR_CLIP) != 0u;
		//const bool bViewHZB = (NaniteView.Flags & NANITE_VIEW_FLAG_HZBTEST) != 0u;
		//const bool bUseViewRangeDistanceCull = (NaniteView.Flags & NANITE_VIEW_FLAG_DISTANCE_CULL) != 0u;

// #if DEBUG_FLAGS
// 		const bool bSkipBoxCullFrustum = (DebugFlags & NANITE_DEBUG_FLAG_DISABLE_CULL_FRUSTUM_BOX) != 0u;
// 		const bool bSkipBoxCullHZB = (DebugFlags & NANITE_DEBUG_FLAG_DISABLE_CULL_HZB_BOX) != 0u;
// #else
// 		const bool bSkipBoxCullFrustum = false;
// 		const bool bSkipBoxCullHZB = false;
// #endif

#if CULLING_PASS == CULLING_PASS_OCCLUSION_POST
		if ((CandidateNode.EnabledBitmask & (1u << ChildIndex)) == 0u)	// Need to check bEnabled because instance cull always writes full mask
		{
			bVisible = false;
		}
#endif

		StreamingPriority = 0.0f;

		[branch]
		if (bVisible)
		{
			float4 LODBounds = HierarchyNodeSlice.LODBounds;

			const float3 NodeBoxBoundsCenter = HierarchyNodeSlice.BoxBoundsCenter;
			const float3 NodeBoxBoundsExtent = HierarchyNodeSlice.BoxBoundsExtent;// *GetPrimitiveData(InstanceData.PrimitiveId).BoundsScale;

			// tix: ignore culling in this case
			//[branch]
			//if (bUseViewRangeDistanceCull)
			//{
			//	const float3 BoundsCenterTranslatedWorld = NodeBoxBoundsCenter;// mul(float4(NodeBoxBoundsCenter, 1.0f), DynamicData.LocalToTranslatedWorld).xyz;
			//	const float BoundsRadius = length(NodeBoxBoundsExtent);// length(NodeBoxBoundsExtent * InstanceData.NonUniformScale.xyz);

			//	bVisible = length2(BoundsCenterTranslatedWorld) <= Square(NaniteView.RangeBasedCullingDistance + BoundsRadius);
			//}

		#if CULLING_PASS == CULLING_PASS_OCCLUSION_POST
			if (bVisible && CandidateNode.Flags & NANITE_CULLING_FLAG_TEST_LOD)
		#endif
			{
				bVisible = ShouldVisitChildInternal(NaniteView, LODBounds, HierarchyNodeSlice.MinLODError, HierarchyNodeSlice.MaxParentLODError, StreamingPriority, DebugValue);//ShouldVisitChildInternal(NaniteView, InstanceData, DynamicData, LODBounds, HierarchyNodeSlice.MinLODError, HierarchyNodeSlice.MaxParentLODError, StreamingPriority);
			}

			// tix: ignore culling in this case
		//	[branch]
		//	if (bVisible)
		//	{
		//		FFrustumCullData Cull = BoxCullFrustum(NodeBoxBoundsCenter, NodeBoxBoundsExtent, DynamicData.LocalToTranslatedWorld, NaniteView.TranslatedWorldToClip, bIsOrtho, bNearClip, bSkipBoxCullFrustum );
		//		FScreenRect Rect = GetScreenRect( NaniteView.ViewRect, Cull, 4 );

		//		bVisible = Cull.bIsVisible && Rect.bOverlapsPixelCenter;

		//	//#if VIRTUAL_TEXTURE_TARGET
		//	//	[branch]
		//	//	if( bVisible )
		//	//	{
		//	//		uint4 RectPages = GetPageRect(Rect, NaniteView.TargetLayerIndex, NaniteView.TargetMipLevel);
		//	//		bVisible = OverlapsAnyValidPage( NaniteView.TargetLayerIndex, NaniteView.TargetMipLevel, RectPages, PageFlagMask);
		//	//	}
		//	//#endif

		//// VSM supports one-pass occlusion culling hijacking CULLING_PASS_NO_OCCLUSION (using only previous-frame with artifacts as a result), hence the || here
		//#if (CULLING_PASS == CULLING_PASS_NO_OCCLUSION && VIRTUAL_TEXTURE_TARGET) || CULLING_PASS == CULLING_PASS_OCCLUSION_MAIN
		//		TestPrevHZB(NaniteView, NodeBoxBoundsCenter, NodeBoxBoundsExtent, InstanceData, DynamicData, bNearClip, bViewHZB, bSkipBoxCullFrustum, bSkipBoxCullHZB, CULLING_PASS == CULLING_PASS_OCCLUSION_MAIN, bVisible, bWasOccluded);
		//	#if CULLING_PASS == CULLING_PASS_NO_OCCLUSION
		//		bVisible = bVisible && !bWasOccluded;
		//	#endif
		//#elif CULLING_PASS == CULLING_PASS_OCCLUSION_POST
		//		TestCurrentHZB(Cull, Rect, NaniteView, InstanceData, DynamicData, bSkipBoxCullHZB, bVisible, bWasOccluded);
		//#endif
		//	}
		}

#if CULLING_PASS == CULLING_PASS_OCCLUSION_MAIN
		[branch]
		if (bVisible && bWasOccluded && HierarchyNodeSlice.bLoaded)
		{
			InterlockedOr(GroupOccludedBitmask[LocalNodeIndex], 1u << ChildIndex);
		}
#endif

		bVisible = bVisible && !bWasOccluded;

		return bVisible;
	}

	void OnPreProcessNodeBatch(uint GroupIndex)
	{
#if CULLING_PASS == CULLING_PASS_OCCLUSION_MAIN
		if (GroupIndex < NANITE_MAX_BVH_NODES_PER_GROUP)
		{
			GroupOccludedBitmask[GroupIndex] = 0u;
		}
#endif
	}

	void OnPostNodeVisit(FHierarchyNodeSlice HierarchyNodeSlice)
	{
		if (bVisible && HierarchyNodeSlice.bLeaf)
		{
			//RequestPageRange(InstanceData.NaniteRuntimeResourceID, HierarchyNodeSlice.StartPageIndex, HierarchyNodeSlice.NumPages, NaniteView.StreamingPriorityCategory, StreamingPriority);
		}

#if CULLING_PASS == CULLING_PASS_OCCLUSION_MAIN
		// Store occluded node for POST occlusion culling
		if (ChildIndex == 0 && GroupOccludedBitmask[LocalNodeIndex])
		{
			uint OccludedNodesOffset;
			WaveInterlockedAddScalar_(QueueState[0].PassState[1].NodeWriteOffset, 1, OccludedNodesOffset);
			WaveInterlockedAddScalar(QueueState[0].PassState[1].NodeCount, 1);

			if (OccludedNodesOffset < DecodeInfo.MaxNodes)
			{
				FCandidateNode Node;
				Node.Flags = CandidateNode.Flags & ~NANITE_CULLING_FLAG_TEST_LOD;
				Node.ViewId = CandidateNode.ViewId;
				Node.InstanceId = CandidateNode.InstanceId;
				Node.NodeIndex = CandidateNode.NodeIndex;
				Node.EnabledBitmask = GroupOccludedBitmask[LocalNodeIndex];

				StoreCandidateNodeDataCoherent(MainAndPostNodesAndClusterBatches, OccludedNodesOffset, PackCandidateNode(Node), true);
			}
		}
#endif
	}

	void StoreChildNode(uint StoreIndex, FHierarchyNodeSlice HierarchyNodeSlice)
	{
		FCandidateNode Node;
		Node.Flags = CandidateNode.Flags | NANITE_CULLING_FLAG_TEST_LOD;
		Node.ViewId = CandidateNode.ViewId;
		Node.InstanceId = CandidateNode.InstanceId;
		Node.NodeIndex = HierarchyNodeSlice.ChildStartReference;
		Node.EnabledBitmask = NANITE_BVH_NODE_ENABLE_MASK;

		StoreCandidateNodeCoherent(MainAndPostNodesAndClusterBatches, StoreIndex, Node, bIsPostPass);
	}

	void StoreCluster(uint StoreIndex, FHierarchyNodeSlice HierarchyNodeSlice, uint ClusterIndex)
	{
		StoreIndex = bIsPostPass ? (DecodeInfo.MaxCandidateClusters - 1 - StoreIndex) : StoreIndex;

		FVisibleCluster CandidateCluster;
		CandidateCluster.Flags = CandidateNode.Flags | NANITE_CULLING_FLAG_TEST_LOD;
		CandidateCluster.ViewId = CandidateNode.ViewId;
		CandidateCluster.InstanceId = CandidateNode.InstanceId;
		CandidateCluster.PageIndex = HierarchyNodeSlice.ChildStartReference >> NANITE_MAX_CLUSTERS_PER_PAGE_BITS;
		CandidateCluster.ClusterIndex = ClusterIndex;

		uint4 PackedCluster = PackVisibleCluster(CandidateCluster, false);
		MainAndPostCandididateClusters.Store2(GetCandidateClusterOffset() + StoreIndex * GetCandidateClusterSize(), PackedCluster.xy);
	}

	uint4 LoadPackedCluster(uint CandidateIndex)
	{
		const uint LoadIndex = bIsPostPass ? (DecodeInfo.MaxCandidateClusters - 1 - CandidateIndex) : CandidateIndex;
		return uint4(MainAndPostCandididateClusters.Load2(GetCandidateClusterOffset() + LoadIndex * GetCandidateClusterSize()), 0u, 0u);
	}

	bool IsNodeDataReady(uint4 RawData)
	{
		return RawData.x != 0xFFFFFFFFu && RawData.y != 0xFFFFFFFFu && (!bIsPostPass || RawData.z != 0xFFFFFFFFu);
	}

	bool LoadCandidateNodeDataToGroup(uint NodeIndex, uint GroupIndex, uint GroupID, int LoopIndex, bool bCheckIfReady = true)
	{
		uint4 NodeData = LoadCandidateNodeDataCoherent(MainAndPostNodesAndClusterBatches, NodeIndex, bIsPostPass);

		bool bNodeReady = IsNodeDataReady(NodeData);
		if (!bCheckIfReady || bNodeReady)
		{
			SetGroupNodeData(GroupIndex, NodeData);
		}

		return bNodeReady;
	}

	void ClearCandidateNodeData(uint NodeIndex)
	{
		ClearCandidateNodeCoherent(MainAndPostNodesAndClusterBatches, NodeIndex, bIsPostPass);
	}

	void AddToClusterBatch(uint BatchIndex, uint Num)
	{
		AddToClusterBatchCoherent(MainAndPostNodesAndClusterBatches, BatchIndex, Num, bIsPostPass);
	}

	void ClearClusterBatch(uint BatchIndex)
	{
		ClearClusterBatchCoherent(MainAndPostNodesAndClusterBatches, BatchIndex, bIsPostPass);
	}

	uint LoadClusterBatch(uint BatchIndex)
	{
		return LoadClusterBatchCoherent(MainAndPostNodesAndClusterBatches, BatchIndex, bIsPostPass);
	}

	void ProcessCluster(uint4 PackedCluster, uint GroupIndex, uint LoopIndex)
	{
		FVisibleCluster VisibleCluster = UnpackVisibleCluster(PackedCluster, false);

		//FInstanceSceneData InstanceData = GetInstanceSceneData(VisibleCluster.InstanceId, false);
		FNaniteView NaniteView = GetNaniteView(VisibleCluster.ViewId);

#if CULLING_PASS == CULLING_PASS_OCCLUSION_POST
		if (VisibleCluster.Flags & NANITE_CULLING_FLAG_FROM_DISOCCLUDED_INSTANCE)
			NaniteView.LODScale *= DisocclusionLodScaleFactor;
#endif

		//FInstanceDynamicData DynamicData = CalculateInstanceDynamicData(NaniteView, InstanceData);

		const uint HWClusterCounterIndex = GetHWClusterCounterIndex(DecodeInfo.RenderFlags);

		// Near depth clipping should only be disabled with orthographic projections
		const bool bIsOrtho = IsOrthoProjection(NaniteView.ViewToClip);
		const bool bNearClip = (NaniteView.Flags & NANITE_VIEW_FLAG_NEAR_CLIP) != 0u;
		const bool bViewHZB = (NaniteView.Flags & NANITE_VIEW_FLAG_HZBTEST) != 0u;
		const bool bUseViewRangeDistanceCull = (NaniteView.Flags & NANITE_VIEW_FLAG_DISTANCE_CULL) != 0u;

#if DEBUG_FLAGS
		const bool bSkipBoxCullFrustum = (DebugFlags & NANITE_DEBUG_FLAG_DISABLE_CULL_FRUSTUM_BOX) != 0u;
		const bool bSkipBoxCullHZB = (DebugFlags & NANITE_DEBUG_FLAG_DISABLE_CULL_HZB_BOX) != 0u;
#else
		const bool bSkipBoxCullFrustum = false;
		const bool bSkipBoxCullHZB = false;
#endif

		FCluster Cluster = GetCluster(VisibleCluster.PageIndex, VisibleCluster.ClusterIndex);

		bool bWasOccluded = false;
		bool bUseHWRaster = false;
		bool bNeedsClipping = false;
		bool bVisible = true;

		//FPrimitiveSceneData PrimitiveData = GetPrimitiveData(InstanceData.PrimitiveId);
		const float3 ClusterBoxBoundsCenter = Cluster.BoxBoundsCenter;
		const float3 ClusterBoxBoundsExtent = Cluster.BoxBoundsExtent;// *PrimitiveData.BoundsScale;

//#if VIRTUAL_TEXTURE_TARGET
//		const bool bMaterialInvalidates = (PrimitiveData.Flags & PRIMITIVE_SCENE_DATA_FLAG_EVALUATE_WORLD_POSITION_OFFSET) != 0u;
//		const bool bHasMoved = DynamicData.bHasMoved || bMaterialInvalidates;
//
//		const uint PageFlagMask = GetPageFlagMaskForRendering(InstanceData, NaniteView, bHasMoved);
//		const bool bIsViewUncached = (NaniteView.Flags & NANITE_VIEW_FLAG_UNCACHED) != 0u;
//		const bool bShouldCacheAsStatic = ShouldCacheInstanceAsStatic(InstanceData, bIsViewUncached);
//
//		// Rect of overlapped virtual pages, is inclusive (as in zw is max, not max + 1)
//		uint4 RectPages = uint4(1U, 1U, 0U, 0U);
//#endif // VIRTUAL_TEXTURE_TARGET
		{
			// tix: ignore culling in this case
			//[branch]
			//if (bUseViewRangeDistanceCull)
			//{
			//	const float3 BoundsCenterTranslatedWorld = ClusterBoxBoundsCenter;// mul(float4(ClusterBoxBoundsCenter, 1.0f), DynamicData.LocalToTranslatedWorld).xyz;
			//	const float BoundsRadius = length(ClusterBoxBoundsExtent);// length(ClusterBoxBoundsExtent * InstanceData.NonUniformScale.xyz);

			//	bVisible = length2(BoundsCenterTranslatedWorld) <= Square(NaniteView.RangeBasedCullingDistance + BoundsRadius);
			//}

			[branch]
			if (bVisible)
			{
#if CULLING_PASS == CULLING_PASS_OCCLUSION_POST
				[branch]
				if ((VisibleCluster.Flags & NANITE_CULLING_FLAG_TEST_LOD) != 0)
#endif
				{
					bVisible = SmallEnoughToDraw(NaniteView, Cluster.LODBounds, Cluster.LODError, Cluster.EdgeLength, bUseHWRaster) || (Cluster.Flags & NANITE_CLUSTER_FLAG_LEAF);
						//SmallEnoughToDraw(NaniteView, InstanceData, DynamicData, Cluster.LODBounds, Cluster.LODError, Cluster.EdgeLength, bUseHWRaster) || (Cluster.Flags & NANITE_CLUSTER_FLAG_LEAF);
				}
#if CULLING_PASS == CULLING_PASS_OCCLUSION_POST
				else
				{
					bUseHWRaster = (VisibleCluster.Flags & NANITE_CULLING_FLAG_USE_HW) != 0;
				}
#endif
			}
		}

#if DEBUG_INFO
		//if (LoopIndex < MaxDebugInfo)
		//{
		//	DebugInfo[LoopIndex].bUseHWRaster[GroupIndex] = bUseHWRaster ? 1 : 0;
		//}
#endif
		
		// tix: ignore culling in this case
		//FFrustumCullData Cull;
		//FScreenRect Rect;

		uint ClusterOffsetHW = 0;
		uint ClusterOffsetSW = 0;

		[branch]
		if (bVisible && !bWasOccluded)
		{
			if (bUseHWRaster)
			{
				WaveInterlockedAddScalar_(VisibleClustersArgsSWHW[HWClusterCounterIndex], 1, ClusterOffsetHW);
			}
			else
			{
				WaveInterlockedAddScalar_(VisibleClustersArgsSWHW[0], 1, ClusterOffsetSW);
			}
		}
//
//#if DEBUG_INFO
//		if (LoopIndex < MaxDebugInfo)
//		{
//			DebugInfo[LoopIndex].ClusterOffsetHW[GroupIndex] = ClusterOffsetHW;
//		}
//#endif

		if (bVisible)
		{
			const uint2 TotalPrevDrawClusters = 0;// (DecodeInfo.RenderFlags & NANITE_RENDER_FLAG_HAS_PREV_DRAW_DATA) ? InTotalPrevDrawClusters[0] : 0;

			if (!bWasOccluded)
			{
				if (bUseHWRaster)
				{
					uint VisibleClusterOffsetHW = ClusterOffsetHW;
					VisibleClusterOffsetHW += TotalPrevDrawClusters.y;
#if CULLING_PASS == CULLING_PASS_OCCLUSION_POST
					VisibleClusterOffsetHW += OffsetClustersArgsSWHW[HWClusterCounterIndex];
#endif
					if (VisibleClusterOffsetHW < DecodeInfo.MaxVisibleClusters)
					{
						StoreVisibleCluster(OutVisibleClustersSWHW, (DecodeInfo.MaxVisibleClusters - 1) - VisibleClusterOffsetHW, VisibleCluster, 0);	// HW clusters written from the top
					}
				}
				else
				{
					uint VisibleClusterOffsetSW = ClusterOffsetSW;
					VisibleClusterOffsetSW += TotalPrevDrawClusters.x;
#if CULLING_PASS == CULLING_PASS_OCCLUSION_POST
					VisibleClusterOffsetSW += OffsetClustersArgsSWHW[0];
#endif
					if (VisibleClusterOffsetSW < DecodeInfo.MaxVisibleClusters)
					{
						StoreVisibleCluster(OutVisibleClustersSWHW, VisibleClusterOffsetSW, VisibleCluster, 0);	// SW clusters written from the bottom
					}
				}
			}
#if CULLING_PASS == CULLING_PASS_OCCLUSION_MAIN
			else
			{
				uint ClusterIndex = 0;
				WaveInterlockedAddScalar_(QueueState[0].TotalClusters, 1, ClusterIndex);
				if (ClusterIndex < DecodeInfo.MaxCandidateClusters)
				{
					uint OccludedClusterOffset = 0;
					WaveInterlockedAddScalar_(QueueState[0].PassState[1].ClusterWriteOffset, 1, OccludedClusterOffset);
					VisibleCluster.Flags = (bUseHWRaster ? NANITE_CULLING_FLAG_USE_HW : 0u);

					StoreCandidateClusterCoherent(MainAndPostCandididateClusters, (DecodeInfo.MaxCandidateClusters - 1) - OccludedClusterOffset, VisibleCluster);

					DeviceMemoryBarrier();
					const uint BatchIndex = OccludedClusterOffset / NANITE_PERSISTENT_CLUSTER_CULLING_GROUP_SIZE;
					AddToClusterBatchCoherent(MainAndPostNodesAndClusterBatches, BatchIndex, 1, true);
				}
			}
#endif
		}
	}
};

#ifndef NANITE_HIERARCHY_TRAVERSAL_TYPE
#	error "NANITE_HIERARCHY_TRAVERSAL_TYPE must be defined."
#endif

#ifndef CHECK_AND_TRIM_CLUSTER_COUNT
#	define CHECK_AND_TRIM_CLUSTER_COUNT 0
#endif


groupshared uint	GroupNumCandidateNodes;
groupshared uint	GroupCandidateNodesOffset;

groupshared int		GroupNodeCount;
groupshared uint	GroupNodeBatchStartIndex;
groupshared uint	GroupNodeMask;

groupshared uint	GroupClusterBatchStartIndex;
groupshared uint	GroupClusterBatchReadySize;

groupshared uint	GroupDebugNum;


//template<typename FNaniteTraversalCallback>
void ProcessNodeBatch(uint BatchSize, uint GroupIndex, uint QueueStateIndex, uint GroupId, uint LoopIndex)
{
	const uint LocalNodeIndex = (GroupIndex >> NANITE_MAX_BVH_NODE_FANOUT_BITS);
	const uint ChildIndex = GroupIndex & NANITE_MAX_BVH_NODE_FANOUT_MASK;
	const uint FetchIndex = min(LocalNodeIndex, BatchSize - 1);

	FNaniteTraversalClusterCullCallback TraversalCallback;
	TraversalCallback.Init(ChildIndex, LocalNodeIndex, FetchIndex, GroupId, LoopIndex);

	const FHierarchyNodeSlice HierarchyNodeSlice = GetHierarchyNodeSlice(TraversalCallback.GetHierarchyNodeIndex(), ChildIndex);


	bool bVisible = HierarchyNodeSlice.bEnabled;
	bool bLoaded = HierarchyNodeSlice.bLoaded;

	if (LocalNodeIndex >= BatchSize)
	{
		bVisible = false;
	}

	float4 ddd = -1.0;
	bVisible = TraversalCallback.ShouldVisitChild(HierarchyNodeSlice, bVisible, ddd);
#if DEBUG_INFO
		if (LoopIndex < MaxDebugInfo && GroupIndex < 4)
		{
			DebugInfo[LoopIndex].Nd[GroupIndex].LODBounds = HierarchyNodeSlice.LODBounds;
			DebugInfo[LoopIndex].Nd[GroupIndex].MinE = HierarchyNodeSlice.MinLODError;
			DebugInfo[LoopIndex].Nd[GroupIndex].MaxE = HierarchyNodeSlice.MaxParentLODError;
			DebugInfo[LoopIndex].Nd[GroupIndex].ProjectedEdgeScale = ddd.x;
			DebugInfo[LoopIndex].Nd[GroupIndex].NodeCullThres = ddd.y;
		}
#endif

	uint CandidateNodesOffset = 0;

	[branch]
		if (bVisible && !HierarchyNodeSlice.bLeaf)
		{
			WaveInterlockedAddScalar_(GroupNumCandidateNodes, 1, CandidateNodesOffset);
		}

	GroupMemoryBarrierWithGroupSync();

#if DEBUG_INFO
	if (LoopIndex < MaxDebugInfo && GroupIndex == 0)
	{
		DebugInfo[LoopIndex].GroupNumCandidateNodes = GroupNumCandidateNodes;
	}
#endif

	if (GroupIndex == 0)
	{
		InterlockedAdd(QueueState[0].PassState[QueueStateIndex].NodeWriteOffset, GroupNumCandidateNodes, GroupCandidateNodesOffset);
		InterlockedAdd(QueueState[0].PassState[QueueStateIndex].NodeCount, (int)GroupNumCandidateNodes);	// NodeCount needs to be conservative, so child count is added before the actual children.
	}

	AllMemoryBarrierWithGroupSync();

	// GPU might not be filled, so latency is important here. Kick new jobs as soon as possible.
	bool bOutputChild = bVisible && bLoaded;
	if (bOutputChild && !HierarchyNodeSlice.bLeaf)
	{
		CandidateNodesOffset += GroupCandidateNodesOffset;

		if (CandidateNodesOffset < DecodeInfo.MaxNodes)
		{
			TraversalCallback.StoreChildNode(CandidateNodesOffset, HierarchyNodeSlice);
		}
	}
	DeviceMemoryBarrierWithGroupSync();
	
	uint NumToAdd = WaveActiveCountBits( HierarchyNodeSlice.bLeaf && LocalNodeIndex < BatchSize );
	if (WaveIsFirstLane())
		InterlockedAdd( GroupDebugNum, NumToAdd );
	GroupMemoryBarrierWithGroupSync();
#if DEBUG_INFO
	if (LoopIndex < MaxDebugInfo && GroupIndex == 0)
	{
		DebugInfo[LoopIndex].NumLeaves = GroupDebugNum;
	}
#endif


	// Continue with remaining independent work
	if (bOutputChild && HierarchyNodeSlice.bLeaf)
	{
		uint NumClusters = HierarchyNodeSlice.NumChildren;

#if CHECK_AND_TRIM_CLUSTER_COUNT
		uint ClusterIndex = 0;
		WaveInterlockedAdd_(QueueState[0].TotalClusters, NumClusters, ClusterIndex);

		// Trim any clusters above MaxCandidateClusters
		const uint ClusterIndexEnd = min(ClusterIndex + NumClusters, DecodeInfo.MaxCandidateClusters);
		NumClusters = (uint)max((int)ClusterIndexEnd - (int)ClusterIndex, 0);
#endif

		uint CandidateClustersOffset = 0;
		WaveInterlockedAdd_(QueueState[0].PassState[QueueStateIndex].ClusterWriteOffset, NumClusters, CandidateClustersOffset);

		const uint BaseClusterIndex = HierarchyNodeSlice.ChildStartReference & NANITE_MAX_CLUSTERS_PER_PAGE_MASK;
		const uint StartIndex = CandidateClustersOffset;
		const uint EndIndex = min(CandidateClustersOffset + NumClusters, DecodeInfo.MaxCandidateClusters);

		for (uint Index = StartIndex; Index < EndIndex; Index++)
		{
			TraversalCallback.StoreCluster(Index, HierarchyNodeSlice, BaseClusterIndex + (Index - StartIndex));
		}

		DeviceMemoryBarrier();

		// Once the cluster indices have been committed to memory, we can update the cluster counters of the overlapping cluster batches.
		for (uint Index1 = StartIndex; Index1 < EndIndex;)
		{
			const uint BatchIndex = Index1 / NANITE_PERSISTENT_CLUSTER_CULLING_GROUP_SIZE;
			const uint NextIndex = (Index1 & ~(NANITE_PERSISTENT_CLUSTER_CULLING_GROUP_SIZE - 1u)) + NANITE_PERSISTENT_CLUSTER_CULLING_GROUP_SIZE;
			const uint MaxIndex = min(NextIndex, EndIndex);
			const uint Num = MaxIndex - Index1;
			TraversalCallback.AddToClusterBatch(BatchIndex, Num);
			Index1 = NextIndex;
		}
	}

	DeviceMemoryBarrierWithGroupSync();
	if (GroupIndex == 0)
	{
		// Done writing clusters/nodes for current pass. Subtract us from NodeCount.
		InterlockedAdd(QueueState[0].PassState[QueueStateIndex].NodeCount, -(int)BatchSize);
	}

	TraversalCallback.OnPostNodeVisit(HierarchyNodeSlice);
}

//#endif

//#if NANITE_HIERARCHY_TRAVERSAL_TYPE == NANITE_CULLING_TYPE_PERSISTENT_NODES_AND_CLUSTERS || NANITE_HIERARCHY_TRAVERSAL_TYPE == NANITE_CULLING_TYPE_CLUSTERS

//template<typename FNaniteTraversalCallback>
void ProcessClusterBatch(uint BatchStartIndex, uint BatchSize, uint GroupIndex, uint LoopIndex)
{
	FNaniteTraversalClusterCullCallback TraversalCallback;

	if (GroupIndex < BatchSize)
	{
		const uint CandidateIndex = BatchStartIndex * NANITE_PERSISTENT_CLUSTER_CULLING_GROUP_SIZE + GroupIndex;
		const uint4 PackedCluster = TraversalCallback.LoadPackedCluster(CandidateIndex);

		TraversalCallback.ProcessCluster(PackedCluster, GroupIndex, LoopIndex);
	}

	// Clear batch so the buffer is cleared for next pass.
	TraversalCallback.ClearClusterBatch(BatchStartIndex);
}


void PersistentNodeAndClusterCull(uint GroupID, uint GroupIndex, uint QueueStateIndex)
{
	FNaniteTraversalClusterCullCallback TraversalCallback;

	bool bProcessNodes = true;						// Should we still try to consume nodes?
	uint NodeBatchReadyOffset = NANITE_MAX_BVH_NODES_PER_GROUP;
	uint NodeBatchStartIndex = 0;
	uint ClusterBatchStartIndex = 0xFFFFFFFFu;

	int LoopIndex = -1;
#if !(DEBUG_MODE)
	while (true)
#else
	[loop]
	for (int __i = 0; __i < MaxDebugInfo; __i ++)
#endif
	{
		LoopIndex++;
		GroupMemoryBarrierWithGroupSync();	// Make sure we are done reading from group shared
		if (GroupIndex == 0)
		{
			GroupNumCandidateNodes = 0;
			GroupNodeMask = 0;
		}

		TraversalCallback.OnPreProcessNodeBatch(GroupIndex);

		GroupMemoryBarrierWithGroupSync();

		uint NodeReadyMask = 0;
		if (bProcessNodes)	// Try grabbing and processing nodes if they could be available.
		{
			if (NodeBatchReadyOffset == NANITE_MAX_BVH_NODES_PER_GROUP)
			{
				// No more data in current batch. Grab a new batch.
				if (GroupIndex == 0)
				{
					InterlockedAdd(QueueState[0].PassState[QueueStateIndex].NodeReadOffset, NANITE_MAX_BVH_NODES_PER_GROUP, GroupNodeBatchStartIndex);
#if DEBUG_INFO
					if (LoopIndex < MaxDebugInfo)
					{
						DebugInfo[LoopIndex].GroupNodeBatchStartIndex = GroupNodeBatchStartIndex;
					}
#endif
				}
				GroupMemoryBarrierWithGroupSync();

				NodeBatchReadyOffset = 0;
				NodeBatchStartIndex = GroupNodeBatchStartIndex;
				if (NodeBatchStartIndex >= DecodeInfo.MaxNodes)
				{
					// The node range is out of bounds and so will any future range be.
					bProcessNodes = false;
					continue;
				}
			}

			// Check which nodes in the range have been completely written and are ready for processing.
			const uint NodeIndex = NodeBatchStartIndex + NodeBatchReadyOffset + GroupIndex;
			bool bNodeReady = (NodeBatchReadyOffset + GroupIndex < NANITE_MAX_BVH_NODES_PER_GROUP);
			if (bNodeReady)
			{
				bNodeReady = TraversalCallback.LoadCandidateNodeDataToGroup(NodeIndex, GroupIndex, GroupID, LoopIndex);
			}

			if (bNodeReady)
			{
				InterlockedOr(GroupNodeMask, 1u << GroupIndex);
			}
			AllMemoryBarrierWithGroupSync();
			NodeReadyMask = GroupNodeMask;

			// Process nodes if at least the first one is ready.
			if (NodeReadyMask & 1u)
			{
				uint BatchSize = firstbitlow(~NodeReadyMask);
#if DEBUG_INFO
				if (LoopIndex < MaxDebugInfo && GroupIndex == 0)
					DebugInfo[LoopIndex].BatchSize = BatchSize;
#endif
				ProcessNodeBatch(BatchSize, GroupIndex, QueueStateIndex, GroupID, LoopIndex);
				if (GroupIndex < BatchSize)
				{
					// Clear processed element so we leave the buffer cleared for next pass.
					TraversalCallback.ClearCandidateNodeData(NodeIndex);
				}
#if DEBUG_INFO
				if (LoopIndex < MaxDebugInfo && GroupIndex == 0)
				{
					DebugInfo[LoopIndex].NodeCount = QueueState[0].PassState[QueueStateIndex].NodeCount;
				}
#endif

				NodeBatchReadyOffset += BatchSize;
				continue;
			}
		}

		// No nodes were ready. Process clusters instead.

		// Grab a range of clusters, if we don't already have one.
		if (ClusterBatchStartIndex == 0xFFFFFFFFu)
		{
			if (GroupIndex == 0)
			{
				InterlockedAdd(QueueState[0].PassState[QueueStateIndex].ClusterBatchReadOffset, 1, GroupClusterBatchStartIndex);
			}
			GroupMemoryBarrierWithGroupSync();
			ClusterBatchStartIndex = GroupClusterBatchStartIndex;
		}

		if (!bProcessNodes && GroupClusterBatchStartIndex >= GetMaxClusterBatches())
			break;	// Has to be break instead of return to make FXC happy.

		if (GroupIndex == 0)
		{
			GroupNodeCount = QueueState[0].PassState[QueueStateIndex].NodeCount;
			GroupClusterBatchReadySize = TraversalCallback.LoadClusterBatch(ClusterBatchStartIndex);
		}
		GroupMemoryBarrierWithGroupSync();
#if DEBUG_INFO
		if (LoopIndex < MaxDebugInfo && GroupIndex == 0)
		{
			DebugInfo[LoopIndex].ClusterBatchStartIndex = ClusterBatchStartIndex;
			DebugInfo[LoopIndex].GroupClusterBatchReadySize = GroupClusterBatchReadySize;
		}
#endif

		uint ClusterBatchReadySize = GroupClusterBatchReadySize;
		if (!bProcessNodes && ClusterBatchReadySize == 0)	// No more clusters to process and no nodes are available to 
			break;	// Has to be break instead of return to make FXC happy.

#if DEBUG_INFO
		if (LoopIndex < MaxDebugInfo && GroupIndex == 0)
		{
			DebugInfo[LoopIndex].Padding1 = LoopIndex;
		}
#endif
		if ((bProcessNodes && ClusterBatchReadySize == NANITE_PERSISTENT_CLUSTER_CULLING_GROUP_SIZE) || (!bProcessNodes && ClusterBatchReadySize > 0))
		{
			ProcessClusterBatch(ClusterBatchStartIndex, ClusterBatchReadySize, GroupIndex, LoopIndex);
			ClusterBatchStartIndex = 0xFFFFFFFFu;
		}

		if (bProcessNodes && GroupNodeCount == 0)
		{
			bProcessNodes = false;
		}
	}
}

// refine RS from render doc results
// SRVs: [x] = ignored by tix
// [x] #0 Table[0] 0 0: GPUScenePrimitiveSceneData GPUScene.PrimitiveData StructuredBuffer[12288] 196608 0 0 12288 float4
// [x] #0 Table[1] 0 1: GPUSceneInstanceSceneData GPUScene.InstanceSceneData StructuredBuffer[4096] 65536 0 0 4096 float4
// [x] #0 Table[2] 0 2: GPUSceneInstancePayloadData GPUScene.InstancePayloadData StructuredBuffer[4096] 65536 0 0 4096 float4
// #0 Table[3] 0 3: ClusterPageData Nanite.StreamingManager.ClusterPageData ByteAddressBuffer 603979776 0 0 0 byte
// #0 Table[4] 0 4: HierarchyBuffer Nanite.StreamingManager.Hierarchy ByteAddressBuffer 65536 0 0 0 byte
// [x] #0 Table[5] 0 5: InViews Nanite.Views StructuredBuffer[73] 65408 0 0 73 struct hostlayout.struct.FPackedNaniteView
// [x] #0 Table[6] 0 6: HZBTexture HZBFurthest Texture 2D 1024 512 1 1 R16_FLOAT
// [x] #0 Table[7] 0 7: InTotalPrevDrawClusters Resource Allocator Underlying Buffer StructuredBuffer[4194304] 33554432 0 0 4194304 int2
// UAVs
// #2 Table[0] 0 0: QueueState Nanite.QueueState RWStructuredBuffer[1] 52 0 0 1 struct FQueueState
// #2 Table[1] 0 1: MainAndPostNodesAndClusterBatches Nanite.MainAndPostNodesAndClusterBatchesBuffer RWByteAddressBuffer 44040192 0 0 0 byte
// #2 Table[2] 0 2: MainAndPostCandididateClusters Nanite.MainAndPostCandididateClustersBuffer RWByteAddressBuffer 134217728 0 0 0 byte
// #2 Table[3] 0 3: OutStreamingRequests Nanite.StreamingRequests RWStructuredBuffer[262144] 3145728 0 0 262144 struct FStreamingRequest
// #2 Table[4] 0 4: OutVisibleClustersSWHW Nanite.VisibleClustersSWHW RWByteAddressBuffer 50331648 0 0 0 byte
// #2 Table[5] 0 5: VisibleClustersArgsSWHW Nanite.RasterizerBinIndirectArgs RWBuffer 65536 0 0 0 R32_UINT



#define PersistentCullRS \
	"RootConstants(num32BitConstants=10, b0)," \
    "DescriptorTable(SRV(t0, numDescriptors=4), UAV(u0, numDescriptors=7))" 

[RootSignature(PersistentCullRS)]
[numthreads(NANITE_PERSISTENT_CLUSTER_CULLING_GROUP_SIZE, 1, 1)]
void NodeAndClusterCull(uint GroupID : SV_GroupID, uint GroupIndex : SV_GroupIndex)
{
	PersistentNodeAndClusterCull(GroupID, GroupIndex, QueueStateIndex);
}
