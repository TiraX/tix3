// Copyright Epic Games, Inc. All Rights Reserved.

//#pragma once

//#include "/Engine/Public/WaveBroadcastIntrinsics.ush"
#include "NaniteDataDecode.h"

struct FNaniteAttributeData
{
	/** Interpolated vertex color, in linear color space. */
	half4 VertexColor;
	half4 VertexColor_DDX;
	half4 VertexColor_DDY;

	/** Orthonormal rotation-only transform from tangent space to world space. */
	half3x3 TangentToWorld;

	float2 TexCoords[NANITE_MAX_UVS];
	float2 TexCoords_DDX[NANITE_MAX_UVS];
	float2 TexCoords_DDY[NANITE_MAX_UVS];

	half UnMirrored;
};

struct FNaniteRawAttributeData
{
	float3 TangentZ;
	float4 Color;
	float2 TexCoords[NANITE_MAX_UVS];
};

#if COMPILER_SUPPORTS_WAVE_PERMUTE
FNaniteRawAttributeData WaveReadLaneAtVarying(FNaniteRawAttributeData In, uint SrcIndex)
{
	FNaniteRawAttributeData Out;
	Out.TangentZ = WaveReadLaneAtVarying(In.TangentZ, SrcIndex);
	Out.Color = WaveReadLaneAtVarying(In.Color, SrcIndex);
	[unroll]
	for (uint i = 0; i < NANITE_MAX_UVS; ++i)
	{
		Out.TexCoords[i] = WaveReadLaneAtVarying(In.TexCoords[i], SrcIndex);
	}
	return Out;
}
#endif

#define SIZEOF_UV_RANGE	32
struct FUVRange
{
	int2	Min;
	uint2	GapStart;
	uint2	GapLength;
	int		Precision;
};


FUVRange UnpackUVRange(uint4 Data[2])
{
	FUVRange Range;
	Range.Min = (int2)Data[0].xy;
	Range.GapStart = Data[0].zw;
	Range.GapLength = Data[1].xy;
	Range.Precision = Data[1].z;
	return Range;
}

FUVRange GetUVRange(ByteAddressBuffer InputBuffer, uint StartOffset, uint Index)
{
	uint Offset = StartOffset + Index * SIZEOF_UV_RANGE;
	uint4 Data[2];
	Data[0] = InputBuffer.Load4(Offset);
	Data[1] = InputBuffer.Load4(Offset + 16);
	return UnpackUVRange(Data);
}

FUVRange GetUVRange(RWByteAddressBuffer InputBuffer, uint StartOffset, uint Index)
{
	uint Offset = StartOffset + Index * SIZEOF_UV_RANGE;
	uint4 Data[2];
	Data[0] = InputBuffer.Load4(Offset);
	Data[1] = InputBuffer.Load4(Offset + 16);
	return UnpackUVRange(Data);
}

float2 UnpackTexCoord(uint2 Packed, FUVRange UVRange)
{
	uint2 T = Packed + select(Packed > UVRange.GapStart, UVRange.GapLength, 0u);

	const float Scale = asfloat(asint(1.0f) - (UVRange.Precision << 23));
	return float2((int2)T + UVRange.Min) * Scale;
}

float3 UnpackNormal(uint Packed, uint Bits)
{
	uint Mask = BitFieldMaskU32(Bits, 0);
	float2 F = uint2(BitFieldExtractU32(Packed, Bits, 0), BitFieldExtractU32(Packed, Bits, Bits)) * (2.0f / Mask) - 1.0f;
	float3 N = float3(F.xy, 1.0 - abs(F.x) - abs(F.y));
	float T = saturate(-N.z);
	N.xy += select(N.xy >= 0.0, -T, T);
	return normalize(N);
}

struct FBarycentrics
{
	float3 UVW;
	float3 UVW_dx;
	float3 UVW_dy;
};

