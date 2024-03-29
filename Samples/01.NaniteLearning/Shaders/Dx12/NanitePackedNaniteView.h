// Copyright Epic Games, Inc. All Rights Reserved.

//#pragma once

struct FPackedNaniteView
{
	//float4x4	SVPositionToTranslatedWorld;
	//float4x4	ViewToTranslatedWorld;

	//float4x4	TranslatedWorldToView;
	float4x4	TranslatedWorldToClip;
	//float4x4	TranslatedWorldToSubpixelClip;	// Divide by w to get to absolute subpixel coordinates
	float4x4	ViewToClip;
	//float4x4	ClipToRelativeWorld;
	
	//float4x4	PrevTranslatedWorldToView;
	//float4x4	PrevTranslatedWorldToClip;
	//float4x4	PrevViewToClip;
	//float4x4	PrevClipToRelativeWorld;

	int4		ViewRect;
	float4		ViewSizeAndInvSize;
	//float4		ClipSpaceScaleOffset;
	//float4		PreViewTranslation;
	//float4		PrevPreViewTranslation;
	float4		WorldCameraOrigin;
	float4		ViewForwardAndNearPlane;

	//float3		ViewTilePosition;
	//float		RangeBasedCullingDistance;

	//float3		MatrixTilePosition;
	
	float2		LODScales;
	//float		MinBoundsRadiusSq;
	uint		StreamingPriorityCategory_AndFlags;
	uint		Padding1;

	//int4		TargetLayerIdX_AndMipLevelY_AndNumMipLevelsZ;

	//int4		HZBTestViewRect;
};
