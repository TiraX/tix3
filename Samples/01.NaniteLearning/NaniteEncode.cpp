/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "NaniteEncode.h"
#include "Stripifier.h"
#include "NaniteMesh.h"

#define CONSTRAINED_CLUSTER_CACHE_SIZE				32
#define MAX_DEPENDENCY_CHAIN_FOR_RELATIVE_ENCODING	6

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

struct FPageGPUHeader
{
	uint32 NumClusters = 0;
	uint32 Pad[3] = { 0 };
};

struct FPageDiskHeader
{
	uint32 GpuSize;
	uint32 NumClusters;
	uint32 NumRawFloat4s;
	uint32 NumTexCoords;
	uint32 NumVertexRefs;
	uint32 DecodeInfoOffset;
	uint32 StripBitmaskOffset;
	uint32 VertexRefBitmaskOffset;
};

struct FClusterDiskHeader
{
	uint32 IndexDataOffset;
	uint32 PageClusterMapOffset;
	uint32 VertexRefDataOffset;
	uint32 PositionDataOffset;
	uint32 AttributeDataOffset;
	uint32 NumVertexRefs;
	uint32 NumPrevRefVerticesBeforeDwords;
	uint32 NumPrevNewVerticesBeforeDwords;
};

struct FPage
{
	uint32	PartsStartIndex = 0;
	uint32	PartsNum = 0;
	uint32	NumClusters = 0;
	bool	bRelativeEncoding = false;

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

// Wasteful to store size for every vert but easier this way.
struct FVariableVertex
{
	const float* Data;
	uint32 SizeInBytes;

	bool operator==(const FVariableVertex& Other) const
	{
		return 0 == memcmp(Data, Other.Data, SizeInBytes);
	}