/** Calculates perspective correct barycentric coordinates and partial derivatives using screen derivatives. */
FBarycentrics CalculateTriangleBarycentrics(float2 Subpixel, float4 PointSubpixelClip0, float4 PointSubpixelClip1, float4 PointSubpixelClip2)
{
	FBarycentrics Result;

	const float3 RcpW = rcp(float3(PointSubpixelClip0.w, PointSubpixelClip1.w, PointSubpixelClip2.w));
	const float3 Pos0 = PointSubpixelClip0.xyz * RcpW.x;
	const float3 Pos1 = PointSubpixelClip1.xyz * RcpW.y;
	const float3 Pos2 = PointSubpixelClip2.xyz * RcpW.z;

	const float3 Pos120X = float3(Pos1.x, Pos2.x, Pos0.x);
	const float3 Pos120Y = float3(Pos1.y, Pos2.y, Pos0.y);
	const float3 Pos201X = float3(Pos2.x, Pos0.x, Pos1.x);
	const float3 Pos201Y = float3(Pos2.y, Pos0.y, Pos1.y);

	const float3 C_dx = Pos201Y - Pos120Y;
	const float3 C_dy = Pos120X - Pos201X;

	const float3 C = C_dx * (Subpixel.x - Pos120X) + C_dy * (Subpixel.y - Pos120Y);	// Evaluate the 3 edge functions
	const float3 G = C * RcpW;

	const float H = dot(C, RcpW);
	const float RcpH = rcp(H);

	// UVW = C * RcpW / dot(C, RcpW)
	Result.UVW = G * RcpH;

	// Texture coordinate derivatives:
	// UVW = G / H where G = C * RcpW and H = dot(C, RcpW)
	// UVW' = (G' * H - G * H') / H^2
	// float2 TexCoordDX = UVW_dx.y * TexCoord10 + UVW_dx.z * TexCoord20;
	// float2 TexCoordDY = UVW_dy.y * TexCoord10 + UVW_dy.z * TexCoord20;
	const float3 G_dx = C_dx * RcpW;
	const float3 G_dy = C_dy * RcpW;

	const float H_dx = dot(C_dx, RcpW);
	const float H_dy = dot(C_dy, RcpW);

	Result.UVW_dx = (G_dx * H - G * H_dx) * (RcpH * RcpH) * NANITE_SUBPIXEL_SAMPLES;
	Result.UVW_dy = (G_dy * H - G * H_dy) * (RcpH * RcpH) * NANITE_SUBPIXEL_SAMPLES;

	return Result;
}

uint CalculateMaxAttributeBits(uint NumTexCoordInterpolators)
{
	return (2 * NANITE_NORMAL_QUANTIZATION_BITS + 4 * NANITE_MAX_COLOR_QUANTIZATION_BITS + NumTexCoordInterpolators * (2 * NANITE_MAX_TEXCOORD_QUANTIZATION_BITS));
}

void DecodeMaterialRange(uint EncodedRange, out uint TriStart, out uint TriLength, out uint MaterialIndex)
{
	// uint32 TriStart      :  8; // max 128 triangles
	// uint32 TriLength     :  8; // max 128 triangles
	// uint32 MaterialIndex :  6; // max  64 materials
	// uint32 Padding       : 10;

	TriStart = BitFieldExtractU32(EncodedRange, 8, 0);
	TriLength = BitFieldExtractU32(EncodedRange, 8, 8);
	MaterialIndex = BitFieldExtractU32(EncodedRange, 6, 16);
}

bool IsMaterialFastPath(FCluster InCluster)
{
	return (InCluster.Material0Length > 0);
}

uint GetMaterialCount(FCluster InCluster)
{
	if (IsMaterialFastPath(InCluster))
	{
		const uint Material2Length = InCluster.NumTris - InCluster.Material0Length - InCluster.Material1Length;
		return 1 + (InCluster.Material1Length > 0) + (Material2Length > 0);
	}
	else
	{
		return InCluster.MaterialTableLength;
	}
}

uint GetRelativeMaterialIndex(FCluster InCluster, uint InTriIndex)
{
	uint MaterialIndex = 0xFFFFFFFF;

	[branch]
	if (IsMaterialFastPath(InCluster))
	{
		if (InTriIndex < InCluster.Material0Length)
		{
			MaterialIndex = InCluster.Material0Index;
		}
		else if (InTriIndex < (InCluster.Material0Length + InCluster.Material1Length))
		{
			MaterialIndex = InCluster.Material1Index;
		}
		else
		{
			MaterialIndex = InCluster.Material2Index;
		}
	}
	else
	{
		uint TableOffset = InCluster.PageBaseAddress + InCluster.MaterialTableOffset * 4;
		[loop]
		for (uint TableEntry = 0; TableEntry < InCluster.MaterialTableLength; ++TableEntry)
		{
			uint EncodedRange = ClusterPageData.Load(TableOffset);
			TableOffset += 4;

			uint TriStart;
			uint TriLength;
			uint TriMaterialIndex;
			DecodeMaterialRange(EncodedRange, TriStart, TriLength, TriMaterialIndex);

			if (InTriIndex >= TriStart && InTriIndex < (TriStart + TriLength))
			{
				MaterialIndex = TriMaterialIndex;
				break;
			}
		}
	}

	return MaterialIndex;
}

