/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

// Class to simultaneously constrain and stripify a cluster
class FCluster;
class FStripifier
{
	static const uint32 MAX_CLUSTER_TRIANGLES_IN_DWORDS = (NANITE_MAX_CLUSTER_TRIANGLES + 31) / 32;
	static const uint32 INVALID_INDEX = 0xFFFFu;
	static const uint32 INVALID_CORNER = 0xFFFFu;
	static const uint32 INVALID_NODE = 0xFFFFu;
	static const uint32 INVALID_NODE_MEMSET = 0xFFu;

	uint32 VertexToTriangleMasks[NANITE_MAX_CLUSTER_TRIANGLES * 3][MAX_CLUSTER_TRIANGLES_IN_DWORDS];
	uint16 OppositeCorner[NANITE_MAX_CLUSTER_TRIANGLES * 3];
	float TrianglePriorities[NANITE_MAX_CLUSTER_TRIANGLES];

	class FContext
	{
	public:
		bool TriangleEnabled(uint32 TriangleIndex) const
		{
			return (TrianglesEnabled[TriangleIndex >> 5] & (1u << (TriangleIndex & 31u))) != 0u;
		}

		uint16 OldToNewVertex[NANITE_MAX_CLUSTER_TRIANGLES * 3];
		uint16 NewToOldVertex[NANITE_MAX_CLUSTER_TRIANGLES * 3];

		uint32 TrianglesEnabled[MAX_CLUSTER_TRIANGLES_IN_DWORDS];	// Enabled triangles are in the current material range and have not yet been visited.
		uint32 TrianglesTouched[MAX_CLUSTER_TRIANGLES_IN_DWORDS];	// Touched triangles have had at least one of their vertices visited.

		uint32 StripBitmasks[4][3];	// [4][Reset, IsLeft, IsRef]

		uint32 NumTriangles;
		uint32 NumVertices;
	};

	void BuildTables(const FCluster& Cluster);

public:
	void ConstrainAndStripifyCluster(FCluster& Cluster);
};