	bool operator<(const FVariableVertex& Other) const
	{
		return memcmp(Data, Other.Data, SizeInBytes) < 0;
	}
};

inline uint32 GetTypeHash(FVariableVertex Vert)
{
	TI_ASSERT(0);
	//return CityHash32((const char*)Vert.Data, Vert.SizeInBytes);
	return 0;
}

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

//int32 QuantizePositions(
//	TVector< FCluster >& Clusters, 
//	const FBox& MeshBounds)
int32 QuantizePositions(
	TVector< FCluster >& Clusters)
{
	// Simple global quantization for EA
	const int32 MaxPositionQuantizedValue = (1 << NANITE_MAX_POSITION_QUANTIZATION_BITS) - 1;

	int32 PositionPrecision = TNumLimit<int32>::min();// Settings.PositionPrecision;
	//if (PositionPrecision == MIN_int32)
	{
		// Auto: Guess required precision from bounds at leaf level
		//FFloat3 Extent = MeshBounds.GetExtent();
		//const float MaxSize = TMath::Max3(Extent.X, Extent.Y, Extent.Z);

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


FORCEINLINE static FFloat2 OctahedronEncode(FFloat3 N)
{
	FFloat3 AbsN = N.GetAbs();
	N /= (AbsN.X + AbsN.Y + AbsN.Z);

	if (N.Z < 0.0)
	{
		AbsN = N.GetAbs();
		N.X = (N.X >= 0.0f) ? (1.0f - AbsN.Y) : (AbsN.Y - 1.0f);
		N.Y = (N.Y >= 0.0f) ? (1.0f - AbsN.X) : (AbsN.X - 1.0f);
	}

	return FFloat2(N.X, N.Y);
}

FORCEINLINE static void OctahedronEncode(FFloat3 N, int32& X, int32& Y, int32 QuantizationBits)
{
	const int32 QuantizationMaxValue = (1 << QuantizationBits) - 1;
	const float Scale = 0.5f * QuantizationMaxValue;
	const float Bias = 0.5f * QuantizationMaxValue + 0.5f;

	FFloat2 Coord = OctahedronEncode(N);

	X = TMath::Clamp(int32(Coord.X * Scale + Bias), 0, QuantizationMaxValue);
	Y = TMath::Clamp(int32(Coord.Y * Scale + Bias), 0, QuantizationMaxValue);
}

FORCEINLINE static FFloat3 OctahedronDecode(int32 X, int32 Y, int32 QuantizationBits)
{
	const int32 QuantizationMaxValue = (1 << QuantizationBits) - 1;
	float fx = X * (2.0f / QuantizationMaxValue) - 1.0f;
	float fy = Y * (2.0f / QuantizationMaxValue) - 1.0f;
	float fz = 1.0f - TMath::Abs(fx) - TMath::Abs(fy);
	float t = TMath::Clamp(-fz, 0.0f, 1.0f);
	fx += (fx >= 0.0f ? -t : t);
	fy += (fy >= 0.0f ? -t : t);

	FFloat3 R(fx, fy, fz);
	R.Normalize();
	return R;
}

//FORCEINLINE static void OctahedronEncodePreciseSIMD(FFloat3 N, int32& X, int32& Y, int32 QuantizationBits)
//{
//	const int32 QuantizationMaxValue = (1 << QuantizationBits) - 1;
//	FFloat2 ScalarCoord = OctahedronEncode(N);
//
//	const VectorRegister Scale = VectorSetFloat1(0.5f * QuantizationMaxValue);
//	const VectorRegister RcpScale = VectorSetFloat1(2.0f / QuantizationMaxValue);
//	VectorRegister4Int IntCoord = VectorFloatToInt(VectorMultiplyAdd(MakeVectorRegister(ScalarCoord.X, ScalarCoord.Y, ScalarCoord.X, ScalarCoord.Y), Scale, Scale));	// x0, y0, x1, y1
//	IntCoord = VectorIntAdd(IntCoord, MakeVectorRegisterInt(0, 0, 1, 1));
//	VectorRegister Coord = VectorMultiplyAdd(VectorIntToFloat(IntCoord), RcpScale, GlobalVectorConstants::FloatMinusOne);	// Coord = Coord * 2.0f / QuantizationMaxValue - 1.0f
//
//	VectorRegister Nx = VectorSwizzle(Coord, 0, 2, 0, 2);
//	VectorRegister Ny = VectorSwizzle(Coord, 1, 1, 3, 3);
//	VectorRegister Nz = VectorSubtract(VectorSubtract(VectorOne(), VectorAbs(Nx)), VectorAbs(Ny));			// Nz = 1.0f - abs(Nx) - abs(Ny)
//
//	VectorRegister T = VectorMin(Nz, VectorZero());	// T = min(Nz, 0.0f)
//
//	VectorRegister NxSign = VectorBitwiseAnd(Nx, GlobalVectorConstants::SignBit());
//	VectorRegister NySign = VectorBitwiseAnd(Ny, GlobalVectorConstants::SignBit());
//
//	Nx = VectorAdd(Nx, VectorBitwiseXor(T, NxSign));	// Nx += T ^ NxSign
//	Ny = VectorAdd(Ny, VectorBitwiseXor(T, NySign));	// Ny += T ^ NySign
//
//	VectorRegister RcpLen = VectorReciprocalSqrtAccurate(VectorMultiplyAdd(Nx, Nx, VectorMultiplyAdd(Ny, Ny, VectorMultiply(Nz, Nz))));	// RcpLen = 1.0f / (Nx * Nx + Ny * Ny + Nz * Nz)
//	VectorRegister Dots = VectorMultiply(RcpLen,
//		VectorMultiplyAdd(Nx, VectorSetFloat1(N.X),
//			VectorMultiplyAdd(Ny, VectorSetFloat1(N.Y),
//				VectorMultiply(Nz, VectorSetFloat1(N.Z)))));	// RcpLen * (Nx * N.x + Ny * N.y + Nz * N.z)
//	VectorRegister Mask = MakeVectorRegister(0xFFFFFFFCu, 0xFFFFFFFCu, 0xFFFFFFFCu, 0xFFFFFFFCu);
//	VectorRegister LaneIndices = MakeVectorRegister(0u, 1u, 2u, 3u);
//	Dots = VectorBitwiseOr(VectorBitwiseAnd(Dots, Mask), LaneIndices);
//
//	// Calculate max component
//	VectorRegister MaxDot = VectorMax(Dots, VectorSwizzle(Dots, 2, 3, 0, 1));
//	MaxDot = VectorMax(MaxDot, VectorSwizzle(MaxDot, 1, 2, 3, 0));
//
//	float fIndex = VectorGetComponent(MaxDot, 0);
//	uint32 Index = *(uint32*)&fIndex;
//
//	uint32 IntCoordValues[4];
//	VectorIntStore(IntCoord, IntCoordValues);
//	X = TMath::Clamp((int32)(IntCoordValues[0] + (Index & 1)), 0, QuantizationMaxValue);
//	Y = TMath::Clamp((int32)(IntCoordValues[1] + ((Index >> 1) & 1)), 0, QuantizationMaxValue);
//}
FORCEINLINE static void OctahedronEncodePrecise(FFloat3 N, int32& X, int32& Y, int32 QuantizationBits)
{
	const int32 QuantizationMaxValue = (1 << QuantizationBits) - 1;
	FFloat2 Coord = OctahedronEncode(N);

	const float Scale = 0.5f * QuantizationMaxValue;
	const float Bias = 0.5f * QuantizationMaxValue;
	int32 NX = TMath::Clamp(int32(Coord.X * Scale + Bias), 0, QuantizationMaxValue);
	int32 NY = TMath::Clamp(int32(Coord.Y * Scale + Bias), 0, QuantizationMaxValue);

	float MinError = 1.0f;
	int32 BestNX = 0;
	int32 BestNY = 0;
	for (int32 OffsetY = 0; OffsetY < 2; OffsetY++)
	{
		for (int32 OffsetX = 0; OffsetX < 2; OffsetX++)
		{
			int32 TX = NX + OffsetX;
			int32 TY = NY + OffsetY;
			if (TX <= QuantizationMaxValue && TY <= QuantizationMaxValue)
			{
				FFloat3 RN = OctahedronDecode(TX, TY, QuantizationBits);
				float Error = TMath::Abs(1.0f - (RN.Dot(N)));
				if (Error < MinError)
				{
					MinError = Error;
					BestNX = TX;
					BestNY = TY;
				}
			}
		}
	}

	X = BestNX;
	Y = BestNY;
}

FORCEINLINE static uint32 PackNormal(FFloat3 Normal, uint32 QuantizationBits)
{
	int32 X, Y;
	OctahedronEncodePrecise(Normal, X, Y, QuantizationBits);

#if 0
	// Test against non-SIMD version
	int32 X2, Y2;
	OctahedronEncodePrecise(Normal, X2, Y2, QuantizationBits);
	FFloat3 N0 = OctahedronDecode(X, Y, QuantizationBits);
	FFloat3 N1 = OctahedronDecode(X2, Y2, QuantizationBits);
	float dt0 = Normal | N0;
	float dt1 = Normal | N1;
	TI_ASSERT(dt0 >= dt1 * 0.99999f);
#endif

	return (Y << QuantizationBits) | X;
}

static uint32 PackMaterialTableRange(uint32 TriStart, uint32 TriLength, uint32 MaterialIndex)
{
	uint32 Packed = 0x00000000;
	// uint32 TriStart      :  8; // max 128 triangles
	// uint32 TriLength     :  8; // max 128 triangles
	// uint32 MaterialIndex :  6; // max  64 materials
	// uint32 Padding       : 10;
	TI_ASSERT(TriStart <= 128);
	TI_ASSERT(TriLength <= 128);
	TI_ASSERT(MaterialIndex < 64);
	Packed |= TriStart;
	Packed |= TriLength << 8;
	Packed |= MaterialIndex << 16;
	return Packed;
}

static uint32 PackMaterialFastPath(uint32 Material0Length, uint32 Material0Index, uint32 Material1Length, uint32 Material1Index, uint32 Material2Index)
{
	uint32 Packed = 0x00000000;
	// Material Packed Range - Fast Path (32 bits)
	// uint Material0Index  : 6;   // max  64 materials (0:Material0Length)
	// uint Material1Index  : 6;   // max  64 materials (Material0Length:Material1Length)
	// uint Material2Index  : 6;   // max  64 materials (remainder)
	// uint Material0Length : 7;   // max 128 triangles (num minus one)
	// uint Material1Length : 7;   // max  64 triangles (materials are sorted, so at most 128/2)
	TI_ASSERT(Material0Index < 64);
	TI_ASSERT(Material1Index < 64);
	TI_ASSERT(Material2Index < 64);
	TI_ASSERT(Material0Length >= 1);
	TI_ASSERT(Material0Length <= 128);
	TI_ASSERT(Material1Length <= 64);
	TI_ASSERT(Material1Length <= Material0Length);
	Packed |= Material0Index;
	Packed |= Material1Index << 6;
	Packed |= Material2Index << 12;
	Packed |= (Material0Length - 1u) << 18;
	Packed |= Material1Length << 25;
	return Packed;
}

static uint32 PackMaterialSlowPath(uint32 MaterialTableOffset, uint32 MaterialTableLength)
{
	// Material Packed Range - Slow Path (32 bits)
	// uint BufferIndex     : 19; // 2^19 max value (tons, it's per prim)
	// uint BufferLength	: 6;  // max 64 materials, so also at most 64 ranges (num minus one)
	// uint Padding			: 7;  // always 127 for slow path. corresponds to Material1Length=127 in fast path
	TI_ASSERT(MaterialTableOffset < 524288); // 2^19 - 1
	TI_ASSERT(MaterialTableLength > 0); // clusters with 0 materials use fast path
	TI_ASSERT(MaterialTableLength <= 64);
	uint32 Packed = MaterialTableOffset;
	Packed |= (MaterialTableLength - 1u) << 19;
	Packed |= (0xFE000000u);
	return Packed;
}

static uint32 CalcMaterialTableSize(const FCluster& InCluster)
{
	uint32 NumMaterials = (uint32)InCluster.MaterialRanges.size();
	return NumMaterials > 3 ? NumMaterials : 0;
}

static uint32 PackMaterialInfo(const FCluster& InCluster, TVector<uint32>& OutMaterialTable, uint32 MaterialTableStartOffset)
{
	// Encode material ranges
	uint32 NumMaterialTriangles = 0;
	for (int32 RangeIndex = 0; RangeIndex < (int32)InCluster.MaterialRanges.size(); ++RangeIndex)
	{
		TI_ASSERT(InCluster.MaterialRanges[RangeIndex].RangeLength <= 128);
		TI_ASSERT(InCluster.MaterialRanges[RangeIndex].RangeLength > 0);
		TI_ASSERT(InCluster.MaterialRanges[RangeIndex].MaterialIndex < NANITE_MAX_CLUSTER_MATERIALS);
		NumMaterialTriangles += InCluster.MaterialRanges[RangeIndex].RangeLength;
	}

	// All triangles accounted for in material ranges?
	TI_ASSERT(NumMaterialTriangles == InCluster.NumTris);

	uint32 PackedMaterialInfo = 0x00000000;

	// The fast inline path can encode up to 3 materials
	if (InCluster.MaterialRanges.size() <= 3)
	{
		uint32 Material0Length = 0;
		uint32 Material0Index = 0;
		uint32 Material1Length = 0;
		uint32 Material1Index = 0;
		uint32 Material2Index = 0;

		if (InCluster.MaterialRanges.size() > 0)
		{
			const FMaterialRange& Material0 = InCluster.MaterialRanges[0];
			TI_ASSERT(Material0.RangeStart == 0);
			Material0Length = Material0.RangeLength;
			Material0Index = Material0.MaterialIndex;
		}

		if (InCluster.MaterialRanges.size() > 1)
		{
			const FMaterialRange& Material1 = InCluster.MaterialRanges[1];
			TI_ASSERT(Material1.RangeStart == InCluster.MaterialRanges[0].RangeLength);
			Material1Length = Material1.RangeLength;
			Material1Index = Material1.MaterialIndex;
		}

		if (InCluster.MaterialRanges.size() > 2)
		{
			const FMaterialRange& Material2 = InCluster.MaterialRanges[2];
			TI_ASSERT(Material2.RangeStart == Material0Length + Material1Length);
			TI_ASSERT(Material2.RangeLength == InCluster.NumTris - Material0Length - Material1Length);
			Material2Index = Material2.MaterialIndex;
		}

		PackedMaterialInfo = PackMaterialFastPath(Material0Length, Material0Index, Material1Length, Material1Index, Material2Index);
	}
	// Slow global table search path
	else
	{
		uint32 MaterialTableOffset = (uint32)OutMaterialTable.size() + MaterialTableStartOffset;
		uint32 MaterialTableLength = (uint32)InCluster.MaterialRanges.size();
		TI_ASSERT(MaterialTableLength > 0);

		for (int32 RangeIndex = 0; RangeIndex < InCluster.MaterialRanges.size(); ++RangeIndex)
		{
			const FMaterialRange& Material = InCluster.MaterialRanges[RangeIndex];
			OutMaterialTable.push_back(PackMaterialTableRange(Material.RangeStart, Material.RangeLength, Material.MaterialIndex));
		}

		PackedMaterialInfo = PackMaterialSlowPath(MaterialTableOffset, MaterialTableLength);
	}

	return PackedMaterialInfo;
}

static void PackCluster(FPackedCluster& OutCluster, const FCluster& InCluster, const FEncodingInfo& EncodingInfo, uint32 NumTexCoords)
{
	memset(&OutCluster, 0, sizeof(FPackedCluster));

	// 0
	OutCluster.SetNumVerts(InCluster.NumVerts);
	OutCluster.SetPositionOffset(0);
	OutCluster.SetNumTris(InCluster.NumTris);
	OutCluster.SetIndexOffset(0);
	OutCluster.ColorMin = EncodingInfo.ColorMin.X | (EncodingInfo.ColorMin.Y << 8) | (EncodingInfo.ColorMin.Z << 16) | (EncodingInfo.ColorMin.W << 24);
	OutCluster.SetColorBitsR(EncodingInfo.ColorBits.X);
	OutCluster.SetColorBitsG(EncodingInfo.ColorBits.Y);
	OutCluster.SetColorBitsB(EncodingInfo.ColorBits.Z);
	OutCluster.SetColorBitsA(EncodingInfo.ColorBits.W);
	OutCluster.SetGroupIndex(InCluster.GroupIndex);

	// 1
	OutCluster.PosStart = InCluster.QuantizedPosStart;
	OutCluster.SetBitsPerIndex(EncodingInfo.BitsPerIndex);
	OutCluster.SetPosPrecision(InCluster.QuantizedPosPrecision);
	OutCluster.SetPosBitsX(InCluster.QuantizedPosBits.X);
	OutCluster.SetPosBitsY(InCluster.QuantizedPosBits.Y);
	OutCluster.SetPosBitsZ(InCluster.QuantizedPosBits.Z);

	// 2
	OutCluster.LODBounds = FFloat4(InCluster.LODBounds.Center.X, InCluster.LODBounds.Center.Y, InCluster.LODBounds.Center.Z, InCluster.LODBounds.W);

	// 3
	OutCluster.BoxBoundsCenter = (InCluster.Bounds.Min + InCluster.Bounds.Max) * 0.5f;
	OutCluster.LODErrorAndEdgeLength = float16(InCluster.LODError).data() | (float16(InCluster.EdgeLength).data() << 16);

	// 4
	OutCluster.BoxBoundsExtent = (InCluster.Bounds.Max - InCluster.Bounds.Min) * 0.5f;
	OutCluster.Flags = NANITE_CLUSTER_FLAG_LEAF;
	OutCluster.Flags |= InCluster.Flags;

	// 5
	TI_ASSERT(NumTexCoords <= NANITE_MAX_UVS);
	static_assert(NANITE_MAX_UVS <= 4, "UV_Prev encoding only supports up to 4 channels");

	OutCluster.SetBitsPerAttribute(EncodingInfo.BitsPerAttribute);
	OutCluster.SetNumUVs(NumTexCoords);
	OutCluster.SetColorMode(EncodingInfo.ColorMode);
	OutCluster.UV_Prec = EncodingInfo.UVPrec;
	OutCluster.PackedMaterialInfo = 0;	// Filled out by WritePages
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

struct FVertexMapEntry
{
	uint32 LocalClusterIndex;
	uint32 VertexIndex;
};

static void EncodeGeometryData(
	const uint32 LocalClusterIndex, 
	const FCluster& Cluster, 
	const FEncodingInfo& EncodingInfo, 
	uint32 NumTexCoords,
	TVector<uint32>& StripBitmask, 
	TVector<uint8>& IndexData,
	TVector<uint32>& PageClusterMapData,
	TVector<uint32>& VertexRefBitmask, 
	TVector<uint16>& VertexRefData, 
	TVector<uint8>& PositionData, 
	TVector<uint8>& AttributeData,
	const TVectorView<uint32> PageDependencies, 
	const TVector<TMap<FVariableVertex, FVertexMapEntry>>& PageVertexMaps,
	TMap<FVariableVertex, uint32>& UniqueVertices, 
	uint32& NumCodedVertices
)
{
	const uint32 NumClusterVerts = Cluster.NumVerts;
	const uint32 NumClusterTris = Cluster.NumTris;

	VertexRefBitmask.resize(VertexRefBitmask.size() + NANITE_MAX_CLUSTER_VERTICES / 32);

	// Find vertices from same page we can reference instead of storing duplicates
	struct FVertexRef
	{
		uint32 PageIndex;
		uint32 LocalClusterIndex;
		uint32 VertexIndex;
	};

	TVector<FVertexRef> VertexRefs;
	TVector<uint32> UniqueToVertexIndex;
	for (uint32 VertexIndex = 0; VertexIndex < NumClusterVerts; VertexIndex++)
	{
		FVariableVertex Vertex;
		Vertex.Data = &Cluster.Verts[VertexIndex * Cluster.GetVertSize()];
		Vertex.SizeInBytes = Cluster.GetVertSize() * sizeof(float);

		FVertexRef VertexRef = {};
		bool bFound = false;

		// Look for vertex in parents
		for (int32 SrcPageIndexIndex = 0; SrcPageIndexIndex < (int32)PageDependencies.Size(); SrcPageIndexIndex++)
		{
			uint32 SrcPageIndex = PageDependencies[SrcPageIndexIndex];
			//const FVertexMapEntry* EntryPtr = PageVertexMaps[SrcPageIndex].Find(Vertex);
			TMap<FVariableVertex, FVertexMapEntry>::const_iterator EntryIt = PageVertexMaps[SrcPageIndex].find(Vertex);
			if (EntryIt != PageVertexMaps[SrcPageIndex].end())
			{
				VertexRef = FVertexRef{ (uint32)SrcPageIndexIndex + 1, EntryIt->second.LocalClusterIndex, EntryIt->second.VertexIndex };
				bFound = true;
				break;
			}
		}

		if (!bFound)
		{
			// Look for vertex in current page
			//uint32* VertexPtr = UniqueVertices.Find(Vertex);
			TMap<FVariableVertex, uint32>::iterator VertexIt = UniqueVertices.find(Vertex);
			if (VertexIt != UniqueVertices.end())
			{
				VertexRef = FVertexRef{ 0, (VertexIt->second >> NANITE_MAX_CLUSTER_VERTICES_BITS), VertexIt->second & NANITE_MAX_CLUSTER_VERTICES_MASK };
				bFound = true;
			}
		}

		if (bFound)
		{
			VertexRefs.push_back(VertexRef);
			const uint32 BitIndex = (LocalClusterIndex << NANITE_MAX_CLUSTER_VERTICES_BITS) + VertexIndex;
			VertexRefBitmask[BitIndex >> 5] |= 1u << (BitIndex & 31);
		}
		else
		{
			uint32 Val = (LocalClusterIndex << NANITE_MAX_CLUSTER_VERTICES_BITS) | (uint32)UniqueToVertexIndex.size();
			//UniqueVertices.Add(Vertex, Val);
			UniqueVertices[Vertex] = Val;
			UniqueToVertexIndex.push_back(VertexIndex);
		}
	}
	NumCodedVertices = (uint32)UniqueToVertexIndex.size();

	struct FClusterRef
	{
		uint32 PageIndex;
		uint32 ClusterIndex;

		bool operator==(const FClusterRef& Other) const { return PageIndex == Other.PageIndex && ClusterIndex == Other.ClusterIndex; }
		bool operator<(const FClusterRef& Other) const { return (PageIndex != Other.PageIndex) ? (PageIndex < Other.PageIndex) : (ClusterIndex == Other.ClusterIndex); }
	};

	// Make list of unique Page-Cluster pairs
	TVector<FClusterRef> ClusterRefs;
	for (const FVertexRef& Ref : VertexRefs)
	{
		FClusterRef CRef;
		CRef.PageIndex = Ref.PageIndex;
		CRef.ClusterIndex = Ref.LocalClusterIndex;
		TVector<FClusterRef>::const_iterator It = TFind(ClusterRefs.begin(), ClusterRefs.end(), CRef);
		if (It == ClusterRefs.end())
			ClusterRefs.push_back(CRef);
	}

	//ClusterRefs.Sort();
	TSort(ClusterRefs.begin(), ClusterRefs.end());

	for (const FClusterRef& Ref : ClusterRefs)
	{
		PageClusterMapData.push_back((Ref.PageIndex << NANITE_MAX_CLUSTERS_PER_PAGE_BITS) | Ref.ClusterIndex);
	}

	// Write vertex refs using Page-Cluster index + vertex index
	for (const FVertexRef& Ref : VertexRefs)
	{
		FClusterRef CRef;
		CRef.PageIndex = Ref.PageIndex;
		CRef.ClusterIndex = Ref.LocalClusterIndex;

		//uint32 PageClusterIndex = ClusterRefs.Find(FClusterRef{ Ref.PageIndex, Ref.LocalClusterIndex });
		TVector<FClusterRef>::iterator It = TFind(ClusterRefs.begin(), ClusterRefs.end(), CRef);
		uint32 PageClusterIndex = (uint32)TDistance(ClusterRefs.begin(), It);
		TI_ASSERT(It != ClusterRefs.end() && PageClusterIndex < 256);
		VertexRefData.push_back((PageClusterIndex << NANITE_MAX_CLUSTER_VERTICES_BITS) | Ref.VertexIndex);
	}

	const uint32 BitsPerIndex = EncodingInfo.BitsPerIndex;

	// Write triangle indices
#if NANITE_USE_STRIP_INDICES
	if ((Cluster.Flags & NANITE_CLUSTER_FLAG_NO_STRIPIFY) == 0)
	{
		for (uint32 i = 0; i < NANITE_MAX_CLUSTER_TRIANGLES / 32; i++)
		{
			StripBitmask.push_back(Cluster.StripDesc.Bitmasks[i][0]);
			StripBitmask.push_back(Cluster.StripDesc.Bitmasks[i][1]);
			StripBitmask.push_back(Cluster.StripDesc.Bitmasks[i][2]);
		}
		//IndexData.Append(Cluster.StripIndexData);
		IndexData.insert(IndexData.end(), Cluster.StripIndexData.begin(), Cluster.StripIndexData.end());
	}
	else
	{
		// Use un-stripified index
		for (uint32 i = 0; i < NumClusterTris * 3; i++)
		{
			uint32 Index = Cluster.Indexes[i];
			IndexData.push_back(Cluster.Indexes[i]);
		}
	}
#else
	for (uint32 i = 0; i < NumClusterTris * 3; i++)
	{
		uint32 Index = Cluster.Indexes[i];
		IndexData.Add(Cluster.Indexes[i]);
	}
#endif

	TI_ASSERT(NumClusterVerts > 0);

	TBitWriter BitWriter_Position(PositionData);
	TBitWriter BitWriter_Attribute(AttributeData);

#if NANITE_USE_UNCOMPRESSED_VERTEX_DATA
	for (uint32 VertexIndex = 0; VertexIndex < NumClusterVerts; VertexIndex++)
	{
		const FFloat3& Position = Cluster.GetPosition(VertexIndex);
		BitWriter_Position.PutBits(*(uint32*)&Position.X, 32);
		BitWriter_Position.PutBits(*(uint32*)&Position.Y, 32);
		BitWriter_Position.PutBits(*(uint32*)&Position.Z, 32);
	}
	BitWriter_Position.Flush(sizeof(uint32));

	for (uint32 VertexIndex = 0; VertexIndex < NumClusterVerts; VertexIndex++)
	{
		// Normal
		const FFloat3& Normal = Cluster.GetNormal(VertexIndex);
		BitWriter_Attribute.PutBits(*(uint32*)&Normal.X, 32);
		BitWriter_Attribute.PutBits(*(uint32*)&Normal.Y, 32);
		BitWriter_Attribute.PutBits(*(uint32*)&Normal.Z, 32);

		// Color
		uint32 ColorDW = Cluster.bHasColors ? Cluster.GetColor(VertexIndex).ToFColor(false).DWColor() : 0xFFFFFFFFu;
		BitWriter_Attribute.PutBits(ColorDW, 32);

		// UVs
		const FFloat2* UVs = Cluster.GetUVs(VertexIndex);
		for (uint32 TexCoordIndex = 0; TexCoordIndex < NumTexCoords; TexCoordIndex++)
		{
			const FFloat2& UV = UVs[TexCoordIndex];
			BitWriter_Attribute.PutBits(*(uint32*)&UV.X, 32);
			BitWriter_Attribute.PutBits(*(uint32*)&UV.Y, 32);
		}
	}
	BitWriter_Attribute.Flush(sizeof(uint32));
#else

	// Generate quantized texture coordinates
	TVector<uint32> PackedUVs;
	PackedUVs.resize(NumClusterVerts * NumTexCoords);

	uint32 TexCoordBits[NANITE_MAX_UVS] = {};
	for (uint32 UVIndex = 0; UVIndex < NumTexCoords; UVIndex++)
	{
		const int32 TexCoordBitsU = (EncodingInfo.UVPrec >> (UVIndex * 8 + 0)) & 15;
		const int32 TexCoordBitsV = (EncodingInfo.UVPrec >> (UVIndex * 8 + 4)) & 15;
		const int32 TexCoordMaxValueU = (1 << TexCoordBitsU) - 1;
		const int32 TexCoordMaxValueV = (1 << TexCoordBitsV) - 1;

		const FUVRange& UVRange = EncodingInfo.UVRanges[UVIndex];
		const float QuantizationScale = TMath::Pow(2.f, (float)UVRange.Precision);

		for (uint32 i : UniqueToVertexIndex)
		{
			const FFloat2& UV = Cluster.GetUVs(i)[UVIndex];

			int32 U = (int32)TMath::RoundToFloat(UV.X * QuantizationScale) - UVRange.Min.X;
			int32 V = (int32)TMath::RoundToFloat(UV.Y * QuantizationScale) - UVRange.Min.Y;
			if (U > UVRange.GapStart.X)
			{
				TI_ASSERT(U >= UVRange.GapStart.X + UVRange.GapLength.X);
				U -= UVRange.GapLength.X;
			}
			if (V > UVRange.GapStart.Y)
			{
				TI_ASSERT(V >= UVRange.GapStart.Y + UVRange.GapLength.Y);
				V -= UVRange.GapLength.Y;
			}

			TI_ASSERT(U >= 0 && U <= TexCoordMaxValueU);
			TI_ASSERT(V >= 0 && V <= TexCoordMaxValueV);
			PackedUVs[NumClusterVerts * UVIndex + i] = (uint32(V) << TexCoordBitsU) | uint32(U);
		}
		TexCoordBits[UVIndex] = TexCoordBitsU + TexCoordBitsV;
	}

	// Quantize and write positions
	for (uint32 VertexIndex : UniqueToVertexIndex)
	{
		const FInt3& Position = Cluster.QuantizedPositions[VertexIndex];
		BitWriter_Position.PutBits(Position.X, Cluster.QuantizedPosBits.X);
		BitWriter_Position.PutBits(Position.Y, Cluster.QuantizedPosBits.Y);
		BitWriter_Position.PutBits(Position.Z, Cluster.QuantizedPosBits.Z);
		BitWriter_Position.Flush(1);
	}
	BitWriter_Position.Flush(sizeof(uint32));

	// Quantize and write remaining shading attributes
	for (uint32 VertexIndex : UniqueToVertexIndex)
	{
		// Normal
		uint32 PackedNormal = PackNormal(Cluster.GetNormal(VertexIndex), NANITE_NORMAL_QUANTIZATION_BITS);
		BitWriter_Attribute.PutBits(PackedNormal, 2 * NANITE_NORMAL_QUANTIZATION_BITS);

		// Color
		if (EncodingInfo.ColorMode == NANITE_VERTEX_COLOR_MODE_VARIABLE)
		{
			SColor Color = Cluster.GetColor(VertexIndex).ToSColor();

			int32 R = Color.R - EncodingInfo.ColorMin.X;
			int32 G = Color.G - EncodingInfo.ColorMin.Y;
			int32 B = Color.B - EncodingInfo.ColorMin.Z;
			int32 A = Color.A - EncodingInfo.ColorMin.W;
			BitWriter_Attribute.PutBits(R, EncodingInfo.ColorBits.X);
			BitWriter_Attribute.PutBits(G, EncodingInfo.ColorBits.Y);
			BitWriter_Attribute.PutBits(B, EncodingInfo.ColorBits.Z);
			BitWriter_Attribute.PutBits(A, EncodingInfo.ColorBits.W);
		}

		// UVs
		for (uint32 TexCoordIndex = 0; TexCoordIndex < NumTexCoords; TexCoordIndex++)
		{
			uint32 PackedUV = PackedUVs[NumClusterVerts * TexCoordIndex + VertexIndex];
			BitWriter_Attribute.PutBits(PackedUV, TexCoordBits[TexCoordIndex]);
		}
		BitWriter_Attribute.Flush(1);
	}
	BitWriter_Attribute.Flush(sizeof(uint32));
#endif
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
			FPage* Page = &Pages.back();
			bool bRootPage = (Pages.size() - 1u) < MaxRootPages;
			if (Page->GpuSizes.GetTotal() + EncodingInfo.GpuSizes.GetTotal() > (bRootPage ? NANITE_ROOT_PAGE_GPU_SIZE : NANITE_STREAMING_PAGE_GPU_SIZE) || Page->NumClusters + 1 > NANITE_MAX_CLUSTERS_PER_PAGE)
			{
				// Page is full. Need to start a new one
				Pages.push_back(FPage());
				Page = &Pages.back();
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
		TI_ASSERT(Group.PageIndexNum >= 1 && Group.PageIndexNum <= 2);
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

static float BVH_Cost(const TVector<FIntermediateNode>& Nodes, const TVectorView<uint32>& NodeIndices)
{
	FBox Bound;
	for (int32 i = 0; i < NodeIndices.Size(); i++)
	{
		uint32 NodeIndex = NodeIndices[i];
		Bound.AddInternalBox(Nodes[NodeIndex].Bound);
	}
	return Bound.GetArea();
}

static void BVH_SortNodes(const TVector<FIntermediateNode>& Nodes, TVectorView<uint32> NodeIndices, const TVector<uint32>& ChildSizes)
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
			TVectorView<uint32> NodeIndices01 = NodeIndices.Slice(BucketStartIndex, Sizes[0] + Sizes[1]);
			TVectorView<uint32> NodeIndices0 = NodeIndices.Slice(BucketStartIndex, Sizes[0]);
			TVectorView<uint32> NodeIndices1 = NodeIndices.Slice(BucketStartIndex + Sizes[0], Sizes[1]);

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

	uint32 NumChildren = (uint32)INode.Children.size();
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
			HNode.NumChildren[ChildIndex] = (uint32)Part.Clusters.size();
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
			TVector< FSpheref> LODBoundSpheres;
			LODBoundSpheres.reserve(NANITE_MAX_BVH_NODE_FANOUT);
			float MinLODError = MAX_flt;
			float MaxParentLODError = 0.0f;
			for (uint32 GrandChildIndex = 0; GrandChildIndex < NANITE_MAX_BVH_NODE_FANOUT && ChildHNode.NumChildren[GrandChildIndex] != 0; GrandChildIndex++)
			{
				Bounds.AddInternalBox(ChildHNode.Bounds[GrandChildIndex]);
				LODBoundSpheres.push_back(ChildHNode.LODBounds[GrandChildIndex]);
				MinLODError = TMath::Min(MinLODError, ChildHNode.MinLODErrors[GrandChildIndex]);
				MaxParentLODError = TMath::Max(MaxParentLODError, ChildHNode.MaxParentLODErrors[GrandChildIndex]);
			}

			FSpheref LODBounds = FSpheref(LODBoundSpheres.data(), (int32)LODBoundSpheres.size());

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
static uint32 BuildHierarchyTopDown(TVector<FIntermediateNode>& Nodes, TVectorView<uint32> NodeIndices, bool bSort)
{
	const uint32 N = (uint32)NodeIndices.Size();
	if (N == 1)
	{
		return NodeIndices[0];
	}

	const uint32 NewRootIndex = (uint32)Nodes.size();
	//Nodes.AddDefaulted_GetRef();
	Nodes.push_back(FIntermediateNode());

	if (N <= NANITE_MAX_BVH_NODE_FANOUT)
	{
		//Nodes[NewRootIndex].Children = NodeIndices;
		Nodes[NewRootIndex].Children.resize(NodeIndices.Size());
		for (int32 i = 0; i < NodeIndices.Size(); i++)
		{
			Nodes[NewRootIndex].Children[i] = NodeIndices[i];
		}
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
		Nodes[NewRootIndex].Children.push_back(NodeIndex);
		Offset += ChildSize;
	}

	return NewRootIndex;
}

void BuildHierarchies(
	const TVector<FClusterGroup>& Groups,
	uint32 NumMeshes,
	TVector<FClusterGroupPart>& Parts,
	TVector<FPackedHierarchyNode>& PackedHierarchyNodes)
{
	TVector<TVector<uint32>> PartsByMesh;
	PartsByMesh.resize(NumMeshes);
	for (auto& P : PartsByMesh)
	{
		P.reserve(64);
	}

	// Assign group parts to the meshes they belong to
	const uint32 NumTotalParts = (uint32)Parts.size();
	for (uint32 PartIndex = 0; PartIndex < NumTotalParts; PartIndex++)
	{
		const FClusterGroupPart& Part = Parts[PartIndex];
		PartsByMesh[Groups[Part.GroupIndex].MeshIndex].push_back(PartIndex);
	}

	for (uint32 MeshIndex = 0; MeshIndex < NumMeshes; MeshIndex++)
	{
		const TVector<uint32>& PartIndices = PartsByMesh[MeshIndex];
		const uint32 NumParts = (uint32)PartIndices.size();

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
			NodesByMip[Group.MipLevel].push_back(i);
		}


		uint32 RootIndex = 0;
		if (Nodes.size() == 1)
		{
			// Just a single leaf.
			// Needs to be special-cased as root should always be an inner node.
			//FIntermediateNode& Node = Nodes.AddDefaulted_GetRef();
			Nodes.push_back(FIntermediateNode());
			FIntermediateNode& Node = Nodes.back();
			Node.Children.push_back(0);
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
						LevelRoots.push_back(NodeIndex);
					}
					else
					{
						// Incomplete node. Discard the code and add the children as roots instead.
						//LevelRoots.Append(Nodes[NodeIndex].Children);
						LevelRoots.insert(LevelRoots.end(), Nodes[NodeIndex].Children.begin(), Nodes[NodeIndex].Children.end());
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

		TI_ASSERT((int32)HierarchyNodes.size() <= TNaniteMesh::MaxHierarchyNodes);
		// Convert hierarchy to packed format
		const uint32 NumHierarchyNodes = (uint32)HierarchyNodes.size();
		TI_ASSERT(PackedHierarchyNodes.size() == 0);
		PackedHierarchyNodes.resize(NumHierarchyNodes);
		for (uint32 i = 0; i < NumHierarchyNodes; i++)
		{
			PackHierarchyNode(PackedHierarchyNodes[i], HierarchyNodes[i], Groups, Parts, 1);
		}
	}
}

// TODO: does unreal already have something like this?
class FBlockPointer
{
	uint8* StartPtr;
	uint8* EndPtr;
	uint8* Ptr;
public:
	FBlockPointer(uint8* Ptr, uint32 SizeInBytes) :
		StartPtr(Ptr), EndPtr(Ptr + SizeInBytes), Ptr(Ptr)
	{
	}

	template<typename T>
	T* Advance(uint32 Num)
	{
		T* Result = (T*)Ptr;
		Ptr += Num * sizeof(T);
		TI_ASSERT(Ptr <= EndPtr);
		return Result;
	}

	template<typename T>
	T* GetPtr() const { return (T*)Ptr; }

	uint32 Offset() const
	{
		return uint32(Ptr - StartPtr);
	}

	void Align(uint32 Alignment)
	{
		while (Offset() % Alignment)
		{
			*Advance<uint8>(1) = 0;
		}
	}
};

static uint32 MarkRelativeEncodingPagesRecursive(TVector<FPage>& Pages, TVector<uint32>& PageDependentsDepth, const TVector<TVector<uint32>>& PageDependents, uint32 PageIndex)
{
	if (PageDependentsDepth[PageIndex] != MAX_uint32)
	{
		return PageDependentsDepth[PageIndex];
	}

	uint32 Depth = 0;
	for (const uint32 DependentPageIndex : PageDependents[PageIndex])
	{
		const uint32 DependentDepth = MarkRelativeEncodingPagesRecursive(Pages, PageDependentsDepth, PageDependents, DependentPageIndex);
		Depth = TMath::Max(Depth, DependentDepth + 1u);
	}

	FPage& Page = Pages[PageIndex];
	Page.bRelativeEncoding = true;

	if (Depth >= MAX_DEPENDENCY_CHAIN_FOR_RELATIVE_ENCODING)
	{
		// Using relative encoding for this page would make the dependency chain too long. Use direct coding instead and reset depth.
		Page.bRelativeEncoding = false;
		Depth = 0;
	}

	PageDependentsDepth[PageIndex] = Depth;
	return Depth;
}

static uint32 MarkRelativeEncodingPages(TNaniteMesh& Mesh, TVector<FPage>& Pages, const TVector<FClusterGroup>& Groups, const TVector<FClusterGroupPart>& Parts)
{
	const uint32 NumPages = (uint32)Mesh.PageStreamingStates.size();

	// Build list of dependents for each page
	TVector<TVector<uint32>> PageDependents;
	PageDependents.resize(NumPages);

	// Memorize how many levels of dependency a given page has
	TVector<uint32> PageDependentsDepth;
	PageDependentsDepth.resize(NumPages);
	TFill(PageDependentsDepth.begin(), PageDependentsDepth.end(), MAX_uint32);

	//TBitArray<> PageHasOnlyRootDependencies(false, NumPages);
	TVector<bool> PageHasOnlyRootDependencies;
	PageHasOnlyRootDependencies.resize(NumPages);
	TFill(PageHasOnlyRootDependencies.begin(), PageHasOnlyRootDependencies.end(), false);

	for (uint32 PageIndex = 0; PageIndex < NumPages; PageIndex++)
	{
		const FPageStreamingState& PageStreamingState = Mesh.PageStreamingStates[PageIndex];

		bool bHasRootDependency = false;
		bool bHasStreamingDependency = false;
		for (uint32 i = 0; i < PageStreamingState.DependenciesNum; i++)
		{
			const uint32 DependencyPageIndex = Mesh.PageDependencies[PageStreamingState.DependenciesStart + i];
			if (Mesh.IsRootPage(DependencyPageIndex))
			{
				bHasRootDependency = true;
			}
			else
			{
				TVector<uint32>::const_iterator It = TFind(PageDependents[DependencyPageIndex].begin(), PageDependents[DependencyPageIndex].end(), PageIndex);
				if (It == PageDependents[DependencyPageIndex].end())
					PageDependents[DependencyPageIndex].push_back(PageIndex);
				bHasStreamingDependency = true;
			}
		}

		PageHasOnlyRootDependencies[PageIndex] = (bHasRootDependency && !bHasStreamingDependency);
	}

	uint32 NumRelativeEncodingPages = 0;
	for (uint32 PageIndex = 0; PageIndex < NumPages; PageIndex++)
	{
		FPage& Page = Pages[PageIndex];

		MarkRelativeEncodingPagesRecursive(Pages, PageDependentsDepth, PageDependents, PageIndex);

		if (Mesh.IsRootPage(PageIndex))
		{
			// Root pages never use relative encoding
			Page.bRelativeEncoding = false;
		}
		else if (PageHasOnlyRootDependencies[PageIndex])
		{
			// Root pages are always resident, so dependencies on them shouldn't count towards dependency chain limit.
			// If a page only has root dependencies, always code it as relative.
			Page.bRelativeEncoding = true;
		}

		if (Page.bRelativeEncoding)
		{
			NumRelativeEncodingPages++;
		}
	}

	return NumRelativeEncodingPages;
}

static TVector<TMap<FVariableVertex, FVertexMapEntry>> BuildVertexMaps(const TVector<FPage>& Pages, const TVector<FCluster>& Clusters, const TVector<FClusterGroupPart>& Parts)
{
	TVector<TMap<FVariableVertex, FVertexMapEntry>> VertexMaps;
	VertexMaps.resize(Pages.size());

	const int32 NumPages = (int32)Pages.size();
	for (int32 PageIndex = 0; PageIndex < NumPages; PageIndex++)
	{
		const FPage& Page = Pages[PageIndex];
		for (uint32 i = 0; i < Page.PartsNum; i++)
		{
			const FClusterGroupPart& Part = Parts[Page.PartsStartIndex + i];
			for (uint32 j = 0; j < (uint32)Part.Clusters.size(); j++)
			{
				const uint32 ClusterIndex = Part.Clusters[j];
				const uint32 LocalClusterIndex = Part.PageClusterOffset + j;
				const FCluster& Cluster = Clusters[ClusterIndex];

				for (uint32 VertexIndex = 0; VertexIndex < Cluster.NumVerts; VertexIndex++)
				{
					FVariableVertex Vertex;
					Vertex.Data = &Cluster.Verts[VertexIndex * Cluster.GetVertSize()];
					Vertex.SizeInBytes = Cluster.GetVertSize() * sizeof(float);
					FVertexMapEntry Entry;
					Entry.LocalClusterIndex = LocalClusterIndex;
					Entry.VertexIndex = VertexIndex;
					VertexMaps[PageIndex][Vertex] = Entry;
				}
			}
		}
	}
	return VertexMaps;
}

void WritePages(
	TNaniteMesh& Mesh,
	TVector<FPage>& Pages,
	const TVector<FClusterGroup>& Groups,
	const TVector<FClusterGroupPart>& Parts,
	TVector<FCluster>& Clusters,
	const TVector<FEncodingInfo>& EncodingInfos,
	uint32 NumTexCoords
)
{ 
	Mesh.NumRootPageClusters = Pages[0].NumClusters;

	TI_ASSERT(Mesh.PageStreamingStates.size() == 0);

	TVector< uint8 > StreamableBulkData;

	const uint32 NumPages = (uint32)Pages.size();
	const uint32 NumClusters = (uint32)Clusters.size();
	Mesh.PageStreamingStates.resize(NumPages);

	TVector<FFixupChunk> FixupChunks;
	FixupChunks.resize(NumPages);
	int32 TotalClusters = 0;
	for (uint32 PageIndex = 0; PageIndex < NumPages; PageIndex++)
	{
		const FPage& Page = Pages[PageIndex];
		FFixupChunk& FixupChunk = FixupChunks[PageIndex];
		FixupChunk.Header.NumClusters = Page.NumClusters;
		TotalClusters += Page.NumClusters;

		uint32 NumHierarchyFixups = 0;
		for (uint32 i = 0; i < Page.PartsNum; i++)
		{
			const FClusterGroupPart& Part = Parts[Page.PartsStartIndex + i];
			NumHierarchyFixups += Groups[Part.GroupIndex].PageIndexNum;
		}

		FixupChunk.Header.NumHierachyFixups = NumHierarchyFixups;	// NumHierarchyFixups must be set before writing cluster fixups
	}

	// Add external fixups to pages
	for (const FClusterGroupPart& Part : Parts)
	{
		TI_ASSERT(Part.PageIndex < NumPages);

		const FClusterGroup& Group = Groups[Part.GroupIndex];
		for (uint32 ClusterPositionInPart = 0; ClusterPositionInPart < (uint32)Part.Clusters.size(); ClusterPositionInPart++)
		{
			const FCluster& Cluster = Clusters[Part.Clusters[ClusterPositionInPart]];
			if (Cluster.GeneratingGroupIndex != INVALID_GROUP_INDEX)
			{
				const FClusterGroup& GeneratingGroup = Groups[Cluster.GeneratingGroupIndex];
				TI_ASSERT(GeneratingGroup.PageIndexNum >= 1);

				uint32 PageDependencyStart = GeneratingGroup.PageIndexStart;
				uint32 PageDependencyNum = GeneratingGroup.PageIndexNum;
				RemoveRootPagesFromRange(PageDependencyStart, PageDependencyNum, Mesh.NumRootPages);
				RemovePageFromRange(PageDependencyStart, PageDependencyNum, Part.PageIndex);

				if (PageDependencyNum == 0)
					continue;	// Dependencies already met by current page and/or root pages

				const FClusterFixup ClusterFixup = FClusterFixup(Part.PageIndex, Part.PageClusterOffset + ClusterPositionInPart, PageDependencyStart, PageDependencyNum);
				for (uint32 i = 0; i < GeneratingGroup.PageIndexNum; i++)
				{
					//TODO: Implement some sort of FFixupPart to not redundantly store PageIndexStart/PageIndexNum?
					FFixupChunk& FixupChunk = FixupChunks[GeneratingGroup.PageIndexStart + i];
					FixupChunk.GetClusterFixup(FixupChunk.Header.NumClusterFixups++) = ClusterFixup;
				}
			}
		}
	}

	// Generate page dependencies
	for (uint32 PageIndex = 0; PageIndex < NumPages; PageIndex++)
	{
		const FFixupChunk& FixupChunk = FixupChunks[PageIndex];
		FPageStreamingState& PageStreamingState = Mesh.PageStreamingStates[PageIndex];
		PageStreamingState.DependenciesStart = (uint32)Mesh.PageDependencies.size();

		for (uint32 i = 0; i < FixupChunk.Header.NumClusterFixups; i++)
		{
			uint32 FixupPageIndex = FixupChunk.GetClusterFixup(i).GetPageIndex();
			TI_ASSERT(FixupPageIndex < NumPages);
			if (FixupPageIndex == PageIndex)	// Never emit dependencies to ourselves
				continue;

			// Only add if not already in the set.
			// O(n^2), but number of dependencies should be tiny in practice.
			bool bFound = false;
			for (uint32 j = PageStreamingState.DependenciesStart; j < (uint32)Mesh.PageDependencies.size(); j++)
			{
				if (Mesh.PageDependencies[j] == FixupPageIndex)
				{
					bFound = true;
					break;
				}
			}

			if (bFound)
				continue;

			Mesh.PageDependencies.push_back(FixupPageIndex);
		}
		PageStreamingState.DependenciesNum = (uint32)Mesh.PageDependencies.size() - PageStreamingState.DependenciesStart;
	}

	auto PageVertexMaps = BuildVertexMaps(Pages, Clusters, Parts);
	const uint32 NumRelativeEncodingPages = MarkRelativeEncodingPages(Mesh, Pages, Groups, Parts);

	// Process pages
	TVector< TVector<uint8> > PageResults;
	PageResults.resize(NumPages);

	for (uint32 PageIndex = 0; PageIndex < NumPages; PageIndex++)
	{
		const FPage& Page = Pages[PageIndex];
		FFixupChunk& FixupChunk = FixupChunks[PageIndex];

		Mesh.PageStreamingStates[PageIndex].Flags = Page.bRelativeEncoding ? NANITE_PAGE_FLAG_RELATIVE_ENCODING : 0;

		// Add hierarchy fixups
		{
			// Parts include the hierarchy fixups for all the other parts of the same group.
			uint32 NumHierarchyFixups = 0;
			for (uint32 i = 0; i < Page.PartsNum; i++)
			{
				const FClusterGroupPart& Part = Parts[Page.PartsStartIndex + i];
				const FClusterGroup& Group = Groups[Part.GroupIndex];
				const uint32 HierarchyRootOffset = 0;// Mesh.HierarchyRootOffsets[Group.MeshIndex];

				uint32 PageDependencyStart = Group.PageIndexStart;
				uint32 PageDependencyNum = Group.PageIndexNum;
				RemoveRootPagesFromRange(PageDependencyStart, PageDependencyNum, Mesh.NumRootPages);

				// Add fixups to all parts of the group
				for (uint32 j = 0; j < Group.PageIndexNum; j++)
				{
					const FPage& Page2 = Pages[Group.PageIndexStart + j];
					for (uint32 k = 0; k < Page2.PartsNum; k++)
					{
						const FClusterGroupPart& Part2 = Parts[Page2.PartsStartIndex + k];
						if (Part2.GroupIndex == Part.GroupIndex)
						{
							const uint32 GlobalHierarchyNodeIndex = HierarchyRootOffset + Part2.HierarchyNodeIndex;
							FixupChunk.GetHierarchyFixup(NumHierarchyFixups++) = FHierarchyFixup(Part2.PageIndex, GlobalHierarchyNodeIndex, Part2.HierarchyChildIndex, Part2.PageClusterOffset, PageDependencyStart, PageDependencyNum);
							break;
						}
					}
				}
			}
			TI_ASSERT(NumHierarchyFixups == FixupChunk.Header.NumHierachyFixups);
		}

		// Pack clusters and generate material range data
		TVector<uint32>				CombinedStripBitmaskData;
		TVector<uint32>				CombinedPageClusterPairData;
		TVector<uint32>				CombinedVertexRefBitmaskData;
		TVector<uint16>				CombinedVertexRefData;
		TVector<uint8>				CombinedIndexData;
		TVector<uint8>				CombinedPositionData;
		TVector<uint8>				CombinedAttributeData;
		TVector<uint32>				MaterialRangeData;
		TVector<uint16>				CodedVerticesPerCluster;
		TVector<uint32>				NumPositionBytesPerCluster;
		TVector<uint32>				NumPageClusterPairsPerCluster;
		TVector<FPackedCluster>		PackedClusters;

		PackedClusters.resize(Page.NumClusters);
		CodedVerticesPerCluster.resize(Page.NumClusters);
		NumPositionBytesPerCluster.resize(Page.NumClusters);
		NumPageClusterPairsPerCluster.resize(Page.NumClusters);

		const uint32 NumPackedClusterDwords = Page.NumClusters * sizeof(FPackedCluster) / sizeof(uint32);
		const uint32 MaterialTableStartOffsetInDwords = (NANITE_GPU_PAGE_HEADER_SIZE / 4) + NumPackedClusterDwords;

		FPageSections GpuSectionOffsets = Page.GpuSizes.GetOffsets();
		TMap<FVariableVertex, uint32> UniqueVertices;

		for (uint32 i = 0; i < Page.PartsNum; i++)
		{
			const FClusterGroupPart& Part = Parts[Page.PartsStartIndex + i];
			for (uint32 j = 0; j < (uint32)Part.Clusters.size(); j++)
			{
				const uint32 ClusterIndex = Part.Clusters[j];
				const FCluster& Cluster = Clusters[ClusterIndex];
				const FEncodingInfo& EncodingInfo = EncodingInfos[ClusterIndex];

				const uint32 LocalClusterIndex = Part.PageClusterOffset + j;
				FPackedCluster& PackedCluster = PackedClusters[LocalClusterIndex];
				PackCluster(PackedCluster, Cluster, EncodingInfos[ClusterIndex], NumTexCoords);

				PackedCluster.PackedMaterialInfo = PackMaterialInfo(Cluster, MaterialRangeData, MaterialTableStartOffsetInDwords);
				TI_ASSERT((GpuSectionOffsets.Index & 3) == 0);
				TI_ASSERT((GpuSectionOffsets.Position & 3) == 0);
				TI_ASSERT((GpuSectionOffsets.Attribute & 3) == 0);
				PackedCluster.SetIndexOffset(GpuSectionOffsets.Index);
				PackedCluster.SetPositionOffset(GpuSectionOffsets.Position);
				PackedCluster.SetAttributeOffset(GpuSectionOffsets.Attribute);
				PackedCluster.SetDecodeInfoOffset(GpuSectionOffsets.DecodeInfo);

				GpuSectionOffsets += EncodingInfo.GpuSizes;

				const FPageStreamingState& PageStreamingState = Mesh.PageStreamingStates[PageIndex];
				const uint32 DependenciesNum = (PageStreamingState.Flags & NANITE_PAGE_FLAG_RELATIVE_ENCODING) ? PageStreamingState.DependenciesNum : 0u;
				const TVectorView<uint32> PageDependencies = TVectorView<uint32>(Mesh.PageDependencies.data() + PageStreamingState.DependenciesStart, DependenciesNum);
				const uint32 PrevPositionBytes = (uint32)CombinedPositionData.size();
				const uint32 PrevPageClusterPairs = (uint32)CombinedPageClusterPairData.size();
				uint32 NumCodedVertices = 0;
				EncodeGeometryData(LocalClusterIndex, Cluster, EncodingInfo, NumTexCoords,
					CombinedStripBitmaskData, CombinedIndexData,
					CombinedPageClusterPairData, CombinedVertexRefBitmaskData, CombinedVertexRefData, CombinedPositionData, CombinedAttributeData,
					PageDependencies, PageVertexMaps,
					UniqueVertices, NumCodedVertices);

				NumPositionBytesPerCluster[LocalClusterIndex] = (uint32)CombinedPositionData.size() - PrevPositionBytes;
				NumPageClusterPairsPerCluster[LocalClusterIndex] = (uint32)CombinedPageClusterPairData.size() - PrevPageClusterPairs;
				CodedVerticesPerCluster[LocalClusterIndex] = NumCodedVertices;
			}
		}
		TI_ASSERT(GpuSectionOffsets.Cluster == Page.GpuSizes.GetMaterialTableOffset());
		TI_ASSERT(TMath::Align(GpuSectionOffsets.MaterialTable, 16) == Page.GpuSizes.GetDecodeInfoOffset());
		TI_ASSERT(GpuSectionOffsets.DecodeInfo == Page.GpuSizes.GetIndexOffset());
		TI_ASSERT(GpuSectionOffsets.Index == Page.GpuSizes.GetPositionOffset());
		TI_ASSERT(GpuSectionOffsets.Position == Page.GpuSizes.GetAttributeOffset());
		TI_ASSERT(GpuSectionOffsets.Attribute == Page.GpuSizes.GetTotal());

		// Dword align index data
		//CombinedIndexData.SetNumZeroed((CombinedIndexData.size() + 3) & -4);
		uint32 ZeroAdded = (CombinedIndexData.size() + 3) & -4;
		TI_ASSERT(ZeroAdded >= (uint32)CombinedIndexData.size());
		if (ZeroAdded > (uint32)CombinedIndexData.size())
		{
			CombinedIndexData.resize(ZeroAdded);
		}

		// Perform page-internal fix up directly on PackedClusters
		for (uint32 LocalPartIndex = 0; LocalPartIndex < Page.PartsNum; LocalPartIndex++)
		{
			const FClusterGroupPart& Part = Parts[Page.PartsStartIndex + LocalPartIndex];
			const FClusterGroup& Group = Groups[Part.GroupIndex];
			for (uint32 ClusterPositionInPart = 0; ClusterPositionInPart < (uint32)Part.Clusters.size(); ClusterPositionInPart++)
			{
				const FCluster& Cluster = Clusters[Part.Clusters[ClusterPositionInPart]];
				if (Cluster.GeneratingGroupIndex != INVALID_GROUP_INDEX)
				{
					const FClusterGroup& GeneratingGroup = Groups[Cluster.GeneratingGroupIndex];
					uint32 PageDependencyStart = GeneratingGroup.PageIndexStart;
					uint32 PageDependencyNum = GeneratingGroup.PageIndexNum;
					RemoveRootPagesFromRange(PageDependencyStart, PageDependencyNum, Mesh.NumRootPages);
					RemovePageFromRange(PageDependencyStart, PageDependencyNum, PageIndex);

					if (PageDependencyNum == 0)
					{
						// Dependencies already met by current page and/or root pages. Fixup directly.
						PackedClusters[Part.PageClusterOffset + ClusterPositionInPart].Flags &= ~NANITE_CLUSTER_FLAG_LEAF;	// Mark parent as no longer leaf
					}
				}
			}
		}

		// Begin page
		TVector<uint8>& PageResult = PageResults[PageIndex];
		PageResult.resize(NANITE_MAX_PAGE_DISK_SIZE);
		FBlockPointer PagePointer(PageResult.data(), (uint32)PageResult.size());

		// Disk header
		FPageDiskHeader* PageDiskHeader = PagePointer.Advance<FPageDiskHeader>(1);

		// 16-byte align material range data to make it easy to copy during GPU transcoding
		MaterialRangeData.resize(TMath::Align((uint32)MaterialRangeData.size(), 4));

		static_assert(sizeof(FPageGPUHeader) % 16 == 0, "sizeof(FGPUPageHeader) must be a multiple of 16");
		static_assert(sizeof(FUVRange) % 16 == 0, "sizeof(FUVRange) must be a multiple of 16");
		static_assert(sizeof(FPackedCluster) % 16 == 0, "sizeof(FPackedCluster) must be a multiple of 16");
		PageDiskHeader->NumClusters = Page.NumClusters;
		PageDiskHeader->GpuSize = Page.GpuSizes.GetTotal();
		PageDiskHeader->NumRawFloat4s = sizeof(FPageGPUHeader) / 16 + Page.NumClusters * (sizeof(FPackedCluster) + NumTexCoords * sizeof(FUVRange)) / 16 + (uint32)MaterialRangeData.size() / 4;
		PageDiskHeader->NumTexCoords = NumTexCoords;

		// Cluster headers
		FClusterDiskHeader* ClusterDiskHeaders = PagePointer.Advance<FClusterDiskHeader>(Page.NumClusters);

		// GPU page header
		FPageGPUHeader* GPUPageHeader = PagePointer.Advance<FPageGPUHeader>(1);
		*GPUPageHeader = FPageGPUHeader{};
		GPUPageHeader->NumClusters = Page.NumClusters;

		// Write clusters in SOA layout
		{
			const uint32 NumClusterFloat4Propeties = sizeof(FPackedCluster) / 16;
			for (uint32 float4Index = 0; float4Index < NumClusterFloat4Propeties; float4Index++)
			{
				for (const FPackedCluster& PackedCluster : PackedClusters)
				{
					uint8* Dst = PagePointer.Advance<uint8>(16);
					memcpy(Dst, (uint8*)&PackedCluster + float4Index * 16, 16);
				}
			}
		}

		// Material table
		uint32 MaterialTableSize = (uint32)MaterialRangeData.size() * sizeof(uint32);
		uint8* MaterialTable = PagePointer.Advance<uint8>(MaterialTableSize);
		memcpy(MaterialTable, MaterialRangeData.data(), MaterialTableSize);
		TI_ASSERT(MaterialTableSize == Page.GpuSizes.GetMaterialTableSize());

		// Decode information
		PageDiskHeader->DecodeInfoOffset = PagePointer.Offset();
		for (uint32 i = 0; i < Page.PartsNum; i++)
		{
			const FClusterGroupPart& Part = Parts[Page.PartsStartIndex + i];
			for (uint32 j = 0; j < (uint32)Part.Clusters.size(); j++)
			{
				const uint32 ClusterIndex = Part.Clusters[j];
				FUVRange* DecodeInfo = PagePointer.Advance<FUVRange>(NumTexCoords);
				for (uint32 k = 0; k < NumTexCoords; k++)
				{
					DecodeInfo[k] = EncodingInfos[ClusterIndex].UVRanges[k];
				}
			}
		}

		// Index data
		{
			uint8* IndexData = PagePointer.GetPtr<uint8>();
#if NANITE_USE_STRIP_INDICES
			for (uint32 i = 0; i < Page.PartsNum; i++)
			{
				const FClusterGroupPart& Part = Parts[Page.PartsStartIndex + i];
				for (uint32 j = 0; j < (uint32)Part.Clusters.size(); j++)
				{
					const uint32 LocalClusterIndex = Part.PageClusterOffset + j;
					const uint32 ClusterIndex = Part.Clusters[j];
					const FCluster& Cluster = Clusters[ClusterIndex];

					if ((Cluster.Flags & NANITE_CLUSTER_FLAG_NO_STRIPIFY) == 0)
					{
						ClusterDiskHeaders[LocalClusterIndex].IndexDataOffset = PagePointer.Offset();
						ClusterDiskHeaders[LocalClusterIndex].NumPrevNewVerticesBeforeDwords = Cluster.StripDesc.NumPrevNewVerticesBeforeDwords;
						ClusterDiskHeaders[LocalClusterIndex].NumPrevRefVerticesBeforeDwords = Cluster.StripDesc.NumPrevRefVerticesBeforeDwords;

						PagePointer.Advance<uint8>((uint32)Cluster.StripIndexData.size());
					}
					else
					{
						ClusterDiskHeaders[LocalClusterIndex].IndexDataOffset = PagePointer.Offset();
						PagePointer.Advance<uint8>(Cluster.NumTris * 3);
					}
				}
			}

			uint32 IndexDataSize = (uint32)CombinedIndexData.size() * sizeof(uint8);
			memcpy(IndexData, CombinedIndexData.data(), IndexDataSize);
			PagePointer.Align(sizeof(uint32));

			PageDiskHeader->StripBitmaskOffset = PagePointer.Offset();
			uint32 StripBitmaskDataSize = (uint32)CombinedStripBitmaskData.size() * sizeof(uint32);
			uint8* StripBitmaskData = PagePointer.Advance<uint8>(StripBitmaskDataSize);
			memcpy(StripBitmaskData, CombinedStripBitmaskData.data(), StripBitmaskDataSize);

#else
			for (uint32 i = 0; i < Page.NumClusters; i++)
			{
				ClusterDiskHeaders[i].IndexDataOffset = PagePointer.Offset();
				PagePointer.Advance<uint8>(PackedClusters[i].GetNumTris() * 3);
			}
			PagePointer.Align(sizeof(uint32));

			uint32 IndexDataSize = CombinedIndexData.size() * CombinedIndexData.GetTypeSize();
			memcpy(IndexData, CombinedIndexData.GetData(), IndexDataSize);
#endif
		}

		// Write PageCluster Map
		{
			uint8* PageClusterMapPtr = PagePointer.GetPtr<uint8>();
			for (uint32 i = 0; i < Page.NumClusters; i++)
			{
				ClusterDiskHeaders[i].PageClusterMapOffset = PagePointer.Offset();
				PagePointer.Advance<uint32>(NumPageClusterPairsPerCluster[i]);
			}
			TI_ASSERT((PagePointer.GetPtr<uint8>() - PageClusterMapPtr) == CombinedPageClusterPairData.size() * sizeof(uint32));
			memcpy(PageClusterMapPtr, CombinedPageClusterPairData.data(), CombinedPageClusterPairData.size() * sizeof(uint32));
		}

		// Write Vertex Reference Bitmask
		{
			PageDiskHeader->VertexRefBitmaskOffset = PagePointer.Offset();
			const uint32 VertexRefBitmaskSize = Page.NumClusters * (NANITE_MAX_CLUSTER_VERTICES / 8);
			uint8* VertexRefBitmask = PagePointer.Advance<uint8>(VertexRefBitmaskSize);
			memcpy(VertexRefBitmask, CombinedVertexRefBitmaskData.data(), VertexRefBitmaskSize);
			TI_ASSERT(CombinedVertexRefBitmaskData.size() * sizeof(uint32) == VertexRefBitmaskSize);
		}

		// Write Vertex References
		{
			PageDiskHeader->NumVertexRefs = (uint32)CombinedVertexRefData.size();

			uint8* VertexRefs = PagePointer.GetPtr<uint8>();
			for (uint32 i = 0; i < Page.NumClusters; i++)
			{
				ClusterDiskHeaders[i].VertexRefDataOffset = PagePointer.Offset();
				const uint32 NumVertexRefs = PackedClusters[i].GetNumVerts() - CodedVerticesPerCluster[i];
				ClusterDiskHeaders[i].NumVertexRefs = NumVertexRefs;
				PagePointer.Advance<uint8>(NumVertexRefs);
			}
			PagePointer.Advance<uint8>((uint32)CombinedVertexRefData.size());	// Low bytes
			PagePointer.Align(sizeof(uint32));

			// Split low and high bytes for better compression
			for (int32 i = 0; i < CombinedVertexRefData.size(); i++)
			{
				VertexRefs[i] = CombinedVertexRefData[i] >> 8;
				VertexRefs[i + CombinedVertexRefData.size()] = CombinedVertexRefData[i] & 0xFF;
			}
		}

		// Write Positions
		{
			uint8* PositionData = PagePointer.GetPtr<uint8>();
			for (uint32 i = 0; i < Page.NumClusters; i++)
			{
				ClusterDiskHeaders[i].PositionDataOffset = PagePointer.Offset();
				PagePointer.Advance<uint8>(NumPositionBytesPerCluster[i]);
			}
			TI_ASSERT((PagePointer.GetPtr<uint8>() - PositionData) == CombinedPositionData.size() * sizeof(uint8));

			memcpy(PositionData, CombinedPositionData.data(), CombinedPositionData.size() * sizeof(uint8));
		}

		// Write Attributes
		{
			uint8* AttribData = PagePointer.GetPtr<uint8>();
			for (uint32 i = 0; i < Page.NumClusters; i++)
			{
				const uint32 BytesPerAttribute = (PackedClusters[i].GetBitsPerAttribute() + 7) / 8;
				ClusterDiskHeaders[i].AttributeDataOffset = PagePointer.Offset();
				PagePointer.Advance<uint8>(TMath::Align(CodedVerticesPerCluster[i] * BytesPerAttribute, 4));
			}
			TI_ASSERT((uint32)(PagePointer.GetPtr<uint8>() - AttribData) == CombinedAttributeData.size() * sizeof(uint8));
			memcpy(AttribData, CombinedAttributeData.data(), CombinedAttributeData.size() * sizeof(uint8));
		}

		PageResult.resize(PagePointer.Offset());
	}

	// Write pages
	uint32 NumRootPages = 0;
	uint32 TotalRootGPUSize = 0;
	uint32 TotalRootDiskSize = 0;
	uint32 NumStreamingPages = 0;
	uint32 TotalStreamingGPUSize = 0;
	uint32 TotalStreamingDiskSize = 0;

	uint32 TotalFixupSize = 0;
	for (uint32 PageIndex = 0; PageIndex < NumPages; PageIndex++)
	{
		const FPage& Page = Pages[PageIndex];
		const bool bRootPage = Mesh.IsRootPage(PageIndex);
		FFixupChunk& FixupChunk = FixupChunks[PageIndex];
		TVector<uint8>& BulkData = bRootPage ? Mesh.RootData : StreamableBulkData;

		FPageStreamingState& PageStreamingState = Mesh.PageStreamingStates[PageIndex];
		PageStreamingState.BulkOffset = (uint32)BulkData.size();

		auto AppendToBulkData = [&BulkData](uint8* Data, uint32 Size)
		{
			uint32 Offset = (uint32)BulkData.size();
			BulkData.resize(Offset + Size);
			memcpy(BulkData.data() + Offset, Data, Size);
		};

		// Write fixup chunk
		uint32 FixupChunkSize = FixupChunk.GetSize();
		TI_ASSERT(FixupChunk.Header.NumHierachyFixups < NANITE_MAX_CLUSTERS_PER_PAGE);
		TI_ASSERT(FixupChunk.Header.NumClusterFixups < NANITE_MAX_CLUSTERS_PER_PAGE);
		AppendToBulkData((uint8*)&FixupChunk, FixupChunkSize);
		TotalFixupSize += FixupChunkSize;

		// Copy page to BulkData
		TVector<uint8>& PageData = PageResults[PageIndex];
		AppendToBulkData(PageData.data(), (uint32)PageData.size());

		if (bRootPage)
		{
			TotalRootGPUSize += Page.GpuSizes.GetTotal();
			TotalRootDiskSize += (uint32)PageData.size();
			NumRootPages++;
		}
		else
		{
			TotalStreamingGPUSize += Page.GpuSizes.GetTotal();
			TotalStreamingDiskSize += (uint32)PageData.size();
			NumStreamingPages++;
		}

		PageStreamingState.BulkSize = (uint32)BulkData.size() - PageStreamingState.BulkOffset;
		PageStreamingState.PageSize = (uint32)PageData.size();
	}

	const uint32 TotalPageGPUSize = TotalRootGPUSize + TotalStreamingGPUSize;
	const uint32 TotalPageDiskSize = TotalRootDiskSize + TotalStreamingDiskSize;
	_LOG(ELog::Log, "WritePages:\n", NumPages);
	_LOG(ELog::Log, "  Root: GPU size: %d bytes. %d Pages. %.3f bytes per page (%.3f%% utilization).\n", TotalRootGPUSize, NumRootPages, TotalRootGPUSize / (float)NumRootPages, TotalRootGPUSize / (float(NumRootPages) * NANITE_ROOT_PAGE_GPU_SIZE) * 100.0f);
	if (NumStreamingPages > 0)
	{
		_LOG(ELog::Log, "  Streaming: GPU size: %d bytes. %d Pages. %.3f bytes per page (%.3f%% utilization).\n", TotalStreamingGPUSize, NumStreamingPages, TotalStreamingGPUSize / float(NumStreamingPages), TotalStreamingGPUSize / (float(NumStreamingPages) * NANITE_STREAMING_PAGE_GPU_SIZE) * 100.0f);
	}
	else
	{
		_LOG(ELog::Log, "  Streaming: 0 bytes.\n");
	}
	_LOG(ELog::Log, "  Page data disk size: %d bytes. Fixup data size: %d bytes.\n", TotalPageDiskSize, TotalFixupSize);
	_LOG(ELog::Log, "  Total GPU size: %d bytes, Total disk size: %d bytes.\n", TotalPageGPUSize, TotalPageDiskSize + TotalFixupSize);

	// Store PageData
	Mesh.StreamablePages.swap(StreamableBulkData);
}

static uint32 CalculateMaxRootPages(uint32 TargetResidencyInKB)
{
	const uint64 SizeInBytes = uint64(TargetResidencyInKB) << 10;
	return (uint32)TMath::Clamp((SizeInBytes + NANITE_ROOT_PAGE_GPU_SIZE - 1u) >> NANITE_ROOT_PAGE_GPU_SIZE_BITS, 1llu, (uint64)MAX_uint32);
}

void Encode(
	TNaniteMesh& Mesh,
	TVector<FClusterGroup>& Groups, 
	TVector<FCluster>& ClusterSources,
	TVector< FClusterInstance>& ClusterInstances
)
{
	const uint32 MaxRootPages = CalculateMaxRootPages(0);

	BuildMaterialRanges(ClusterSources);

	ConstrainClusters(Groups, ClusterSources);

	TVector<FCluster> Clusters;
	ExpandClusters(ClusterSources, ClusterInstances, Clusters);
	FBox MeshBounds;
	for (FCluster& Cluster : Clusters)
	{
		MeshBounds.AddInternalBox(Cluster.Bounds);
	}

	Mesh.PositionPrecision = QuantizePositions(Clusters);

	TVector<FEncodingInfo> EncodingInfos;
	CalcEncodingInfoFromInstances(EncodingInfos, ClusterSources, Clusters, ClusterInstances, false, 1);
	 
	TVector<FPage> Pages;
	TVector<FClusterGroupPart> GroupParts;
	AssignClusterToPages(Groups, Clusters, EncodingInfos, Pages, GroupParts, MaxRootPages);
	Mesh.NumRootPages = TMath::Min((uint32)Pages.size(), MaxRootPages);

	BuildHierarchies(Groups, 1, GroupParts, Mesh.HierarchyNodes);

	WritePages(Mesh, Pages, Groups, GroupParts, Clusters, EncodingInfos, 1);
}