uint RemapMaterialIndexToOffset(uint InPrimitiveIndex, uint InMaterialIndex, uint BytesPerMaterial)
{
	// Remap local primitive material indices (i.e. 0...8) to global indices of all primitives in current scene
	const uint MaxMaterials = 64;
	const uint GlobalMaterialIndex = InPrimitiveIndex * MaxMaterials + InMaterialIndex;
	const uint RemapOffset = GlobalMaterialIndex * BytesPerMaterial;
	return RemapOffset;
}

struct FNaniteMaterialSlot {
	uint MaterialIndex;
	uint RasterSlot;
	uint SecondaryRasterSlot;
};

FNaniteMaterialSlot UnpackMaterialSlot(uint2 Data)
{
	FNaniteMaterialSlot Output;
	Output.MaterialIndex = Data.x >> 16u;
	Output.RasterSlot = Data.x & 0xFFFFu;
	Output.SecondaryRasterSlot = Data.y;

	return Output;
}

FNaniteMaterialSlot LoadMaterialSlot(ByteAddressBuffer InMaterialTable, uint Offset)
{
	uint2 Data = InMaterialTable.Load2(Offset);
	return UnpackMaterialSlot(Data);
}

FNaniteMaterialSlot LoadMaterialSlot(uint InPrimitiveIndex, uint InMaterialIndex, ByteAddressBuffer InMaterialTable)
{
	const uint BytesPerMaterial = 4 * 2; // 2 dwords per material
	const uint RemapOffset = RemapMaterialIndexToOffset(InPrimitiveIndex, InMaterialIndex, BytesPerMaterial);
	return LoadMaterialSlot(InMaterialTable, RemapOffset);
}

uint GetMaterialShadingSlotFromIndex(
	uint InRelativeMaterialIndex,
	uint InPrimitiveIndex,
	ByteAddressBuffer InMaterialSlotTable)
{
	FNaniteMaterialSlot MaterialSlot = LoadMaterialSlot(InPrimitiveIndex, InRelativeMaterialIndex, InMaterialSlotTable);
	return MaterialSlot.MaterialIndex;
}

uint GetMaterialRasterSlotFromIndex(
	uint InRelativeMaterialIndex,
	uint InPrimitiveIndex,
	uint InRegularSlotCount,
	bool bSecondaryBin,
	ByteAddressBuffer InMaterialSlotTable)
{
	FNaniteMaterialSlot MaterialSlot = LoadMaterialSlot(InPrimitiveIndex, InRelativeMaterialIndex, InMaterialSlotTable);
	uint RasterSlot = MaterialSlot.RasterSlot;
	if (bSecondaryBin && MaterialSlot.SecondaryRasterSlot != 0xFFFFFFFFu)
	{
		RasterSlot = MaterialSlot.SecondaryRasterSlot;
	}
	if (RasterSlot >= InRegularSlotCount)
	{
		RasterSlot = 0xffff - RasterSlot + InRegularSlotCount;
	}

	return RasterSlot;
}

uint GetMaterialShadingSlot(
	FCluster InCluster,
	uint InPrimitiveIndex,
	uint InTriIndex,
	ByteAddressBuffer InMaterialSlotTable)
{
	const uint RelativeMaterialIndex = GetRelativeMaterialIndex(InCluster, InTriIndex);
	return GetMaterialShadingSlotFromIndex(RelativeMaterialIndex, InPrimitiveIndex, InMaterialSlotTable);
}

uint GetMaterialRasterSlot(
	FCluster InCluster,
	uint InPrimitiveIndex,
	uint InTriIndex,
	uint InRegularSlotCount,
	bool bSecondaryBin,
	ByteAddressBuffer InMaterialSlotTable)
{
	const uint RelativeMaterialIndex = GetRelativeMaterialIndex(InCluster, InTriIndex);
	return GetMaterialRasterSlotFromIndex(RelativeMaterialIndex, InPrimitiveIndex, InRegularSlotCount, bSecondaryBin, InMaterialSlotTable);
}

