/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "NaniteView.h"

const float GNaniteMaxPixelsPerEdge = 1.0f;
const float GNaniteMinPixelsPerEdgeHW = 32.0f;

void FPackedView::UpdateLODScales(float ViewHeight)
{
	const float ViewToPixels = 0.5f * ViewToClip(1, 1) * ViewHeight;

	const float LODScale = ViewToPixels / GNaniteMaxPixelsPerEdge;
	const float LODScaleHW = ViewToPixels / GNaniteMinPixelsPerEdgeHW;

	LODScales = FFloat2(LODScale, LODScaleHW);
}

//FMat4 FPackedView::CalcTranslatedWorldToSubpixelClip(const FMat4& TranslatedWorldToClip, const FRecti& ViewRect)
//{
//	const FFloat2 SubpixelScale = FFloat2(0.5f * ViewRect.Width() * NANITE_SUBPIXEL_SAMPLES,
//		-0.5f * ViewRect.Height() * NANITE_SUBPIXEL_SAMPLES);
//
//	const FFloat2 SubpixelOffset = FFloat2((0.5f * ViewRect.Width() + ViewRect.Min.X) * NANITE_SUBPIXEL_SAMPLES,
//		(0.5f * ViewRect.Height() + ViewRect.Min.Y) * NANITE_SUBPIXEL_SAMPLES);
//
//	return TranslatedWorldToClip * FScaleMatrix44f(FFloat3(SubpixelScale, 1.0f)) * FTranslationMatrix44f(FFloat3(SubpixelOffset, 0.0f));
//}


