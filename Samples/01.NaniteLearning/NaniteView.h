/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

struct FPackedView
{
	//FMat4 SVPositionToTranslatedWorld;
	//FMat4 ViewToTranslatedWorld;
		  
	//FMat4 TranslatedWorldToView;
	//FMat4 TranslatedWorldToClip;
	//FMat4 TranslatedWorldToSubpixelClip;
	FMat4 ViewToClip;
	//FMat4 ClipToRelativeWorld;
		  
	//FMat4 PrevTranslatedWorldToView;
	//FMat4 PrevTranslatedWorldToClip;
	//FMat4 PrevViewToClip;
	//FMat4 PrevClipToRelativeWorld;

	//FInt4 ViewRect;
	//FFloat4 ViewSizeAndInvSize;
	//FFloat4 ClipSpaceScaleOffset;
	//FFloat4 PreViewTranslation;
	//FFloat4 PrevPreViewTranslation;
	//FFloat4 WorldCameraOrigin;
	FFloat4 ViewForwardAndNearPlane;

	//FFloat3 ViewTilePosition;
	//float RangeBasedCullingDistance;

	//FFloat3 MatrixTilePosition;
	//uint32 Padding1;

	FFloat2 LODScales;
	uint32 Padding1;
	uint32 Padding2;
	//float MinBoundsRadiusSq;
	//uint32 StreamingPriorityCategory_AndFlags;

	//FInt4 TargetLayerIdX_AndMipLevelY_AndNumMipLevelsZ;

	//FInt4 HZBTestViewRect;	// In full resolution

	/**
	 * Calculates the LOD scales assuming view size and projection is already set up.
	 * TODO: perhaps more elegant/robust if this happened at construction time, and input was a non-packed NaniteView.
	 * Note: depends on the global 'GNaniteMaxPixelsPerEdge'.
	 */
	void UpdateLODScales(float ViewHeight);


	/**
	 * Helper to compute the derived subpixel transform.
	 */
	static FMat4 CalcTranslatedWorldToSubpixelClip(const FMat4& TranslatedWorldToClip, const FRecti& ViewRect);
};

struct FPackedViewParams
{
	//FViewMatrices ViewMatrices;
	//FViewMatrices PrevViewMatrices;
	FViewProjectionInfo ViewInfo;
	FRecti ViewRect;
	FInt2 RasterContextSize;
	uint32 StreamingPriorityCategory = 0;
	float MinBoundsRadius = 0.0f;
	float LODScaleFactor = 1.0f;
	uint32 Flags = NANITE_VIEW_FLAG_NEAR_CLIP;

	int32 TargetLayerIndex = 0;
	int32 PrevTargetLayerIndex = -1;
	int32 TargetMipLevel = 0;
	int32 TargetMipCount = 1;

	float RangeBasedCullingDistance = 0.0f; // not used unless the flag NANITE_VIEW_FLAG_DISTANCE_CULL is set

	FRecti HZBTestViewRect = { 0, 0, 0, 0 };
};

FPackedView CreatePackedView(const FPackedViewParams& Params);

// Convenience function to pull relevant packed view parameters out of a FViewInfo
FPackedView CreatePackedViewFromViewInfo(
	const FViewProjectionInfo& ViewInfo,
	FInt2 RasterContextSize,
	uint32 Flags,
	uint32 StreamingPriorityCategory = 0,
	float MinBoundsRadius = 0.0f,
	float LODScaleFactor = 1.0f
);