uint GetMaterialDepthId(
	uint InMaterialSlot,
	ByteAddressBuffer InMaterialDepthTable)
{
	const uint MaterialDepthId = InMaterialDepthTable.Load(InMaterialSlot * 4);
	return MaterialDepthId;
}

uint LoadMaterialHitProxyId(uint InPrimitiveIndex, uint InMaterialIndex, ByteAddressBuffer InMaterialHitProxyTable)
{
	const uint BytesPerMaterial = 4; // Hit proxy ID
	const uint RemapOffset = RemapMaterialIndexToOffset(InPrimitiveIndex, InMaterialIndex, BytesPerMaterial);
	return InMaterialHitProxyTable.Load(RemapOffset);
}

uint GetMaterialHitProxyId(
	FCluster InCluster,
	uint InPrimitiveIndex,
	uint InTriIndex,
	ByteAddressBuffer InMaterialHitProxyTable)
{
	const uint RelativeMaterialIndex = GetRelativeMaterialIndex(InCluster, InTriIndex);
	const uint MaterialHitProxyId = LoadMaterialHitProxyId(InPrimitiveIndex, RelativeMaterialIndex, InMaterialHitProxyTable);
	return MaterialHitProxyId;
}

uint GetMaterialBucketIdFromDepth(float Depth)
{
	return (uint)(Depth * NANITE_MAX_STATE_BUCKET_ID);
}

// TODO: We should really expose the derivative types from the material compiler and use them instead.
struct FTexCoord
{
	float2 Value;
	float2 DDX;
	float2 DDY;
};

FTexCoord InterpolateTexCoord(float2 TexCoord0, float2 TexCoord1, float2 TexCoord2, FBarycentrics Barycentrics)
{
	float2 TexCoord10 = TexCoord1 - TexCoord0;
	float2 TexCoord20 = TexCoord2 - TexCoord0;

	FTexCoord Result;
	Result.Value		= TexCoord0 + Barycentrics.UVW.y * TexCoord10 + Barycentrics.UVW.z * TexCoord20;
	Result.DDX			= Barycentrics.UVW_dx.y * TexCoord10 + Barycentrics.UVW_dx.z * TexCoord20;
	Result.DDY			= Barycentrics.UVW_dy.y * TexCoord10 + Barycentrics.UVW_dy.z * TexCoord20;
	return Result;
}

