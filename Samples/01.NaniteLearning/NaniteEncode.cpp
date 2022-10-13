/*
	TiX Engine v2.0 Copyright (C) 2018~2021
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "NaniteEncode.h"
#include "Stripifier.h"
#include "NaniteMesh.h"

#define INVALID_PART_INDEX				0xFFFFFFFFu
#define INVALID_GROUP_INDEX				0xFFFFFFFFu
#define INVALID_PAGE_INDEX				0xFFFFFFFFu

const float FLT_INT_MIN = (float)TNumLimit<int32>::min();
const float FLT_INT_MAX = (float)TNumLimit<int32>::max();
const float MAX_flt = TNumLimit<float>::max();
const uint32 MAX_uint32 = TNumLimit<uint32>::max();

struct FClusterGroupPart					// Whole group or a part of a group that has been split.
{
	TVector<uint32> Clusters;				// Can be reordered during page allocation, so we need to store a list here.
	FBox Bounds;
	uint32 PageIndex;
	uint32 GroupIndex;				// Index of group this is a part of.
	uint32 HierarchyNodeIndex;
	uint32 HierarchyChildIndex;
	uint32 PageClusterOffset;
};

struct FPageSections
{
	uint32 Cluster = 0;
	uint32 MaterialTable = 0;
	uint32 DecodeInfo = 0;
	uint32 Index = 0;
	uint32 Position = 0;
	uint32 Attribute = 0;

	uint32 GetMaterialTableSize() const { return TMath::Align(MaterialTable, 16); }
	uint32 GetClusterOffset() const { return NANITE_GPU_PAGE_HEADER_SIZE; }
	uint32 GetMaterialTableOffset() const { return GetClusterOffset() + Cluster; }
	uint32 GetDecodeInfoOffset() const { return GetMaterialTableOffset() + GetMaterialTableSize(); }
	uint32 GetIndexOffset() const { return GetDecodeInfoOffset() + DecodeInfo; }
	uint32 GetPositionOffset() const { return GetIndexOffset() + Index; }
	uint32 GetAttributeOffset() const { return GetPositionOffset() + Position; }
	uint32 GetTotal() const { return GetAttributeOffset() + Attribute; }

	FPageSections GetOffsets() const
	{
		return FPageSections{ GetClusterOffset(), GetMaterialTableOffset(), GetDecodeInfoOffset(), GetIndexOffset(), GetPositionOffset(), GetAttributeOffset() };
	}

	void operator+=(const FPageSections& Other)
	{
		Cluster += Other.Cluster;
		MaterialTable += Other.MaterialTable;
		DecodeInfo += Other.DecodeInfo;
		Index += Other.Index;
		Position += Other.Position;
		Attribute += Other.Attribute;
	}
};

struct FPage
{
	uint32	PartsStartIndex = 0;
	uint32	PartsNum = 0;
	uint32	NumClusters = 0;

	FPageSections	GpuSizes;
};

// TODO: optimize me
struct FUVRange
{
	FInt2 Min;
	FInt2 GapStart;
	FInt2 GapLength;
	int32 Precision = 0;
	int32 Pad = 0;
};

struct FEncodingInfo
{
	uint32 BitsPerIndex;
	uint32 BitsPerAttribute;
	uint32 UVPrec;

	uint32 ColorMode;
	FInt4 ColorMin;
	FInt4 ColorBits;

	FPageSections GpuSizes;

	FUVRange UVRanges[NANITE_MAX_UVS];

	FEncodingInfo& operator = (const FEncodingInfo& Other)
	{
		BitsPerIndex = Other.BitsPerIndex;
		BitsPerAttribute = Other.BitsPerAttribute;
		UVPrec = Other.UVPrec;
		ColorMode = Other.ColorMode;
		ColorMin = Other.ColorMin;
		ColorBits = Other.ColorBits;
		GpuSizes = Other.GpuSizes;
		for (int32 i = 0; i < NANITE_MAX_UVS; i++)
		{
			UVRanges[i] = Other.UVRanges[i];
		}
		return *this;
	}
};


void BuildMaterialRanges(TVector<FCluster>& ClusterSources)
{
	const int32 NumClusters = (int32)ClusterSources.size();
	OMP_PARALLEL_FOR
	for (int32 c = 0; c < NumClusters; c++)
	{
		FCluster& Cluster = ClusterSources[c];

		TVector<FMaterialTriangle> MaterialTris;
		MaterialTris.reserve(128);

		TVector<FMaterialRange>& MaterialRanges = Cluster.MaterialRanges;
		const TVector<uint32>& TriangleIndices = Cluster.Indexes;
		const TVector<int32>& MaterialIndices = Cluster.MaterialIndexes;

		TI_ASSERT(MaterialTris.size() == 0);
		TI_ASSERT(MaterialRanges.size() == 0);
		TI_ASSERT(MaterialIndices.size() * 3 == TriangleIndices.size());

		const uint32 TriangleCount = (uint32)MaterialIndices.size();

		TVector<uint32> MaterialCounts;
		MaterialCounts.resize(64);

		// Tally up number tris per material index
		for (uint32 i = 0; i < TriangleCount; i++)
		{
			const uint32 MaterialIndex = MaterialIndices[i];
			++MaterialCounts[MaterialIndex];
		}

		for (uint32 i = 0; i < TriangleCount; i++)
		{
			FMaterialTriangle MaterialTri;
			MaterialTri.Index0 = TriangleIndices[(i * 3) + 0];
			MaterialTri.Index1 = TriangleIndices[(i * 3) + 1];
			MaterialTri.Index2 = TriangleIndices[(i * 3) + 2];
			MaterialTri.MaterialIndex = MaterialIndices[i];
			MaterialTri.RangeCount = MaterialCounts[MaterialTri.MaterialIndex];
			TI_ASSERT(MaterialTri.RangeCount > 0);
			MaterialTris.push_back(MaterialTri);
		}

		// Sort by triangle range count descending, and material index ascending.
		// This groups the material ranges from largest to smallest, which is
		// more efficient for evaluating the sequences on the GPU, and also makes
		// the minus one encoding work (the first range must have more than 1 tri).
		TSort(MaterialTris.begin(), MaterialTris.end(), 
			[](const FMaterialTriangle& A, const FMaterialTriangle& B)
			{
				if (A.RangeCount != B.RangeCount)
				{
					return (A.RangeCount > B.RangeCount);
				}

				return (A.MaterialIndex < B.MaterialIndex);
			});

		FMaterialRange CurrentRange;
		CurrentRange.RangeStart = 0;
		CurrentRange.RangeLength = 0;
		CurrentRange.MaterialIndex = MaterialTris.size() > 0 ? MaterialTris[0].MaterialIndex : 0;

		for (int32 TriIndex = 0; TriIndex < (int32)MaterialTris.size(); ++TriIndex)
		{
			const FMaterialTriangle& Triangle = MaterialTris[TriIndex];

			// Material changed, so add current range and reset
			if (CurrentRange.RangeLength > 0 && Triangle.MaterialIndex != CurrentRange.MaterialIndex)
			{
				MaterialRanges.push_back(CurrentRange);

				CurrentRange.RangeStart = TriIndex;
				CurrentRange.RangeLength = 1;
				CurrentRange.MaterialIndex = Triangle.MaterialIndex;
			}
			else
			{
				++CurrentRange.RangeLength;
			}
		}

		// Add last triangle to range
		if (CurrentRange.RangeLength > 0)
		{
			MaterialRanges.push_back(CurrentRange);
		}

		TI_ASSERT(MaterialTris.size() == TriangleCount);

		// Write indices back to clusters
		for (uint32 Triangle = 0; Triangle < Cluster.NumTris; ++Triangle)
		{
			Cluster.Indexes[Triangle * 3 + 0] = MaterialTris[Triangle].Index0;
			Cluster.Indexes[Triangle * 3 + 1] = MaterialTris[Triangle].Index1;
			Cluster.Indexes[Triangle * 3 + 2] = MaterialTris[Triangle].Index2;
			Cluster.MaterialIndexes[Triangle] = MaterialTris[Triangle].MaterialIndex;
		}
	}
}

void ConstrainClusters(TVector<FClusterGroup>& Groups, TVector<FCluster>& ClusterSources, bool bDoStripify = true)
{
	// Calculate stats
	uint32 TotalOldTriangles = 0;
	uint32 TotalOldVertices = 0;
	for (const FCluster& Cluster : ClusterSources)
	{
		TotalOldTriangles += Cluster.NumTris;
		TotalOldVertices += Cluster.NumVerts;
	}

	if (bDoStripify)
	{
		const int32 NumClusters = (int32)ClusterSources.size();
		for (int32 c = 0; c < NumClusters; c++)
		{
			FStripifier Stripifier;
			Stripifier.ConstrainAndStripifyCluster(ClusterSources[c]);
		}
	}
	else
	{
		for (auto& Cluster : ClusterSources)
		{
			Cluster.Flags |= NANITE_CLUSTER_FLAG_NO_STRIPIFY;
		}
	}

	uint32 TotalNewTriangles = 0;
	uint32 TotalNewVertices = 0;

	// Constrain clusters
	const uint32 NumOldClusters = (uint32)ClusterSources.size();
	for (uint32 i = 0; i < NumOldClusters; i++)
	{
		TotalNewTriangles += ClusterSources[i].NumTris;
		TotalNewVertices += ClusterSources[i].NumVerts;

		// we make sure cluster verts not > 256.
		// or else we need to deal with cluster instance split
		TI_ASSERT(ClusterSources[i].NumVerts <= 256);

		// Split clusters with too many verts
		//if (ClusterSources[i].NumVerts > 256)
		//{
		//	FCluster ClusterA, ClusterB;
		//	uint32 NumTrianglesA = ClusterSources[i].NumTris / 2;
		//	uint32 NumTrianglesB = ClusterSources[i].NumTris - NumTrianglesA;
		//	BuildClusterFromClusterTriangleRange(ClusterSources[i], ClusterA, 0, NumTrianglesA);
		//	BuildClusterFromClusterTriangleRange(ClusterSources[i], ClusterB, NumTrianglesA, NumTrianglesB);
		//	ClusterSources[i] = ClusterA;
		//	ClusterGroups[ClusterB.GroupIndex].Children.Add(ClusterSources.size());
		//	ClusterSources.Add(ClusterB);
		//}
	}

}

void ExpandClusters(
	const TVector< FCluster >& ClusterSources,
	const TVector< FClusterInstance>& ClusterInstances,
	TVector< FCluster >& OutClusters
)
{
	OutClusters.resize(ClusterInstances.size());

	const int32 NumInstances = (int32)ClusterInstances.size();
	for (int32 ci = 0; ci < NumInstances; ci++)
	{
		const FClusterInstance& CI = ClusterInstances[ci];
		const FCluster& ClusterSource = ClusterSources[CI.ClusterId];
		FCluster& oCluster = OutClusters[ci];
		oCluster = ClusterSource;

		if (CI.IsInstanced)
		{
			// Transform positions and normals
			const FMat4& Trans = CI.Transform;
			oCluster.Bounds = FBox();
			for (uint32 v = 0; v < ClusterSource.NumVerts; v++)
			{
				FFloat3& oPos = oCluster.GetPosition(v);
				Trans.TransformVect(oPos, ClusterSource.GetPosition(v));
				oCluster.Bounds.AddInternalPoint(oCluster.GetPosition(v));
				FFloat3& oNor = oCluster.GetNormal(v);
				Trans.RotateVect(oNor, ClusterSource.GetNormal(v));
			}
		}

		oCluster.GenerateGUID(ci);
		oCluster.LODError = CI.LODError;
		oCluster.LODBounds = CI.LODBounds;
		oCluster.SphereBounds = CI.SphereBounds;
		oCluster.GroupIndex = CI.GroupIndex;
		oCluster.GeneratingGroupIndex = CI.GeneratingGroupIndex;
		oCluster.EdgeLength = CI.EdgeLength;
	}
}

int32 QuantizePositions(TVector< FCluster >& Clusters, const FBox& MeshBounds)
{
	// Simple global quantization for EA
	const int32 MaxPositionQuantizedValue = (1 << NANITE_MAX_POSITION_QUANTIZATION_BITS) - 1;

	int32 PositionPrecision = TNumLimit<int32>::min();// Settings.PositionPrecision;
	//if (PositionPrecision == MIN_int32)
	{
		// Auto: Guess required precision from bounds at leaf level
		FFloat3 Extent = MeshBounds.GetExtent();
		const float MaxSize = TMath::Max3(Extent.X, Extent.Y, Extent.Z);

		// Heuristic: We want higher resolution if the mesh is denser.
		// Use geometric average of cluster size as a proxy for density.
		// Alternative interpretation: Bit precision is average of what is needed by the clusters.
		// For roughly uniformly sized clusters this gives results very similar to the old quantization code.
		float TotalLogSize = 0.0;
		int32 TotalNum = 0;
		for (const FCluster& Cluster : Clusters)
		{
			if (Cluster.MipLevel == 0)
			{
				float ExtentSize = Cluster.Bounds.GetExtent().GetLength();
				if (ExtentSize > 0.0)
				{
					TotalLogSize += TMath::Log2(ExtentSize);
					TotalNum++;
				}
			}
		}
		float AvgLogSize = TotalNum > 0 ? TotalLogSize / TotalNum : 0.0f;
		PositionPrecision = 7 - TMath::Round(AvgLogSize);

		// Clamp precision. The user now needs to explicitly opt-in to the lowest precision settings.
		// These settings are likely to cause issues and contribute little to disk size savings (~0.4% on test project),
		// so we shouldn't pick them automatically.
		// Example: A very low resolution road or building frame that needs little precision to look right in isolation,
		// but still requires fairly high precision in a scene because smaller meshes are placed on it or in it.
		const int32 AUTO_MIN_PRECISION = 4;	// 1/16cm
		PositionPrecision = TMath::Max(PositionPrecision, AUTO_MIN_PRECISION);
	}

	PositionPrecision = TMath::Clamp(PositionPrecision, NANITE_MIN_POSITION_PRECISION, NANITE_MAX_POSITION_PRECISION);

	float QuantizationScale = TMath::Pow(2.f, (float)PositionPrecision);

	// Make sure all clusters are encodable. A large enough cluster could hit the 21bpc limit. If it happens scale back until it fits.
	for (const FCluster& Cluster : Clusters)
	{
		const FBox& Bounds = Cluster.Bounds;

		int32 Iterations = 0;
		while (true)
		{
			float MinX = TMath::RoundToFloat(Bounds.Min.X * QuantizationScale);
			float MinY = TMath::RoundToFloat(Bounds.Min.Y * QuantizationScale);
			float MinZ = TMath::RoundToFloat(Bounds.Min.Z * QuantizationScale);

			float MaxX = TMath::RoundToFloat(Bounds.Max.X * QuantizationScale);
			float MaxY = TMath::RoundToFloat(Bounds.Max.Y * QuantizationScale);
			float MaxZ = TMath::RoundToFloat(Bounds.Max.Z * QuantizationScale);

			if (MinX >= FLT_INT_MIN && MinY >= FLT_INT_MIN && MinZ >= FLT_INT_MIN &&
				MaxX <= FLT_INT_MAX && MaxY <= FLT_INT_MAX && MaxZ <= FLT_INT_MAX &&
				((int64)MaxX - (int64)MinX) <= MaxPositionQuantizedValue && ((int64)MaxY - (int64)MinY) <= MaxPositionQuantizedValue && ((int64)MaxZ - (int64)MinZ) <= MaxPositionQuantizedValue)
			{
				break;
			}

			QuantizationScale *= 0.5f;
			PositionPrecision--;
			TI_ASSERT(PositionPrecision >= NANITE_MIN_POSITION_PRECISION);
			TI_ASSERT(++Iterations < 100);	// Endless loop?
		}
	}

	const float RcpQuantizationScale = 1.0f / QuantizationScale;

	const int32 NumClusters = (int32)Clusters.size();
	for (int32 ClusterIndex = 0; ClusterIndex < NumClusters; ClusterIndex++)
	{
		FCluster& Cluster = Clusters[ClusterIndex];

		const uint32 NumClusterVerts = Cluster.NumVerts;
		Cluster.QuantizedPositions.resize(NumClusterVerts);

		// Quantize positions
		const int32 MIN_int32 = TNumLimit<int32>::min();
		const int32 MAX_int32 = TNumLimit<int32>::max();
		FInt3 IntClusterMax = { MIN_int32,	MIN_int32, MIN_int32 };
		FInt3 IntClusterMin = { MAX_int32,	MAX_int32, MAX_int32 };

		for (uint32 i = 0; i < NumClusterVerts; i++)
		{
			const FFloat3 Position = Cluster.GetPosition(i);

			FInt3& IntPosition = Cluster.QuantizedPositions[i];
			float PosX = TMath::RoundToFloat(Position.X * QuantizationScale);
			float PosY = TMath::RoundToFloat(Position.Y * QuantizationScale);
			float PosZ = TMath::RoundToFloat(Position.Z * QuantizationScale);

			IntPosition = FInt3((int32)PosX, (int32)PosY, (int32)PosZ);

			IntClusterMax.X = TMath::Max(IntClusterMax.X, IntPosition.X);
			IntClusterMax.Y = TMath::Max(IntClusterMax.Y, IntPosition.Y);
			IntClusterMax.Z = TMath::Max(IntClusterMax.Z, IntPosition.Z);
			IntClusterMin.X = TMath::Min(IntClusterMin.X, IntPosition.X);
			IntClusterMin.Y = TMath::Min(IntClusterMin.Y, IntPosition.Y);
			IntClusterMin.Z = TMath::Min(IntClusterMin.Z, IntPosition.Z);
		}

		// Store in minimum number of bits
		const uint32 NumBitsX = TMath::CeilLogTwo(IntClusterMax.X - IntClusterMin.X + 1);
		const uint32 NumBitsY = TMath::CeilLogTwo(IntClusterMax.Y - IntClusterMin.Y + 1);
		const uint32 NumBitsZ = TMath::CeilLogTwo(IntClusterMax.Z - IntClusterMin.Z + 1);
		TI_ASSERT(NumBitsX <= NANITE_MAX_POSITION_QUANTIZATION_BITS);
		TI_ASSERT(NumBitsY <= NANITE_MAX_POSITION_QUANTIZATION_BITS);
		TI_ASSERT(NumBitsZ <= NANITE_MAX_POSITION_QUANTIZATION_BITS);

		for (uint32 i = 0; i < NumClusterVerts; i++)
		{
			FInt3& IntPosition = Cluster.QuantizedPositions[i];

			// Update float position with quantized data
			Cluster.GetPosition(i) = FFloat3(IntPosition.X * RcpQuantizationScale, IntPosition.Y * RcpQuantizationScale, IntPosition.Z * RcpQuantizationScale);

			IntPosition.X -= IntClusterMin.X;
			IntPosition.Y -= IntClusterMin.Y;
			IntPosition.Z -= IntClusterMin.Z;
			TI_ASSERT(IntPosition.X >= 0 && IntPosition.X < (1 << NumBitsX));
			TI_ASSERT(IntPosition.Y >= 0 && IntPosition.Y < (1 << NumBitsY));
			TI_ASSERT(IntPosition.Z >= 0 && IntPosition.Z < (1 << NumBitsZ));
		}


		// Update bounds
		Cluster.Bounds.Min = FFloat3(IntClusterMin.X * RcpQuantizationScale, IntClusterMin.Y * RcpQuantizationScale, IntClusterMin.Z * RcpQuantizationScale);
		Cluster.Bounds.Max = FFloat3(IntClusterMax.X * RcpQuantizationScale, IntClusterMax.Y * RcpQuantizationScale, IntClusterMax.Z * RcpQuantizationScale);

		Cluster.QuantizedPosBits = FInt3(NumBitsX, NumBitsY, NumBitsZ);
		Cluster.QuantizedPosStart = IntClusterMin;
		Cluster.QuantizedPosPrecision = PositionPrecision;
	}
	return PositionPrecision;
}

static uint32 CalcMaterialTableSize(const FCluster& InCluster)
{
	uint32 NumMaterials = (uint32)InCluster.MaterialRanges.size();
	return NumMaterials > 3 ? NumMaterials : 0;
}

void CalculateEncodingInfo(FEncodingInfo& Info, const FCluster& Cluster, bool bHasColors, uint32 NumTexCoords)
{
	const uint32 NumClusterVerts = Cluster.NumVerts;
	const uint32 NumClusterTris = Cluster.NumTris;

	memset(&Info, 0, sizeof(FEncodingInfo));

	// Write triangles indices. Indices are stored in a dense packed bitstream using ceil(log2(NumClusterVerices)) bits per index. The shaders implement unaligned bitstream reads to support this.
	uint32 BitsPerIndex = NumClusterVerts > 1 ? (TMath::FloorLog2(NumClusterVerts - 1) + 1) : 0;
	uint32 BitsPerTriangle = BitsPerIndex + 2 * 5;	// Base index + two 5-bit offsets
	Info.BitsPerIndex = BitsPerIndex;

	if ((Cluster.Flags & NANITE_CLUSTER_FLAG_NO_STRIPIFY) != 0)
	{
		BitsPerIndex = 8;
		BitsPerTriangle = 24;
		Info.BitsPerIndex = 8;
	}

	FPageSections& GpuSizes = Info.GpuSizes;
	GpuSizes.Cluster = sizeof(FPackedCluster);
	GpuSizes.MaterialTable = CalcMaterialTableSize(Cluster) * sizeof(uint32);
	GpuSizes.DecodeInfo = NumTexCoords * sizeof(FUVRange);
	GpuSizes.Index = (NumClusterTris * BitsPerTriangle + 31) / 32 * 4;

//#if NANITE_USE_UNCOMPRESSED_VERTEX_DATA
//	const uint32 AttribBytesPerVertex = (3 * sizeof(float) + sizeof(uint32) + NumTexCoords * 2 * sizeof(float));
//
//	Info.BitsPerAttribute = AttribBytesPerVertex * 8;
//	Info.ColorMin = FInt4(0, 0, 0, 0);
//	Info.ColorBits = FInt4(8, 8, 8, 8);
//	Info.ColorMode = NANITE_VERTEX_COLOR_MODE_VARIABLE;
//	Info.UVPrec = 0;
//
//	GpuSizes.Position = NumClusterVerts * 3 * sizeof(float);
//	GpuSizes.Attribute = NumClusterVerts * AttribBytesPerVertex;
//#else
	Info.BitsPerAttribute = 2 * NANITE_NORMAL_QUANTIZATION_BITS;

	TI_ASSERT(NumClusterVerts > 0);
	const bool bIsLeaf = (Cluster.GeneratingGroupIndex == INVALID_GROUP_INDEX);

	// Vertex colors
	Info.ColorMode = NANITE_VERTEX_COLOR_MODE_WHITE;
	Info.ColorMin = FInt4(255, 255, 255, 255);
	if (bHasColors)
	{
		FInt4 ColorMin = FInt4(255, 255, 255, 255);
		FInt4 ColorMax = FInt4(0, 0, 0, 0);
		for (uint32 i = 0; i < NumClusterVerts; i++)
		{
			SColor Color = Cluster.GetColor(i).ToSColor();
			ColorMin.X = TMath::Min(ColorMin.X, (int32)Color.R);
			ColorMin.Y = TMath::Min(ColorMin.Y, (int32)Color.G);
			ColorMin.Z = TMath::Min(ColorMin.Z, (int32)Color.B);
			ColorMin.W = TMath::Min(ColorMin.W, (int32)Color.A);
			ColorMax.X = TMath::Max(ColorMax.X, (int32)Color.R);
			ColorMax.Y = TMath::Max(ColorMax.Y, (int32)Color.G);
			ColorMax.Z = TMath::Max(ColorMax.Z, (int32)Color.B);
			ColorMax.W = TMath::Max(ColorMax.W, (int32)Color.A);
		}

		const FInt4 ColorDelta = ColorMax - ColorMin;
		const int32 R_Bits = TMath::CeilLogTwo(ColorDelta.X + 1);
		const int32 G_Bits = TMath::CeilLogTwo(ColorDelta.Y + 1);
		const int32 B_Bits = TMath::CeilLogTwo(ColorDelta.Z + 1);
		const int32 A_Bits = TMath::CeilLogTwo(ColorDelta.W + 1);

		uint32 NumColorBits = R_Bits + G_Bits + B_Bits + A_Bits;
		Info.BitsPerAttribute += NumColorBits;
		Info.ColorMin = ColorMin;
		Info.ColorBits = FInt4(R_Bits, G_Bits, B_Bits, A_Bits);
		if (NumColorBits > 0)
		{
			Info.ColorMode = NANITE_VERTEX_COLOR_MODE_VARIABLE;
		}
		else
		{
			if (ColorMin.X == 255 && ColorMin.Y == 255 && ColorMin.Z == 255 && ColorMin.W == 255)
				Info.ColorMode = NANITE_VERTEX_COLOR_MODE_WHITE;
			else
				Info.ColorMode = NANITE_VERTEX_COLOR_MODE_CONSTANT;
		}
	}

	for (uint32 UVIndex = 0; UVIndex < NumTexCoords; UVIndex++)
	{
		FUVRange& UVRange = Info.UVRanges[UVIndex];
		// Block compress texture coordinates
		// Texture coordinates are stored relative to the clusters min/max UV coordinates.
		// UV seams result in very large sparse bounding rectangles. To mitigate this the largest gap in U and V of the bounding rectangle are excluded from the coding space.
		// Decoding this is very simple: UV += (UV >= GapStart) ? GapRange : 0;

		// Generate sorted U and V arrays.
		TVector<float> UValues;
		TVector<float> VValues;
		UValues.resize(NumClusterVerts);
		VValues.resize(NumClusterVerts);
		for (uint32 i = 0; i < NumClusterVerts; i++)
		{
			const FFloat2& UV = Cluster.GetUVs(i)[UVIndex];
			UValues[i] = UV.X;
			VValues[i] = UV.Y;
		}

		TSort(UValues.begin(), UValues.end());
		TSort(VValues.begin(), VValues.end());

		// Find largest gap between sorted UVs
		FFloat2 LargestGapStart = FFloat2(UValues[0], VValues[0]);
		FFloat2 LargestGapEnd = FFloat2(UValues[0], VValues[0]);
		for (uint32 i = 0; i < NumClusterVerts - 1; i++)
		{
			if (UValues[i + 1] - UValues[i] > LargestGapEnd.X - LargestGapStart.X)
			{
				LargestGapStart.X = UValues[i];
				LargestGapEnd.X = UValues[i + 1];
			}
			if (VValues[i + 1] - VValues[i] > LargestGapEnd.Y - LargestGapStart.Y)
			{
				LargestGapStart.Y = VValues[i];
				LargestGapEnd.Y = VValues[i + 1];
			}
		}

		const FFloat2 UVMin = FFloat2(UValues[0], VValues[0]);
		const FFloat2 UVMax = FFloat2(UValues[NumClusterVerts - 1], VValues[NumClusterVerts - 1]);
		const int32 MaxTexCoordQuantizedValue = (1 << NANITE_MAX_TEXCOORD_QUANTIZATION_BITS) - 1;

		int TexCoordPrecision = 14;

		{
			float QuantizationScale = TMath::Pow(2.f, (float)TexCoordPrecision);

			int32 Iterations = 0;
			while (true)
			{
				float MinU = TMath::RoundToFloat(UVMin.X * QuantizationScale);
				float MinV = TMath::RoundToFloat(UVMin.Y * QuantizationScale);

				float MaxU = TMath::RoundToFloat(UVMax.X * QuantizationScale);
				float MaxV = TMath::RoundToFloat(UVMax.Y * QuantizationScale);

				if (MinU >= FLT_INT_MIN && MinV >= FLT_INT_MIN &&
					MaxU <= FLT_INT_MAX && MaxV <= FLT_INT_MAX)
				{
					float GapStartU = TMath::RoundToFloat(LargestGapStart.X * QuantizationScale);
					float GapStartV = TMath::RoundToFloat(LargestGapStart.Y * QuantizationScale);

					float GapEndU = TMath::RoundToFloat(LargestGapEnd.X * QuantizationScale);
					float GapEndV = TMath::RoundToFloat(LargestGapEnd.Y * QuantizationScale);

					// GapStartU
					const int64 IMinU = (int64)MinU;
					const int64 IMinV = (int64)MinV;
					const int64 IMaxU = (int64)MaxU;
					const int64 IMaxV = (int64)MaxV;
					const int64 IGapStartU = (int64)GapStartU;
					const int64 IGapStartV = (int64)GapStartV;
					const int64 IGapEndU = (int64)GapEndU;
					const int64 IGapEndV = (int64)GapEndV;

					int64 MaxDeltaU = IMaxU - IMinU - (IMaxU > IGapStartU ? (IGapEndU - IGapStartU - 1) : 0);
					int64 MaxDeltaV = IMaxV - IMinV - (IMaxV > IGapStartV ? (IGapEndV - IGapStartV - 1) : 0);
					if (MaxDeltaU <= MaxTexCoordQuantizedValue && MaxDeltaV <= MaxTexCoordQuantizedValue)
					{
						uint32 TexCoordBitsU = TMath::CeilLogTwo((int32)MaxDeltaU + 1);
						uint32 TexCoordBitsV = TMath::CeilLogTwo((int32)MaxDeltaV + 1);
						TI_ASSERT(TexCoordBitsU <= NANITE_MAX_TEXCOORD_QUANTIZATION_BITS);
						TI_ASSERT(TexCoordBitsV <= NANITE_MAX_TEXCOORD_QUANTIZATION_BITS);
						Info.UVPrec |= ((TexCoordBitsV << 4) | TexCoordBitsU) << (UVIndex * 8);
						Info.BitsPerAttribute += TexCoordBitsU + TexCoordBitsV;

						UVRange.Min = FInt2((int32)IMinU, (int32)IMinV);
						UVRange.GapStart = FInt2((int32)(IGapStartU - MinU), (int32)(IGapStartV - MinV));
						UVRange.GapLength = FInt2((int32)(IGapEndU - IGapStartU - 1), (int32)(IGapEndV - IGapStartV - 1));
						UVRange.Precision = TexCoordPrecision;
						UVRange.Pad = 0;
						break;
					}
				}
				QuantizationScale *= 0.5f;
				TexCoordPrecision--;
				TI_ASSERT(++Iterations < 256);	// Endless loop?
			}
		}
	}

	// Calc GpuSizes.Position outside
	//const uint32 PositionBitsPerVertex = Cluster.QuantizedPosBits.X + Cluster.QuantizedPosBits.Y + Cluster.QuantizedPosBits.Z;
	//GpuSizes.Position = (NumClusterVerts * PositionBitsPerVertex + 31) / 32 * 4;
	GpuSizes.Attribute = (NumClusterVerts * Info.BitsPerAttribute + 31) / 32 * 4;
//#endif
}

void CalcEncodingInfoFromInstances(
	TVector<FEncodingInfo>& EncodingInfos,
	const TVector<FCluster>& ClusterSources,
	const TVector<FCluster>& ClusterExpanded,
	const TVector<FClusterInstance>& ClusterInstances,
	bool bHasColors, uint32 NumTexCoords)
{
	TVector<FEncodingInfo> SourceEncodingInfos;

	const uint32 NumClusterSources = (uint32)ClusterSources.size();
	SourceEncodingInfos.resize(NumClusterSources);

	for (uint32 i = 0; i < NumClusterSources; i++)
	{
		CalculateEncodingInfo(SourceEncodingInfos[i], ClusterSources[i], bHasColors, NumTexCoords);
	}

	const uint32 NumClusterInstances = (uint32)ClusterInstances.size();
	EncodingInfos.resize(NumClusterInstances);
	TI_ASSERT(NumClusterInstances == ClusterExpanded.size());
	for (uint32 i = 0; i < NumClusterInstances; i++)
	{
		const FClusterInstance& CI = ClusterInstances[i];
		EncodingInfos[i] = SourceEncodingInfos[CI.ClusterId];
		// Recalc GpuSizes.Position from expanded clusters
		const FCluster& CP = ClusterExpanded[i];
		const uint32 PositionBitsPerVertex = CP.QuantizedPosBits.X + CP.QuantizedPosBits.Y + CP.QuantizedPosBits.Z;
		EncodingInfos[i].GpuSizes.Position = (CP.NumVerts * PositionBitsPerVertex + 31) / 32 * 4;
	}
}

// Generate a permutation of cluster groups that is sorted first by mip level and then by Morton order x, y and z.
// Sorting by mip level first ensure that there can be no cyclic dependencies between formed pages.
static TVector<uint32> CalculateClusterGroupPermutation(const TVector< FClusterGroup >& ClusterGroups)
{
	struct FClusterGroupSortEntry {
		int32	MipLevel;
		uint32	MortonXYZ;
		uint32	OldIndex;
	};

	const uint32 NumClusterGroups = (uint32)ClusterGroups.size();
	TVector< FClusterGroupSortEntry > ClusterGroupSortEntries;
	ClusterGroupSortEntries.resize(NumClusterGroups);

	FFloat3 MinCenter = FFloat3(FLT_MAX, FLT_MAX, FLT_MAX);
	FFloat3 MaxCenter = FFloat3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
	for (const FClusterGroup& ClusterGroup : ClusterGroups)
	{
		const FFloat3& Center = ClusterGroup.LODBounds.Center;
		MinCenter = FFloat3::Min(MinCenter, Center);
		MaxCenter = FFloat3::Max(MaxCenter, Center);
	}

	for (uint32 i = 0; i < NumClusterGroups; i++)
	{
		const FClusterGroup& ClusterGroup = ClusterGroups[i];
		FClusterGroupSortEntry& SortEntry = ClusterGroupSortEntries[i];
		const FFloat3& Center = ClusterGroup.LODBounds.Center;
		const FFloat3 ScaledCenter = (Center - MinCenter) / (MaxCenter - MinCenter) * 1023.0f + 0.5f;
		uint32 X = TMath::Clamp((int32)ScaledCenter.X, 0, 1023);
		uint32 Y = TMath::Clamp((int32)ScaledCenter.Y, 0, 1023);
		uint32 Z = TMath::Clamp((int32)ScaledCenter.Z, 0, 1023);

		SortEntry.MipLevel = ClusterGroup.MipLevel;
		SortEntry.MortonXYZ = (TMath::MortonCode3(Z) << 2) | (TMath::MortonCode3(Y) << 1) | TMath::MortonCode3(X);
		SortEntry.OldIndex = i;
	}

	TSort(ClusterGroupSortEntries.begin(), ClusterGroupSortEntries.end(), 
		[](const FClusterGroupSortEntry& A, const FClusterGroupSortEntry& B) {
			if (A.MipLevel != B.MipLevel)
				return A.MipLevel > B.MipLevel;
			return A.MortonXYZ < B.MortonXYZ;
		});

	TVector<uint32> Permutation;
	Permutation.resize(NumClusterGroups);
	for (uint32 i = 0; i < NumClusterGroups; i++)
		Permutation[i] = ClusterGroupSortEntries[i].OldIndex;
	return Permutation;
}

static void SortGroupClusters(TVector<FClusterGroup>& ClusterGroups, const TVector<FCluster>& Clusters)
{
	for (FClusterGroup& Group : ClusterGroups)
	{
		FFloat3 SortDirection = FFloat3(1.0f, 1.0f, 1.0f);
		TSort(Group.Children.begin(), Group.Children.end(), [&Clusters, SortDirection](uint32 ClusterIndexA, uint32 ClusterIndexB)
			{
				const FCluster& ClusterA = Clusters[ClusterIndexA];
				const FCluster& ClusterB = Clusters[ClusterIndexB];

				float DotA = ClusterA.SphereBounds.Center.Dot(SortDirection);
				float DotB = ClusterB.SphereBounds.Center.Dot(SortDirection);
				return DotA < DotB;
			});
	}
}

void AssignClusterToPages(
	TVector< FClusterGroup >& ClusterGroups,
	TVector< FCluster >& Clusters,
	const TVector< FEncodingInfo >& EncodingInfos,
	TVector<FPage>& Pages,
	TVector<FClusterGroupPart>& Parts,
	const uint32 MaxRootPages
)
{
	TI_ASSERT(Pages.size() == 0);
	TI_ASSERT(Parts.size() == 0);

	const uint32 NumClusterGroups = (uint32)ClusterGroups.size();
	Pages.push_back(FPage());

	SortGroupClusters(ClusterGroups, Clusters);
	TVector<uint32> ClusterGroupPermutation = CalculateClusterGroupPermutation(ClusterGroups);

	for (uint32 i = 0; i < NumClusterGroups; i++)
	{
		// Pick best next group			// TODO
		uint32 GroupIndex = ClusterGroupPermutation[i];
		FClusterGroup& Group = ClusterGroups[GroupIndex];
		if (Group.bTrimmed)
			continue;

		uint32 GroupStartPage = INVALID_PAGE_INDEX;

		for (uint32 ClusterIndex : Group.Children)
		{
			// Pick best next cluster		// TODO
			FCluster& Cluster = Clusters[ClusterIndex];
			const FEncodingInfo& EncodingInfo = EncodingInfos[ClusterIndex];

			// Add to page
			FPage* Page = &Pages.front();
			bool bRootPage = (Pages.size() - 1u) < MaxRootPages;
			if (Page->GpuSizes.GetTotal() + EncodingInfo.GpuSizes.GetTotal() > (bRootPage ? NANITE_ROOT_PAGE_GPU_SIZE : NANITE_STREAMING_PAGE_GPU_SIZE) || Page->NumClusters + 1 > NANITE_MAX_CLUSTERS_PER_PAGE)
			{
				// Page is full. Need to start a new one
				Pages.push_back(FPage());
				Page = &Pages.front();
			}

			// Start a new part?
			if (Page->PartsNum == 0 || Parts[Page->PartsStartIndex + Page->PartsNum - 1].GroupIndex != GroupIndex)
			{
				if (Page->PartsNum == 0)
				{
					Page->PartsStartIndex = (uint32)Parts.size();
				}
				Page->PartsNum++;

				//FClusterGroupPart& Part = Parts.AddDefaulted_GetRef();
				Parts.push_back(FClusterGroupPart());
				FClusterGroupPart& Part = Parts.back();
				Part.GroupIndex = GroupIndex;
			}

			// Add cluster to page
			uint32 PageIndex = (uint32)Pages.size() - 1;
			uint32 PartIndex = (uint32)Parts.size() - 1;

			FClusterGroupPart& Part = Parts.back();
			if (Part.Clusters.size() == 0)
			{
				Part.PageClusterOffset = Page->NumClusters;
				Part.PageIndex = PageIndex;
			}
			Part.Clusters.push_back(ClusterIndex);
			TI_ASSERT(Part.Clusters.size() <= NANITE_MAX_CLUSTERS_PER_GROUP);

			Cluster.GroupPartIndex = PartIndex;

			if (GroupStartPage == INVALID_PAGE_INDEX)
			{
				GroupStartPage = PageIndex;
			}

			Page->GpuSizes += EncodingInfo.GpuSizes;
			Page->NumClusters++;
		}

		TI_ASSERT((uint32)Pages.size() >= GroupStartPage);
		Group.PageIndexStart = GroupStartPage;
		Group.PageIndexNum = (uint32)Pages.size() - GroupStartPage;
		TI_ASSERT(Group.PageIndexNum >= 1);
		TI_ASSERT(Group.PageIndexNum <= NANITE_MAX_GROUP_PARTS_MASK);
	}

	// Recalculate bounds for group parts
	for (FClusterGroupPart& Part : Parts)
	{
		TI_ASSERT(Part.Clusters.size() <= NANITE_MAX_CLUSTERS_PER_GROUP);
		TI_ASSERT(Part.PageIndex < (uint32)Pages.size());

		FBox Bounds;
		for (uint32 ClusterIndex : Part.Clusters)
		{
			Bounds.AddInternalBox(Clusters[ClusterIndex].Bounds);
		}
		Part.Bounds = Bounds;
	}
}

struct FIntermediateNode
{
	uint32 PartIndex = TNumLimit<uint32>::max();
	uint32 MipLevel = TNumLimit<uint32>::max();
	bool bLeaf = false;

	FBox Bound;
	TVector< uint32 > Children;
};

struct FHierarchyNode
{
	FSpheref LODBounds[NANITE_MAX_BVH_NODE_FANOUT];
	FBox Bounds[NANITE_MAX_BVH_NODE_FANOUT];
	float MinLODErrors[NANITE_MAX_BVH_NODE_FANOUT];
	float MaxParentLODErrors[NANITE_MAX_BVH_NODE_FANOUT];
	uint32 ChildrenStartIndex[NANITE_MAX_BVH_NODE_FANOUT];
	uint32 NumChildren[NANITE_MAX_BVH_NODE_FANOUT];
	uint32 ClusterGroupPartIndex[NANITE_MAX_BVH_NODE_FANOUT];

	FHierarchyNode()
	{
		memset(this, 0, sizeof(FHierarchyNode));
	}
};

static void RemoveRootPagesFromRange(uint32& StartPage, uint32& NumPages, const uint32 NumResourceRootPages)
{
	if (StartPage < NumResourceRootPages)
	{
		NumPages = (uint32)TMath::Max((int32)NumPages - (int32)(NumResourceRootPages - StartPage), 0);
		StartPage = NumResourceRootPages;
	}

	if (NumPages == 0)
	{
		StartPage = 0;
	}
}

static void RemovePageFromRange(uint32& StartPage, uint32& NumPages, const uint32 PageIndex)
{
	if (NumPages > 0)
	{
		if (StartPage == PageIndex)
		{
			StartPage++;
			NumPages--;
		}
		else if (StartPage + NumPages - 1 == PageIndex)
		{
			NumPages--;
		}
	}

	if (NumPages == 0)
	{
		StartPage = 0;
	}
}

static void PackHierarchyNode(FPackedHierarchyNode& OutNode, const FHierarchyNode& InNode, const TVector<FClusterGroup>& Groups, const TVector<FClusterGroupPart>& GroupParts, const uint32 NumResourceRootPages)
{
	static_assert(NANITE_MAX_RESOURCE_PAGES_BITS + NANITE_MAX_CLUSTERS_PER_GROUP_BITS + NANITE_MAX_GROUP_PARTS_BITS <= 32, "");
	for (uint32 i = 0; i < NANITE_MAX_BVH_NODE_FANOUT; i++)
	{
		OutNode.LODBounds[i] = FFloat4(InNode.LODBounds[i].Center, InNode.LODBounds[i].W);

		const FBox& Bounds = InNode.Bounds[i];
		OutNode.Misc0[i].BoxBoundsCenter = Bounds.GetCenter();
		OutNode.Misc1[i].BoxBoundsExtent = Bounds.GetExtent();

		TI_ASSERT(InNode.NumChildren[i] <= NANITE_MAX_CLUSTERS_PER_GROUP);
		OutNode.Misc0[i].MinLODError_MaxParentLODError = float16(InNode.MinLODErrors[i]).data() | (float16(InNode.MaxParentLODErrors[i]).data() << 16);
		OutNode.Misc1[i].ChildStartReference = InNode.ChildrenStartIndex[i];

		uint32 ResourcePageIndex_NumPages_GroupPartSize = 0;
		if (InNode.NumChildren[i] > 0)
		{
			if (InNode.ClusterGroupPartIndex[i] != INVALID_PART_INDEX)
			{
				// Leaf node
				const FClusterGroup& Group = Groups[GroupParts[InNode.ClusterGroupPartIndex[i]].GroupIndex];
				uint32 GroupPartSize = InNode.NumChildren[i];

				// If group spans multiple pages, request all of them, except the root pages
				uint32 PageIndexStart = Group.PageIndexStart;
				uint32 PageIndexNum = Group.PageIndexNum;
				RemoveRootPagesFromRange(PageIndexStart, PageIndexNum, NumResourceRootPages);
				ResourcePageIndex_NumPages_GroupPartSize = (PageIndexStart << (NANITE_MAX_CLUSTERS_PER_GROUP_BITS + NANITE_MAX_GROUP_PARTS_BITS)) | (PageIndexNum << NANITE_MAX_CLUSTERS_PER_GROUP_BITS) | GroupPartSize;
			}
			else
			{
				// Hierarchy node. No resource page or group size.
				ResourcePageIndex_NumPages_GroupPartSize = 0xFFFFFFFFu;
			}
		}
		OutNode.Misc2[i].ResourcePageIndex_NumPages_GroupPartSize = ResourcePageIndex_NumPages_GroupPartSize;
	}
}

static float BVH_Cost(const TVector<FIntermediateNode>& Nodes, TArrayView<uint32> NodeIndices)
{
	FBox Bound;
	for (uint32 NodeIndex : NodeIndices)
	{
		Bound.AddInternalBox(Nodes[NodeIndex].Bound);
	}
	return Bound.GetSurfaceArea();
}

static void BVH_SortNodes(const TVector<FIntermediateNode>& Nodes, TArrayView<uint32> NodeIndices, const TVector<uint32>& ChildSizes)
{
	// Perform NANITE_MAX_BVH_NODE_FANOUT_BITS binary splits
	for (uint32 Level = 0; Level < NANITE_MAX_BVH_NODE_FANOUT_BITS; Level++)
	{
		const uint32 NumBuckets = 1 << Level;
		const uint32 NumChildrenPerBucket = NANITE_MAX_BVH_NODE_FANOUT >> Level;
		const uint32 NumChildrenPerBucketHalf = NumChildrenPerBucket >> 1;

		uint32 BucketStartIndex = 0;
		for (uint32 BucketIndex = 0; BucketIndex < NumBuckets; BucketIndex++)
		{
			const uint32 FirstChild = NumChildrenPerBucket * BucketIndex;

			uint32 Sizes[2] = {};
			for (uint32 i = 0; i < NumChildrenPerBucketHalf; i++)
			{
				Sizes[0] += ChildSizes[FirstChild + i];
				Sizes[1] += ChildSizes[FirstChild + i + NumChildrenPerBucketHalf];
			}
			TArrayView<uint32> NodeIndices01 = NodeIndices.Slice(BucketStartIndex, Sizes[0] + Sizes[1]);
			TArrayView<uint32> NodeIndices0 = NodeIndices.Slice(BucketStartIndex, Sizes[0]);
			TArrayView<uint32> NodeIndices1 = NodeIndices.Slice(BucketStartIndex + Sizes[0], Sizes[1]);

			BucketStartIndex += Sizes[0] + Sizes[1];

			auto SortByAxis = [&](uint32 AxisIndex)
			{
				if (AxisIndex == 0)
					NodeIndices01.Sort([&Nodes](uint32 A, uint32 B) { return Nodes[A].Bound.GetCenter().X < Nodes[B].Bound.GetCenter().X; });
				else if (AxisIndex == 1)
					NodeIndices01.Sort([&Nodes](uint32 A, uint32 B) { return Nodes[A].Bound.GetCenter().Y < Nodes[B].Bound.GetCenter().Y; });
				else if (AxisIndex == 2)
					NodeIndices01.Sort([&Nodes](uint32 A, uint32 B) { return Nodes[A].Bound.GetCenter().Z < Nodes[B].Bound.GetCenter().Z; });
				else
				{
					TI_ASSERT(false);
				}
			};

			float BestCost = MAX_flt;
			uint32 BestAxisIndex = 0;

			// Try sorting along different axes and pick the best one
			const uint32 NumAxes = 3;
			for (uint32 AxisIndex = 0; AxisIndex < NumAxes; AxisIndex++)
			{
				SortByAxis(AxisIndex);

				float Cost = BVH_Cost(Nodes, NodeIndices0) + BVH_Cost(Nodes, NodeIndices1);
				if (Cost < BestCost)
				{
					BestCost = Cost;
					BestAxisIndex = AxisIndex;
				}
			}

			// Resort if we the best one wasn't the last one
			if (BestAxisIndex != NumAxes - 1)
			{
				SortByAxis(BestAxisIndex);
			}
		}
	}
}

static uint32 BuildHierarchyRecursive(TVector<FHierarchyNode>& HierarchyNodes, const TVector<FIntermediateNode>& Nodes, const TVector<FClusterGroup>& Groups, TVector<FClusterGroupPart>& Parts, uint32 CurrentNodeIndex)
{
	const FIntermediateNode& INode = Nodes[CurrentNodeIndex];
	TI_ASSERT(INode.PartIndex == MAX_uint32);
	TI_ASSERT(!INode.bLeaf);

	uint32 HNodeIndex = (uint32)HierarchyNodes.size();
	HierarchyNodes.push_back(FHierarchyNode());

	uint32 NumChildren = INode.Children.size();
	TI_ASSERT(NumChildren > 0 && NumChildren <= NANITE_MAX_BVH_NODE_FANOUT);
	for (uint32 ChildIndex = 0; ChildIndex < NumChildren; ChildIndex++)
	{
		uint32 ChildNodeIndex = INode.Children[ChildIndex];
		const FIntermediateNode& ChildNode = Nodes[ChildNodeIndex];
		if (ChildNode.bLeaf)
		{
			// Cluster Group
			TI_ASSERT(ChildNode.bLeaf);
			FClusterGroupPart& Part = Parts[ChildNode.PartIndex];
			const FClusterGroup& Group = Groups[Part.GroupIndex];

			FHierarchyNode& HNode = HierarchyNodes[HNodeIndex];
			HNode.Bounds[ChildIndex] = Part.Bounds;
			HNode.LODBounds[ChildIndex] = Group.LODBounds;
			HNode.MinLODErrors[ChildIndex] = Group.MinLODError;
			HNode.MaxParentLODErrors[ChildIndex] = Group.MaxParentLODError;
			HNode.ChildrenStartIndex[ChildIndex] = 0xFFFFFFFFu;
			HNode.NumChildren[ChildIndex] = Part.Clusters.size();
			HNode.ClusterGroupPartIndex[ChildIndex] = ChildNode.PartIndex;

			TI_ASSERT(HNode.NumChildren[ChildIndex] <= NANITE_MAX_CLUSTERS_PER_GROUP);
			Part.HierarchyNodeIndex = HNodeIndex;
			Part.HierarchyChildIndex = ChildIndex;
		}
		else
		{

			// Hierarchy node
			uint32 ChildHierarchyNodeIndex = BuildHierarchyRecursive(HierarchyNodes, Nodes, Groups, Parts, ChildNodeIndex);

			const FHierarchyNode& ChildHNode = HierarchyNodes[ChildHierarchyNodeIndex];

			FBox Bounds;
			TVector< FSpheref, TInlineAllocator<NANITE_MAX_BVH_NODE_FANOUT> > LODBoundSpheres;
			float MinLODError = MAX_flt;
			float MaxParentLODError = 0.0f;
			for (uint32 GrandChildIndex = 0; GrandChildIndex < NANITE_MAX_BVH_NODE_FANOUT && ChildHNode.NumChildren[GrandChildIndex] != 0; GrandChildIndex++)
			{
				Bounds.AddInternalBox(ChildHNode.Bounds[GrandChildIndex]);
				LODBoundSpheres.Add(ChildHNode.LODBounds[GrandChildIndex]);
				MinLODError = TMath::Min(MinLODError, ChildHNode.MinLODErrors[GrandChildIndex]);
				MaxParentLODError = TMath::Max(MaxParentLODError, ChildHNode.MaxParentLODErrors[GrandChildIndex]);
			}

			FSpheref LODBounds = FSpheref(LODBoundSpheres.GetData(), LODBoundSpheres.size());

			FHierarchyNode& HNode = HierarchyNodes[HNodeIndex];
			HNode.Bounds[ChildIndex] = Bounds;
			HNode.LODBounds[ChildIndex] = LODBounds;
			HNode.MinLODErrors[ChildIndex] = MinLODError;
			HNode.MaxParentLODErrors[ChildIndex] = MaxParentLODError;
			HNode.ChildrenStartIndex[ChildIndex] = ChildHierarchyNodeIndex;
			HNode.NumChildren[ChildIndex] = NANITE_MAX_CLUSTERS_PER_GROUP;
			HNode.ClusterGroupPartIndex[ChildIndex] = INVALID_GROUP_INDEX;
		}
	}

	return HNodeIndex;
}

// Build hierarchy using a top-down splitting approach.
// WIP:	So far it just focuses on minimizing worst-case tree depth/latency.
//		It does this by building a complete tree with at most one partially filled level.
//		At most one node is partially filled.
//TODO:	Experiment with sweeping, even if it results in more total nodes and/or makes some paths slightly longer.
static uint32 BuildHierarchyTopDown(TVector<FIntermediateNode>& Nodes, TArrayView<uint32> NodeIndices, bool bSort)
{
	const uint32 N = NodeIndices.size();
	if (N == 1)
	{
		return NodeIndices[0];
	}

	const uint32 NewRootIndex = Nodes.size();
	Nodes.AddDefaulted_GetRef();

	if (N <= NANITE_MAX_BVH_NODE_FANOUT)
	{
		Nodes[NewRootIndex].Children = NodeIndices;
		return NewRootIndex;
	}

	// Where does the last (incomplete) level start
	uint32 TopSize = NANITE_MAX_BVH_NODE_FANOUT;
	while (TopSize * NANITE_MAX_BVH_NODE_FANOUT <= N)
	{
		TopSize *= NANITE_MAX_BVH_NODE_FANOUT;
	}

	const uint32 LargeChildSize = TopSize;
	const uint32 SmallChildSize = TopSize / NANITE_MAX_BVH_NODE_FANOUT;
	const uint32 MaxExcessPerChild = LargeChildSize - SmallChildSize;

	TVector<uint32> ChildSizes;
	ChildSizes.resize(NANITE_MAX_BVH_NODE_FANOUT);

	uint32 Excess = N - TopSize;
	for (int32 i = NANITE_MAX_BVH_NODE_FANOUT - 1; i >= 0; i--)
	{
		const uint32 ChildExcess = TMath::Min(Excess, MaxExcessPerChild);
		ChildSizes[i] = SmallChildSize + ChildExcess;
		Excess -= ChildExcess;
	}
	TI_ASSERT(Excess == 0);

	if (bSort)
	{
		BVH_SortNodes(Nodes, NodeIndices, ChildSizes);
	}

	uint32 Offset = 0;
	for (uint32 i = 0; i < NANITE_MAX_BVH_NODE_FANOUT; i++)
	{
		uint32 ChildSize = ChildSizes[i];
		uint32 NodeIndex = BuildHierarchyTopDown(Nodes, NodeIndices.Slice(Offset, ChildSize), bSort);	// Needs to be separated from next statement with sequence point to order access to Nodes array.
		Nodes[NewRootIndex].Children.Add(NodeIndex);
		Offset += ChildSize;
	}

	return NewRootIndex;
}

void BuildHierarchies(const TVector<FClusterGroup>& Groups, TVector<FClusterGroupPart>& Parts, uint32 NumMeshes)
{
	TVector<TVector<uint32>> PartsByMesh;
	PartsByMesh.resize(NumMeshes);

	// Assign group parts to the meshes they belong to
	const uint32 NumTotalParts = Parts.size();
	for (uint32 PartIndex = 0; PartIndex < NumTotalParts; PartIndex++)
	{
		FClusterGroupPart& Part = Parts[PartIndex];
		PartsByMesh[Groups[Part.GroupIndex].MeshIndex].Add(PartIndex);
	}

	for (uint32 MeshIndex = 0; MeshIndex < NumMeshes; MeshIndex++)
	{
		const TVector<uint32>& PartIndices = PartsByMesh[MeshIndex];
		const uint32 NumParts = PartIndices.size();

		int32 MaxMipLevel = 0;
		for (uint32 i = 0; i < NumParts; i++)
		{
			MaxMipLevel = TMath::Max(MaxMipLevel, Groups[Parts[PartIndices[i]].GroupIndex].MipLevel);
		}

		TVector< FIntermediateNode >	Nodes;
		Nodes.resize(NumParts);

		// Build leaf nodes for each LOD level of the mesh
		TVector<TVector<uint32>> NodesByMip;
		NodesByMip.resize(MaxMipLevel + 1);
		for (uint32 i = 0; i < NumParts; i++)
		{
			const uint32 PartIndex = PartIndices[i];
			const FClusterGroupPart& Part = Parts[PartIndex];
			const FClusterGroup& Group = Groups[Part.GroupIndex];

			const int32 MipLevel = Group.MipLevel;
			FIntermediateNode& Node = Nodes[i];
			Node.Bound = Part.Bounds;
			Node.PartIndex = PartIndex;
			Node.MipLevel = Group.MipLevel;
			Node.bLeaf = true;
			NodesByMip[Group.MipLevel].Add(i);
		}


		uint32 RootIndex = 0;
		if (Nodes.size() == 1)
		{
			// Just a single leaf.
			// Needs to be special-cased as root should always be an inner node.
			FIntermediateNode& Node = Nodes.AddDefaulted_GetRef();
			Node.Children.Add(0);
			Node.Bound = Nodes[0].Bound;
			RootIndex = 1;
		}
		else
		{
			// Build hierarchy:
			// Nanite meshes contain cluster data for many levels of detail. Clusters from different levels
			// of detail can vary wildly in size, which can already be challenge for building a good hierarchy. 
			// Apart from the visibility bounds, the hierarchy also tracks conservative LOD error metrics for the child nodes.
			// The runtime traversal descends into children as long as they are visible and the conservative LOD error is not
			// more detailed than what we are looking for. We have to be very careful when mixing clusters from different LODs
			// as less detailed clusters can easily end up bloating both bounds and error metrics.

			// We have experimented with a bunch of mixed LOD approached, but currently, it seems, building separate hierarchies
			// for each LOD level and then building a hierarchy of those hierarchies gives the best and most predictable results.

			// TODO: The roots of these hierarchies all share the same visibility and LOD bounds, or at least close enough that we could
			//       make a shared conservative bound without losing much. This makes a lot of the work around the root node fairly
			//       redundant. Perhaps we should consider evaluating a shared root during instance cull instead and enable/disable
			//       the per-level hierarchies based on 1D range tests for LOD error.

			TVector<uint32> LevelRoots;
			for (int32 MipLevel = 0; MipLevel <= MaxMipLevel; MipLevel++)
			{
				if (NodesByMip[MipLevel].size() > 0)
				{
					// Build a hierarchy for the mip level
					uint32 NodeIndex = BuildHierarchyTopDown(Nodes, NodesByMip[MipLevel], true);

					if (Nodes[NodeIndex].bLeaf || Nodes[NodeIndex].Children.size() == NANITE_MAX_BVH_NODE_FANOUT)
					{
						// Leaf or filled node. Just add it.
						LevelRoots.Add(NodeIndex);
					}
					else
					{
						// Incomplete node. Discard the code and add the children as roots instead.
						LevelRoots.Append(Nodes[NodeIndex].Children);
					}
				}
			}
			// Build top hierarchy. A hierarchy of MIP hierarchies.
			RootIndex = BuildHierarchyTopDown(Nodes, LevelRoots, false);
		}

		TI_ASSERT(Nodes.size() > 0);

#if BVH_BUILD_WRITE_GRAPHVIZ
		WriteDotGraph(Nodes);
#endif

		TVector< FHierarchyNode > HierarchyNodes;
		BuildHierarchyRecursive(HierarchyNodes, Nodes, Groups, Parts, RootIndex);

		// Convert hierarchy to packed format
		const uint32 NumHierarchyNodes = HierarchyNodes.size();
		const uint32 PackedBaseIndex = Resources.HierarchyNodes.size();
		Resources.HierarchyRootOffsets.Add(PackedBaseIndex);
		Resources.HierarchyNodes.AddDefaulted(NumHierarchyNodes);
		for (uint32 i = 0; i < NumHierarchyNodes; i++)
		{
			PackHierarchyNode(Resources.HierarchyNodes[PackedBaseIndex + i], HierarchyNodes[i], Groups, Parts, Resources.NumRootPages);
		}
	}
}

void WritePages()
{}

void Encode(
	TVector<FClusterGroup>& Groups, 
	TVector<FCluster>& ClusterSources,
	TVector< FClusterInstance>& ClusterInstances
)
{
	BuildMaterialRanges(ClusterSources);

	ConstrainClusters(Groups, ClusterSources);

	TVector<FCluster> Clusters;
	ExpandClusters(ClusterSources, ClusterInstances, Clusters);
	FBox MeshBounds;
	for (FCluster& Cluster : Clusters)
	{
		MeshBounds.AddInternalBox(Cluster.Bounds);
	}

	int32 PositionPrecision = QuantizePositions(Clusters, MeshBounds);

	TVector<FEncodingInfo> EncodingInfos;
	CalcEncodingInfoFromInstances(EncodingInfos, ClusterSources, Clusters, ClusterInstances, false, 1);

	TVector<FPage> Pages;
	TVector<FClusterGroupPart> GroupParts;
	AssignClusterToPages(Groups, Clusters, EncodingInfos, Pages, GroupParts, 1);

	BuildHierarchies();

	WritePages();
}