/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

struct RawVertex
{
	FFloat3 Pos;
	FFloat3 Nor;
	FFloat2 UV;
};

struct FMaterialTriangle
{
	uint32 Index0;
	uint32 Index1;
	uint32 Index2;
	uint32 MaterialIndex;
	uint32 RangeCount;
};

struct FMaterialRange
{
	uint32 RangeStart;
	uint32 RangeLength;
	uint32 MaterialIndex;

	//friend FArchive& operator<<(FArchive& Ar, FMaterialRange& Range);
};

struct FStripDesc
{
	uint32 Bitmasks[4][3];
	uint32 NumPrevRefVerticesBeforeDwords;
	uint32 NumPrevNewVerticesBeforeDwords;

	//friend FArchive& operator<<(FArchive& Ar, FStripDesc& Desc);
};

class FCluster
{
public:
	FCluster() {}
	void GenerateGUID(int32 _Id);

	void BuildCluster(
		const TVector<RawVertex>& InVerts,
		const TVector<int32>& InClusterIndexes,
		const TVector<int32>& InMaterialIndexes,
		int32 InNumTexCoords);
	void Bound();

private:

public:
	uint32 GetVertSize() const;
	FFloat3& GetPosition(uint32 VertIndex);
	float* GetAttributes(uint32 VertIndex);
	FFloat3& GetNormal(uint32 VertIndex);
	SColorf& GetColor(uint32 VertIndex);
	FFloat2* GetUVs(uint32 VertIndex);

	const FFloat3& GetPosition(uint32 VertIndex) const;
	const FFloat3& GetNormal(uint32 VertIndex) const;
	const SColorf& GetColor(uint32 VertIndex) const;
	const FFloat2* GetUVs(uint32 VertIndex) const;

	static const uint32	ClusterSize = 128;

	uint32 NumVerts = 0;
	uint32 NumTris = 0;
	uint32 NumTexCoords = 0;
	bool bHasColors = false;

	TVector< float > Verts;
	TVector< uint32 > Indexes;
	TVector< int32 > MaterialIndexes;
	TVector< int8 > ExternalEdges;
	uint32 NumExternalEdges;

	//TMap< uint32, uint32 >	AdjacentClusters;

	FBox Bounds;
	uint64 GUID = 0;
	uint32 _ID = 0;
	uint32 Flags = 0;
	int32 MipLevel = 0;

	FInt3 QuantizedPosStart = { 0u, 0u, 0u };
	int32 QuantizedPosPrecision = 0u;
	FInt3 QuantizedPosBits = { 0u, 0u, 0u };

	float EdgeLength = 0.0f;
	float LODError = 0.0f;
	float SurfaceArea = 0.0f;

	FSpheref SphereBounds;
	FSpheref LODBounds;

	uint32 GroupIndex = TNumLimit<uint32>::max();
	//uint32 GroupPartIndex = TNumLimit<uint32>::max();	// Seems not used after remove test
	uint32 GeneratingGroupIndex = TNumLimit<uint32>::max();

	TVector<FMaterialRange> MaterialRanges;
	TVector<FInt3> QuantizedPositions;

	FStripDesc StripDesc;
	TVector<uint8> StripIndexData;
};

FORCEINLINE uint32 FCluster::GetVertSize() const
{
	return 6 + (bHasColors ? 4 : 0) + NumTexCoords * 2;
}

FORCEINLINE FFloat3& FCluster::GetPosition(uint32 VertIndex)
{
	return *reinterpret_cast<FFloat3*>(&Verts[VertIndex * GetVertSize()]);
}

FORCEINLINE const FFloat3& FCluster::GetPosition(uint32 VertIndex) const
{
	return *reinterpret_cast<const FFloat3*>(&Verts[VertIndex * GetVertSize()]);
}

FORCEINLINE float* FCluster::GetAttributes(uint32 VertIndex)
{
	return &Verts[VertIndex * GetVertSize() + 3];
}

FORCEINLINE FFloat3& FCluster::GetNormal(uint32 VertIndex)
{
	return *reinterpret_cast<FFloat3*>(&Verts[VertIndex * GetVertSize() + 3]);
}

FORCEINLINE const FFloat3& FCluster::GetNormal(uint32 VertIndex) const
{
	return *reinterpret_cast<const FFloat3*>(&Verts[VertIndex * GetVertSize() + 3]);
}

FORCEINLINE SColorf& FCluster::GetColor(uint32 VertIndex)
{
	return *reinterpret_cast<SColorf*>(&Verts[VertIndex * GetVertSize() + 6]);
}

FORCEINLINE const SColorf& FCluster::GetColor(uint32 VertIndex) const
{
	return *reinterpret_cast<const SColorf*>(&Verts[VertIndex * GetVertSize() + 6]);
}

FORCEINLINE FFloat2* FCluster::GetUVs(uint32 VertIndex)
{
	return reinterpret_cast<FFloat2*>(&Verts[VertIndex * GetVertSize() + 6 + (bHasColors ? 4 : 0)]);
}

FORCEINLINE const FFloat2* FCluster::GetUVs(uint32 VertIndex) const
{
	return reinterpret_cast<const FFloat2*>(&Verts[VertIndex * GetVertSize() + 6 + (bHasColors ? 4 : 0)]);
}


struct FClusterInstance
{
	int32 ClusterId;
	bool IsInstanced;
	FMat4 Transform;
	float TransformedEdgeLength = 0.0f;
	float LODError = 0.0f;
	FBox TransformedBounds;
	FSpheref TransformedLODBounds;
	FSpheref TransformedSphereBounds;
	uint32 GroupIndex = TNumLimit<uint32>::max();
	uint32 GeneratingGroupIndex = TNumLimit<uint32>::max();

	FClusterInstance()
		: ClusterId(-1)
		, IsInstanced(false)
	{}

	FClusterInstance(int32 CId)
		: ClusterId(CId)
		, IsInstanced(false)
	{}

	FClusterInstance(int32 CId, const FMat4& InTransform)
		: ClusterId(CId)
		, IsInstanced(true)
		, Transform(InTransform)
	{}

};

struct FClusterGroup
{
	FSpheref Bounds;
	FSpheref LODBounds;
	float MinLODError;
	float MaxParentLODError;
	int32 MipLevel;
	uint32 MeshIndex;
	bool bTrimmed;

	uint32 PageIndexStart;
	uint32 PageIndexNum;
	TVector<uint32> Children;
};