// Decodes vertex attributes for N vertices. N must be compile-time constant and <= 3.
// Decoding multiple vertices from the same cluster simultaneously tends to generate better code than decoding them individually.
void GetRawAttributeDataN(inout FNaniteRawAttributeData RawAttributeData[3],
	FCluster Cluster,
	uint3 TriIndices,
	uint CompileTimeN,
	uint CompileTimeMaxTexCoords
)
{
	// Always process first UV set. Even if it isn't used, we might still need TangentToWorld.
	CompileTimeMaxTexCoords = max(1, min(NANITE_MAX_UVS, CompileTimeMaxTexCoords));

	const uint DecodeInfoOffset = Cluster.PageBaseAddress + Cluster.DecodeInfoOffset;
	const uint AttributeDataOffset = Cluster.PageBaseAddress + Cluster.AttributeOffset;

	uint i;
	[unroll]
	for (i = 0; i < CompileTimeN; i++)
	{
		RawAttributeData[i] = (FNaniteRawAttributeData)0;
	}

#if NANITE_USE_UNCOMPRESSED_VERTEX_DATA
	uint3 ReadOffset = AttributeDataOffset + TriIndices * Cluster.BitsPerAttribute / 8;
	[unroll]
	for(i = 0; i < CompileTimeN; i++)
	{
		RawAttributeData[i].TangentZ = asfloat(ClusterPageData.Load3(ReadOffset[i]));
		ReadOffset[i] += 12;
		RawAttributeData[i].Color = float4(UnpackToUint4(ClusterPageData.Load(ReadOffset[i]), 8)) * (1.0f / 255.0f);
		ReadOffset[i] += 4;
	}

	[unroll]
	for (uint TexCoordIndex = 0; TexCoordIndex < CompileTimeMaxTexCoords; TexCoordIndex++)
	{
		if(TexCoordIndex < Cluster.NumUVs)
		{
			[unroll]
			for (uint i = 0; i < CompileTimeN; i++)
			{
				RawAttributeData[i].TexCoords[TexCoordIndex] = asfloat(ClusterPageData.Load2(ReadOffset[i]));
			}
			ReadOffset += 8;
		}
	}
#else
	const uint MaxAttributeBits = CalculateMaxAttributeBits(CompileTimeMaxTexCoords);

	// Watch out! Make sure control flow around BitStreamReader is always compile-time constant or codegen degrades significantly

	uint4 ColorMin = uint4(UnpackByte0(Cluster.ColorMin), UnpackByte1(Cluster.ColorMin), UnpackByte2(Cluster.ColorMin), UnpackByte3(Cluster.ColorMin));
	const uint4 NumComponentBits = UnpackToUint4(Cluster.ColorBits, 4);

	FBitStreamReaderState AttributeStream[3];
	[unroll]
	for (i = 0; i < CompileTimeN; i++)
	{
		AttributeStream[i] = BitStreamReader_Create_Aligned(AttributeDataOffset, TriIndices[i] * Cluster.BitsPerAttribute, MaxAttributeBits);
		const uint NormalBits = BitStreamReader_Read_RO(ClusterPageData, AttributeStream[i], 2 * NANITE_NORMAL_QUANTIZATION_BITS, 2 * NANITE_NORMAL_QUANTIZATION_BITS);
		RawAttributeData[i].TangentZ = UnpackNormal(NormalBits, NANITE_NORMAL_QUANTIZATION_BITS);

		const uint4 ColorDelta = BitStreamReader_Read4_RO(ClusterPageData, AttributeStream[i], NumComponentBits, NANITE_MAX_COLOR_QUANTIZATION_BITS);
		RawAttributeData[i].Color = float4(ColorMin + ColorDelta) * (1.0f / 255.0f);
	}

	//[unroll]
	//for (uint TexCoordIndex = 0; TexCoordIndex < CompileTimeMaxTexCoords; ++TexCoordIndex)
	uint TexCoordIndex = 0;
	{
		const uint2 UVPrec = uint2(BitFieldExtractU32(Cluster.UV_Prec, 4, TexCoordIndex * 8), BitFieldExtractU32(Cluster.UV_Prec, 4, TexCoordIndex * 8 + 4));
		
		uint2 UVBits[3];
		[unroll]
		for (uint i = 0; i < CompileTimeN; i++)
		{
			UVBits[i] = BitStreamReader_Read2_RO(ClusterPageData, AttributeStream[i], UVPrec, NANITE_MAX_TEXCOORD_QUANTIZATION_BITS);
		}

		[branch]
		if (TexCoordIndex < Cluster.NumUVs)
		{
			FUVRange UVRange = GetUVRange(ClusterPageData, DecodeInfoOffset, TexCoordIndex);
			[unroll]
			for (uint i = 0; i < CompileTimeN; i++)
			{
				RawAttributeData[i].TexCoords[TexCoordIndex] = UnpackTexCoord(UVBits[i], UVRange);
			}
		}
	}
#endif
}

void GetRawAttributeData3(inout FNaniteRawAttributeData RawAttributeData[3],
	FCluster Cluster,
	uint3 VertexIndices,
	uint CompileTimeMaxTexCoords
	)
{
	GetRawAttributeDataN(RawAttributeData, Cluster, VertexIndices, 3, CompileTimeMaxTexCoords);
}

FNaniteRawAttributeData GetRawAttributeData(
	FCluster Cluster,
	uint VertexIndex,
	uint CompileTimeMaxTexCoords
	)
{
	FNaniteRawAttributeData RawAttributeData[3];
	GetRawAttributeDataN(RawAttributeData, Cluster, VertexIndex, 1, CompileTimeMaxTexCoords);
	return RawAttributeData[0];
}