FPackedView CreatePackedView(const FPackedViewParams& Params)
{
	// NOTE: There is some overlap with the logic - and this should stay consistent with - FSceneView::SetupViewRectUniformBufferParameters
	// Longer term it would be great to refactor a common place for both of this logic, but currently FSceneView has a lot of heavy-weight
	// stuff in it beyond the relevant parameters to SetupViewRectUniformBufferParameters (and Nanite has a few of its own parameters too).

	//const FRelativeViewMatrices RelativeMatrices = FRelativeViewMatrices::Create(Params.ViewMatrices, Params.PrevViewMatrices);
	//const FLargeWorldRenderPosition AbsoluteViewOrigin(Params.ViewMatrices.GetViewOrigin());
	//const FFloat3 ViewTileOffset = AbsoluteViewOrigin.GetTileOffset();

	const FRecti& ViewRect = Params.ViewRect;
	const FFloat4 ViewSizeAndInvSize(float(ViewRect.GetWidth()), float(ViewRect.GetHeight()), 1.0f / float(ViewRect.GetWidth()), 1.0f / float(ViewRect.GetHeight()));

	FPackedView PackedView;
	//PackedView.TranslatedWorldToView = FMat4(Params.ViewMatrices.GetOverriddenTranslatedViewMatrix());	// LWC_TODO: Precision loss? (and below)
	PackedView.TranslatedWorldToClip = FMat4((Params.ViewInfo.MatProj * Params.ViewInfo.MatView));
		//(VPInfo.MatProj * VPInfo.MatView).GetTransposed();
		//Params.ViewMatrices.GetTranslatedViewProjectionMatrix());
	//PackedView.TranslatedWorldToSubpixelClip = FPackedView::CalcTranslatedWorldToSubpixelClip(PackedView.TranslatedWorldToClip, Params.ViewRect);
	PackedView.ViewToClip = Params.ViewInfo.MatProj;// RelativeMatrices.ViewToClip;
	//PackedView.ClipToRelativeWorld = RelativeMatrices.ClipToRelativeWorld;
	//PackedView.PreViewTranslation = FFloat4(FFloat3(Params.ViewMatrices.GetPreViewTranslation() + ViewTileOffset)); // LWC_TODO: precision loss
	//PackedView.WorldCameraOrigin = FFloat4(FFloat3(Params.ViewMatrices.GetViewOrigin() - ViewTileOffset), 0.0f);
	FMat4 ViewTrans;
	ViewTrans.SetTranslation(Params.ViewInfo.CamPos);
	FMat4 TranslatedViewMat = ViewTrans * Params.ViewInfo.MatView;

	const FMat4& MatProj = Params.ViewInfo.MatProj;
	float NearPlane = (MatProj(3, 3) - MatProj(3, 2)) / (MatProj(2, 2) - MatProj(2, 3));
	
	PackedView.ViewForwardAndNearPlane = FFloat4(
		Params.ViewInfo.CamDir.X,
		Params.ViewInfo.CamDir.Y,
		Params.ViewInfo.CamDir.Z,
		NearPlane);
	//PackedView.ViewTilePosition = AbsoluteViewOrigin.GetTile();
	//PackedView.RangeBasedCullingDistance = Params.RangeBasedCullingDistance;
	//PackedView.MatrixTilePosition = RelativeMatrices.TilePosition;
	PackedView.Padding1 = 0u;

	//PackedView.PrevTranslatedWorldToView = FMat4(Params.PrevViewMatrices.GetOverriddenTranslatedViewMatrix()); // LWC_TODO: Precision loss? (and below)
	//PackedView.PrevTranslatedWorldToClip = FMat4(Params.PrevViewMatrices.GetTranslatedViewProjectionMatrix());
	//PackedView.PrevViewToClip = FMat4(Params.PrevViewMatrices.GetProjectionMatrix());
	//PackedView.PrevClipToRelativeWorld = RelativeMatrices.PrevClipToRelativeWorld;
	//PackedView.PrevPreViewTranslation = FFloat4(FFloat3(Params.PrevViewMatrices.GetPreViewTranslation() + ViewTileOffset)); // LWC_TODO: precision loss


	PackedView.ViewRect = FInt4(ViewRect.Left, ViewRect.Upper, ViewRect.Right, ViewRect.Lower);
	PackedView.ViewSizeAndInvSize = ViewSizeAndInvSize;

	// Transform clip from full screen to viewport.
	FFloat2 RcpRasterContextSize = FFloat2(1.0f / Params.RasterContextSize.X, 1.0f / Params.RasterContextSize.Y);
	//PackedView.ClipSpaceScaleOffset = FFloat4(ViewSizeAndInvSize.X * RcpRasterContextSize.X,
	//	ViewSizeAndInvSize.Y * RcpRasterContextSize.Y,
	//	(ViewSizeAndInvSize.X + 2.0f * ViewRect.Left) * RcpRasterContextSize.X - 1.0f,
	//	-(ViewSizeAndInvSize.Y + 2.0f * ViewRect.Upper) * RcpRasterContextSize.Y + 1.0f);

	const float Mx = 2.0f * ViewSizeAndInvSize.Z;
	const float My = -2.0f * ViewSizeAndInvSize.W;
	const float Ax = -1.0f - 2.0f * ViewRect.Left * ViewSizeAndInvSize.Z;
	const float Ay = 1.0f + 2.0f * ViewRect.Upper * ViewSizeAndInvSize.W;

	//PackedView.SVPositionToTranslatedWorld = FMat4(			// LWC_TODO: Precision loss? (and below)
	//	FMatrix(FPlane(Mx, 0, 0, 0),
	//		FPlane(0, My, 0, 0),
	//		FPlane(0, 0, 1, 0),
	//		FPlane(Ax, Ay, 0, 1)) * Params.ViewMatrices.GetInvTranslatedViewProjectionMatrix());
	//PackedView.ViewToTranslatedWorld = FMat4(Params.ViewMatrices.GetOverriddenInvTranslatedViewMatrix());

	TI_ASSERT(Params.StreamingPriorityCategory <= NANITE_STREAMING_PRIORITY_CATEGORY_MASK);
	PackedView.StreamingPriorityCategory_AndFlags = (Params.Flags << NANITE_NUM_STREAMING_PRIORITY_CATEGORY_BITS) | Params.StreamingPriorityCategory;
	//PackedView.MinBoundsRadiusSq = Params.MinBoundsRadius * Params.MinBoundsRadius;
	PackedView.UpdateLODScales(ViewSizeAndInvSize.Y);

	PackedView.LODScales.X *= Params.LODScaleFactor;

	//PackedView.TargetLayerIdX_AndMipLevelY_AndNumMipLevelsZ.X = Params.TargetLayerIndex;
	//PackedView.TargetLayerIdX_AndMipLevelY_AndNumMipLevelsZ.Y = Params.TargetMipLevel;
	//PackedView.TargetLayerIdX_AndMipLevelY_AndNumMipLevelsZ.Z = Params.TargetMipCount;
	//PackedView.TargetLayerIdX_AndMipLevelY_AndNumMipLevelsZ.W = Params.PrevTargetLayerIndex;

	//PackedView.HZBTestViewRect = FInt4(Params.HZBTestViewRect.Min.X, Params.HZBTestViewRect.Min.Y, Params.HZBTestViewRect.Max.X, Params.HZBTestViewRect.Max.Y);

	return PackedView;

}

FPackedView CreatePackedViewFromViewInfo
(
	const FViewProjectionInfo& ViewInfo,
	FInt2 RasterContextSize,
	uint32 Flags,
	uint32 StreamingPriorityCategory,
	float MinBoundsRadius,
	float LODScaleFactor
)
{
	FPackedViewParams Params;
	Params.ViewInfo = ViewInfo;
	Params.ViewRect = FRecti(0, 0, RasterContextSize.X, RasterContextSize.Y);
	Params.RasterContextSize = RasterContextSize;
	Params.Flags = Flags;
	Params.StreamingPriorityCategory = StreamingPriorityCategory;
	Params.MinBoundsRadius = MinBoundsRadius;
	Params.LODScaleFactor = LODScaleFactor;
	// Note - it is incorrect to use ViewRect as it is in a different space, but keeping this for backward compatibility reasons with other callers
	//Params.HZBTestViewRect = InHZBTestViewRect ? *InHZBTestViewRect : View.PrevViewInfo.ViewRect;
	return CreatePackedView(Params);
}