FNaniteAttributeData GetAttributeData(
	FCluster Cluster,
	float3 PointLocal0,
	float3 PointLocal1,
	float3 PointLocal2,
	FNaniteRawAttributeData RawAttributeData0,
	FNaniteRawAttributeData RawAttributeData1,
	FNaniteRawAttributeData RawAttributeData2,
	FBarycentrics Barycentrics,
	//FInstanceSceneData InstanceData,
	uint CompileTimeMaxTexCoords
)
{
	FNaniteAttributeData AttributeData = (FNaniteAttributeData)0;

	// Always process first UV set. Even if it isn't used, we might still need TangentToWorld.
	CompileTimeMaxTexCoords = max(1, min(NANITE_MAX_UVS, CompileTimeMaxTexCoords));

	float3 TangentZ = normalize(Barycentrics.UVW.x * RawAttributeData0.TangentZ + Barycentrics.UVW.y * RawAttributeData1.TangentZ + Barycentrics.UVW.z * RawAttributeData2.TangentZ);

	// Decode vertex color
	// This needs to happen even if INTERPOLATE_VERTEX_COLOR is not defined as the data might be there regardless of what the shader needs.
	// When INTERPOLATE_VERTEX_COLOR is not defined, the results are not used and the code mostly disappears.
	AttributeData.UnMirrored = 1.0f;

#if NANITE_USE_UNCOMPRESSED_VERTEX_DATA
	AttributeData.VertexColor = Barycentrics.UVW.x * RawAttributeData0.Color + Barycentrics.UVW.y * RawAttributeData1.Color + Barycentrics.UVW.z * RawAttributeData2.Color;
#else
	AttributeData.VertexColor = RawAttributeData0.Color;
	
	if (Cluster.ColorMode == NANITE_VERTEX_COLOR_MODE_VARIABLE)
	{
		AttributeData.VertexColor = Barycentrics.UVW.x * RawAttributeData0.Color + Barycentrics.UVW.y * RawAttributeData1.Color + Barycentrics.UVW.z * RawAttributeData2.Color;
		AttributeData.VertexColor_DDX = Barycentrics.UVW_dx.x * RawAttributeData0.Color + Barycentrics.UVW_dx.y * RawAttributeData1.Color + Barycentrics.UVW_dx.z * RawAttributeData2.Color;
		AttributeData.VertexColor_DDY = Barycentrics.UVW_dy.x * RawAttributeData0.Color + Barycentrics.UVW_dy.y * RawAttributeData1.Color + Barycentrics.UVW_dy.z * RawAttributeData2.Color;
	}
#endif

	[unroll]
	for (uint TexCoordIndex = 0; TexCoordIndex < CompileTimeMaxTexCoords; ++TexCoordIndex)
	{
		if (TexCoordIndex < Cluster.NumUVs)
		{
			FTexCoord TexCoord = InterpolateTexCoord(RawAttributeData0.TexCoords[TexCoordIndex], RawAttributeData1.TexCoords[TexCoordIndex], RawAttributeData2.TexCoords[TexCoordIndex], Barycentrics);
			AttributeData.TexCoords[TexCoordIndex] = TexCoord.Value;
			AttributeData.TexCoords_DDX[TexCoordIndex] = TexCoord.DDX;
			AttributeData.TexCoords_DDY[TexCoordIndex] = TexCoord.DDY;

			// Generate tangent frame for UV0
			if (TexCoordIndex == 0)
			{
				// Implicit tangent space
				// Based on Christian Schlüler's derivation: http://www.thetenthplanet.de/archives/1180
				// The technique derives a tangent space from the interpolated normal and (position,uv) deltas in two not necessarily orthogonal directions.
				// The described technique uses screen space derivatives as a way to obtain these direction deltas in a pixel shader,
				// but as we have the triangle vertices explicitly available using the local space corner deltas directly is faster and more convenient.

				float3 PointLocal10 = PointLocal1 - PointLocal0;
				float3 PointLocal20 = PointLocal2 - PointLocal0;
				float2 TexCoord10 = RawAttributeData1.TexCoords[0] - RawAttributeData0.TexCoords[0];
				float2 TexCoord20 = RawAttributeData2.TexCoords[0] - RawAttributeData0.TexCoords[0];

				bool TangentXValid = abs(TexCoord10.x) + abs(TexCoord20.x) > 1e-6;

				float3 TangentX;
				float3 TangentY;
				if (TangentXValid)
				{
					float3 Perp2 = cross(TangentZ, PointLocal20);
					float3 Perp1 = cross(PointLocal10, TangentZ);
					float3 TangentU = Perp2 * TexCoord10.x + Perp1 * TexCoord20.x;
					float3 TangentV = Perp2 * TexCoord10.y + Perp1 * TexCoord20.y;

					TangentX = normalize(TangentU);
					TangentY = cross(TangentZ, TangentX);

					AttributeData.UnMirrored = dot(TangentV, TangentY) < 0.0f ? -1.0f : 1.0f;
					TangentY *= AttributeData.UnMirrored;
				}
				else
				{
					const float Sign = TangentZ.z >= 0 ? 1 : -1;
					const float a = -rcp( Sign + TangentZ.z );
					const float b = TangentZ.x * TangentZ.y * a;
	
					TangentX = float3(1 + Sign * a * Pow2(TangentZ.x), Sign * b, -Sign * TangentZ.x);
					TangentY = float3(b,  Sign + a * Pow2(TangentZ.y), -TangentZ.y);

					AttributeData.UnMirrored = 1;
				}

				float3x3 TangentToLocal = float3x3(TangentX, TangentY, TangentZ);

				// Tix: no instance data, only apply a default transform
				// Should be Pow2(InvScale) but that requires renormalization
				//float3x3 LocalToWorld = LWCToFloat3x3(InstanceData.LocalToWorld);
				//float3 InvScale = InstanceData.InvNonUniformScale;
				//LocalToWorld[0] *= InvScale.x;
				//LocalToWorld[1] *= InvScale.y;
				//LocalToWorld[2] *= InvScale.z;
				//AttributeData.TangentToWorld = mul(TangentToLocal, LocalToWorld);
				AttributeData.TangentToWorld = TangentToLocal;
			}
		}
		else
		{
			AttributeData.TexCoords[TexCoordIndex]		= float2(0, 0);
			AttributeData.TexCoords_DDX[TexCoordIndex]	= float2(0, 0);
			AttributeData.TexCoords_DDY[TexCoordIndex]	= float2(0, 0);
			if (TexCoordIndex == 0)
			{
				// Tix: no instance data, only apply a default transform
				//AttributeData.TangentToWorld = float3x3(float3(0, 0, 0), float3(0, 0, 0), LWCMultiplyVector(TangentZ * InstanceData.InvNonUniformScale.z, InstanceData.LocalToWorld));
				AttributeData.TangentToWorld = float3x3(float3(0, 0, 0), float3(0, 0, 0), TangentZ);
			}
		}
	}

	return AttributeData;
}

FTexCoord GetTexCoord(
	FCluster Cluster,
	uint3 TriIndices,
	FBarycentrics Barycentrics,
	uint TexCoordIndex
)
{
	if (TexCoordIndex >= Cluster.NumUVs)
		return (FTexCoord)0;

	// Unpack and interpolate attributes
	const uint DecodeInfoOffset = Cluster.PageBaseAddress + Cluster.DecodeInfoOffset;
	const uint AttributeDataOffset = Cluster.PageBaseAddress + Cluster.AttributeOffset;

#if NANITE_USE_UNCOMPRESSED_VERTEX_DATA
	uint3 ReadOffset = AttributeDataOffset + TriIndices * Cluster.BitsPerAttribute / 8;
	ReadOffset += 12 + 4 + TexCoordIndex * 8;	// Normal + Color + TexCoord
#else
	const uint4 NumColorComponentBits = UnpackToUint4(Cluster.ColorBits, 4);
	
	// Sum U and V bit costs to combined UV bit costs in 8:8:8:8.
	const uint NumUVBits_8888 = (Cluster.UV_Prec & 0x0F0F0F0Fu) + ((Cluster.UV_Prec >> 4) & 0x0F0F0F0Fu);	// 8:8:8:8 (U0+V0, U1+V1, U2+V2, U3+V3)
	const uint UVBitsOffset_8888 = NumUVBits_8888 * 0x01010100u;											// 8:8:8:8 (0, UV0, UV0+UV1, UV0+UV1+UV2)
	const uint TexCoord_OffsetBits = (UVBitsOffset_8888 >> (8 * TexCoordIndex)) & 0xFFu;

	const uint BitOffset = 2 * NANITE_NORMAL_QUANTIZATION_BITS + dot(NumColorComponentBits, 1u) + TexCoord_OffsetBits;

	FBitStreamReaderState AttributeStream0 = BitStreamReader_Create_Aligned(AttributeDataOffset, BitOffset + TriIndices.x * Cluster.BitsPerAttribute, 2 * NANITE_MAX_TEXCOORD_QUANTIZATION_BITS);
	FBitStreamReaderState AttributeStream1 = BitStreamReader_Create_Aligned(AttributeDataOffset, BitOffset + TriIndices.y * Cluster.BitsPerAttribute, 2 * NANITE_MAX_TEXCOORD_QUANTIZATION_BITS);
	FBitStreamReaderState AttributeStream2 = BitStreamReader_Create_Aligned(AttributeDataOffset, BitOffset + TriIndices.z * Cluster.BitsPerAttribute, 2 * NANITE_MAX_TEXCOORD_QUANTIZATION_BITS);
#endif

#if NANITE_USE_UNCOMPRESSED_VERTEX_DATA
	float2 TexCoord0 = asfloat(ClusterPageData.Load2(ReadOffset.x));
	float2 TexCoord1 = asfloat(ClusterPageData.Load2(ReadOffset.y));
	float2 TexCoord2 = asfloat(ClusterPageData.Load2(ReadOffset.z));
#else
	uint2 UVPrec = uint2(BitFieldExtractU32(Cluster.UV_Prec, 4, TexCoordIndex * 8), BitFieldExtractU32(Cluster.UV_Prec, 4, TexCoordIndex * 8 + 4));
	uint2 Bits0_UV = BitStreamReader_Read2_RO(ClusterPageData, AttributeStream0, UVPrec, NANITE_MAX_TEXCOORD_QUANTIZATION_BITS);
	uint2 Bits1_UV = BitStreamReader_Read2_RO(ClusterPageData, AttributeStream1, UVPrec, NANITE_MAX_TEXCOORD_QUANTIZATION_BITS);
	uint2 Bits2_UV = BitStreamReader_Read2_RO(ClusterPageData, AttributeStream2, UVPrec, NANITE_MAX_TEXCOORD_QUANTIZATION_BITS);

	FUVRange UVRange = GetUVRange(ClusterPageData, DecodeInfoOffset, TexCoordIndex);
	float2 TexCoord0 = UnpackTexCoord(Bits0_UV, UVRange);
	float2 TexCoord1 = UnpackTexCoord(Bits1_UV, UVRange);
	float2 TexCoord2 = UnpackTexCoord(Bits2_UV, UVRange);
#endif

	return InterpolateTexCoord(TexCoord0, TexCoord1, TexCoord2, Barycentrics);	
}

// TODO: Move to an appropriate common location outside of Nanite
uint LowerBound(Buffer<uint> SearchBuffer, uint BufferCount, uint Key)
{
	uint Index = 0;
	uint Width = BufferCount >> 1;
	
	[unroll]
	while (Width > 0)
	{
		Index += (Key < SearchBuffer[Index + Width]) ? 0 : Width;
		Width = Width >> 1;
	}
	
	return Index;
}

// TODO: Move to an appropriate common location outside of Nanite
bool BinarySearch(Buffer<uint> SearchBuffer, uint BufferCount, uint Key)
{
	uint Index = LowerBound(SearchBuffer, BufferCount, Key);
	return SearchBuffer[Index] == Key;
}

#ifndef DEFINE_ITERATE_CLUSTER_SEGMENTS
#	define DEFINE_ITERATE_CLUSTER_SEGMENTS (0)
#endif

// Need manually strip unused template functions here due to a compiler issue: https://github.com/microsoft/DirectXShaderCompiler/issues/4649
#if DEFINE_ITERATE_CLUSTER_SEGMENTS

template<class ClusterSegmentProcessor>
void IterateClusterSegments(FCluster Cluster, ByteAddressBuffer InClusterPageData, inout ClusterSegmentProcessor Processor)
{
	[branch]
	if (IsMaterialFastPath(Cluster))
	{
		{
			Processor.Process(0, Cluster.Material0Length, Cluster.Material0Index);
		}

		if (Cluster.Material1Length > 0)
		{
			Processor.Process(Cluster.Material0Length, Cluster.Material1Length, Cluster.Material1Index);
		}

		const uint Material2Length = Cluster.NumTris - Cluster.Material0Length - Cluster.Material1Length;
		if (Material2Length > 0)
		{
			Processor.Process(Cluster.Material0Length + Cluster.Material1Length, Material2Length, Cluster.Material2Index);
		}
	}
	else
	{
		uint TableOffset = Cluster.PageBaseAddress + Cluster.MaterialTableOffset * 4;
		LOOP for (uint TableEntry = 0; TableEntry < Cluster.MaterialTableLength; ++TableEntry)
		{
			uint EncodedRange = InClusterPageData.Load(TableOffset);
			TableOffset += 4;

			uint TriStart;
			uint TriLength;
			uint MaterialIndex;
			DecodeMaterialRange(EncodedRange, TriStart, TriLength, MaterialIndex);

			Processor.Process(TriStart, TriLength, MaterialIndex);
		}
	}
}

#endif
