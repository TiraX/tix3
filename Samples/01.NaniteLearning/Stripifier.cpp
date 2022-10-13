/*
	TiX Engine v2.0 Copyright (C) 2018~2021
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "Stripifier.h"
#include "Cluster.h"

// Class to simultaneously constrain and stripify a cluster
#define CONSTRAINED_CLUSTER_CACHE_SIZE				32
static inline uint32 SetCorner(uint32 Triangle, uint32 LocalCorner)
{
	return (Triangle << 2) | LocalCorner;
}

static inline uint32 CornerToTriangle(uint32 Corner)
{
	return Corner >> 2;
}

static inline uint32 NextCorner(uint32 Corner)
{
	if ((Corner & 3) == 2)
		Corner &= ~3;
	else
		Corner++;
	return Corner;
}

static inline uint32 PrevCorner(uint32 Corner)
{
	if ((Corner & 3) == 0)
		Corner |= 2;
	else
		Corner--;
	return Corner;
}

static inline uint32 CornerToIndex(uint32 Corner)
{
	return (Corner >> 2) * 3 + (Corner & 3);
}

struct FStripifyWeights
{
	int32 Weights[2][2][2][2][CONSTRAINED_CLUSTER_CACHE_SIZE];
};

static const FStripifyWeights DefaultStripifyWeights = {
	{
		{
			{
				{
					// IsStart=0, HasOpposite=0, HasLeft=0, HasRight=0
					{  142,  124,  131,  184,  138,  149,  148,  127,  154,  148,  152,  133,  133,  132,  170,  141,  109,  148,  138,  117,  126,  112,  144,  126,  116,  139,  122,  141,  122,  133,  134,  137 },
					// IsStart=0, HasOpposite=0, HasLeft=0, HasRight=1
					{  128,  144,  134,  122,  130,  133,  129,  122,  128,  107,  127,  126,   89,  135,   88,  130,   94,  134,  103,  118,  128,   96,   90,  139,   89,  139,  113,  100,  119,  131,  113,  121 },
				},
				{
					// IsStart=0, HasOpposite=0, HasLeft=1, HasRight=0
					{  128,  144,  134,  129,  110,  142,  111,  140,  116,  139,   98,  110,  125,  143,  122,  109,  127,  154,  113,  119,  126,  131,  123,  127,   93,  118,  101,   93,  131,  139,  130,  139 },
					// IsStart=0, HasOpposite=0, HasLeft=1, HasRight=1
					{  120,  128,  137,  105,  113,  121,  120,  120,  112,  117,  124,  129,  129,   98,  137,  133,  122,  159,  141,  104,  129,  119,   98,  111,  110,  115,  114,  125,  115,  140,  109,  137 },
				}
			},
			{
				{
					// IsStart=0, HasOpposite=1, HasLeft=0, HasRight=0
					{  128,  137,  154,  169,  140,  162,  156,  157,  164,  144,  171,  145,  148,  146,  124,  138,  144,  158,  140,  137,  141,  145,  140,  148,  110,  160,  128,  129,  144,  155,  125,  123 },
					// IsStart=0, HasOpposite=1, HasLeft=0, HasRight=1
					{  124,  115,  136,  131,  145,  143,  159,  144,  158,  165,  128,  191,  135,  173,  147,  137,  128,  163,  164,  151,  162,  178,  161,  143,  168,  166,  122,  160,  170,  175,  132,  109 },
				},
				{
					// IsStart=0, HasOpposite=1, HasLeft=1, HasRight=0
					{  134,  112,  132,  123,  126,  138,  148,  138,  145,  136,  146,  133,  141,  165,  139,  145,  119,  167,  135,  120,  146,  120,  117,  136,  102,  156,  128,  120,  132,  143,   91,  136 },
					// IsStart=0, HasOpposite=1, HasLeft=1, HasRight=1
					{  140,   95,  118,  117,  127,  102,  119,  119,  134,  107,  135,  128,  109,  133,  120,  122,  132,  150,  152,  119,  128,  137,  119,  128,  131,  165,  156,  143,  135,  134,  135,  154 },
				}
			}
		},
		{
			{
				{
					// IsStart=1, HasOpposite=0, HasLeft=0, HasRight=0
					{  139,  132,  139,  133,  130,  134,  135,  131,  133,  139,  141,  139,  132,  136,  139,  150,  140,  137,  143,  157,  149,  157,  168,  155,  159,  181,  176,  185,  219,  167,  133,  143 },
					// IsStart=1, HasOpposite=0, HasLeft=0, HasRight=1
					{  125,  127,  126,  131,  128,  114,  130,  126,  129,  131,  125,  127,  131,  126,  137,  129,  140,   99,  142,   99,  149,  121,  155,  118,  131,  156,  168,  144,  175,  155,  112,  129 },
				},
				{
					// IsStart=1, HasOpposite=0, HasLeft=1, HasRight=0
					{  129,  129,  128,  128,  128,  129,  128,  129,  130,  127,  131,  130,  131,  130,  134,  133,  136,  134,  134,  138,  144,  139,  137,  154,  147,  141,  175,  214,  140,  140,  130,  122 },
					// IsStart=1, HasOpposite=0, HasLeft=1, HasRight=1
					{  128,  128,  124,  123,  125,  107,  127,  128,  125,  128,  128,  128,  128,  128,  128,  130,  107,  124,  136,  119,  139,  127,  132,  140,  125,  150,  133,  150,  138,  130,  127,  127 },
				}
			},
			{
				{
					// IsStart=1, HasOpposite=1, HasLeft=0, HasRight=0
					{  104,  125,  126,  129,  126,  122,  128,  126,  126,  127,  125,  122,  130,  126,  130,  131,  130,  132,  118,  101,  119,  121,  143,  114,  122,  145,  132,  144,  116,  142,  114,  127 },
					// IsStart=1, HasOpposite=1, HasLeft=0, HasRight=1
					{  128,  124,   93,  126,  108,  128,  127,  122,  128,  126,  128,  123,   92,  125,   98,   99,  127,  131,  126,  128,  121,  133,  113,  121,  122,  137,  145,  138,  137,  109,  129,  100 },
				},
				{
					// IsStart=1, HasOpposite=1, HasLeft=1, HasRight=0
					{  119,  128,  122,  128,  127,  123,  126,  128,  126,  122,  120,  127,  128,  122,  130,  121,  138,  122,  136,  130,  133,  124,  139,  134,  138,  118,  139,  145,  132,  122,  124,   86 },
					// IsStart=1, HasOpposite=1, HasLeft=1, HasRight=1
					{  116,  124,  119,  126,  118,  113,  114,  125,  128,  111,  129,  122,  129,  129,  135,  130,  138,  132,  115,  138,  114,  119,  122,  136,  138,  128,  141,  119,  139,  119,  130,  128 },
				}
			}
		}
	}
};


static uint32 countbits(uint32 x)
{
	return TMath::CountBits64(x);
}

static uint32 firstbithigh(uint32 x)
{
	return TMath::FloorLog2(x);
}

static int32 BitFieldExtractI32(int32 Data, int32 NumBits, int32 StartBit)
{
	return (Data << (32 - StartBit - NumBits)) >> (32 - NumBits);
}

static uint32 BitFieldExtractU32(uint32 Data, int32 NumBits, int32 StartBit)
{
	return (Data << (32 - StartBit - NumBits)) >> (32 - NumBits);
}

static uint32 ReadUnalignedDword(const uint8* SrcPtr, int32 BitOffset)	// Note: Only guarantees 25 valid bits
{
	if (BitOffset < 0)
	{
		// Workaround for reading slightly out of bounds
		TI_ASSERT(BitOffset > -8);
		return *(const uint32*)(SrcPtr) << (8 - (BitOffset & 7));
	}
	else
	{
		const uint32* DwordPtr = (const uint32*)(SrcPtr + (BitOffset >> 3));
		return *DwordPtr >> (BitOffset & 7);
	}
}

static void UnpackTriangleIndices(const FStripDesc& StripDesc, const uint8* StripIndexData, uint32 TriIndex, uint32* OutIndices)
{
	const uint32 DwordIndex = TriIndex >> 5;
	const uint32 BitIndex = TriIndex & 31u;

	//Bitmask.x: bIsStart, Bitmask.y: bIsRight, Bitmask.z: bIsNewVertex
	const uint32 SMask = StripDesc.Bitmasks[DwordIndex][0];
	const uint32 LMask = StripDesc.Bitmasks[DwordIndex][1];
	const uint32 WMask = StripDesc.Bitmasks[DwordIndex][2];
	const uint32 SLMask = SMask & LMask;

	//const uint HeadRefVertexMask = ( SMask & LMask & WMask ) | ( ~SMask & WMask );
	const uint32 HeadRefVertexMask = (SLMask | ~SMask) & WMask;	// 1 if head of triangle is ref. S case with 3 refs or L/R case with 1 ref.

	const uint32 PrevBitsMask = (1u << BitIndex) - 1u;
	const uint32 NumPrevRefVerticesBeforeDword = DwordIndex ? BitFieldExtractU32(StripDesc.NumPrevRefVerticesBeforeDwords, 10u, DwordIndex * 10u - 10u) : 0u;
	const uint32 NumPrevNewVerticesBeforeDword = DwordIndex ? BitFieldExtractU32(StripDesc.NumPrevNewVerticesBeforeDwords, 10u, DwordIndex * 10u - 10u) : 0u;

	int32 CurrentDwordNumPrevRefVertices = (countbits(SLMask & PrevBitsMask) << 1) + countbits(WMask & PrevBitsMask);
	int32 CurrentDwordNumPrevNewVertices = (countbits(SMask & PrevBitsMask) << 1) + BitIndex - CurrentDwordNumPrevRefVertices;

	int32 NumPrevRefVertices = NumPrevRefVerticesBeforeDword + CurrentDwordNumPrevRefVertices;
	int32 NumPrevNewVertices = NumPrevNewVerticesBeforeDword + CurrentDwordNumPrevNewVertices;

	const int32 IsStart = BitFieldExtractI32(SMask, 1, BitIndex);		// -1: true, 0: false
	const int32 IsLeft = BitFieldExtractI32(LMask, 1, BitIndex);		// -1: true, 0: false
	const int32 IsRef = BitFieldExtractI32(WMask, 1, BitIndex);		// -1: true, 0: false

	const uint32 BaseVertex = NumPrevNewVertices - 1u;

	uint32 IndexData = ReadUnalignedDword(StripIndexData, (NumPrevRefVertices + ~IsStart) * 5);	// -1 if not Start

	if (IsStart)
	{
		const int32 MinusNumRefVertices = (IsLeft << 1) + IsRef;
		uint32 NextVertex = NumPrevNewVertices;

		if (MinusNumRefVertices <= -1) { OutIndices[0] = BaseVertex - (IndexData & 31u); IndexData >>= 5; }
		else { OutIndices[0] = NextVertex++; }
		if (MinusNumRefVertices <= -2) { OutIndices[1] = BaseVertex - (IndexData & 31u); IndexData >>= 5; }
		else { OutIndices[1] = NextVertex++; }
		if (MinusNumRefVertices <= -3) { OutIndices[2] = BaseVertex - (IndexData & 31u); }
		else { OutIndices[2] = NextVertex++; }
	}
	else
	{
		// Handle two first vertices
		const uint32 PrevBitIndex = BitIndex - 1u;
		const int32 IsPrevStart = BitFieldExtractI32(SMask, 1, PrevBitIndex);
		const int32 IsPrevHeadRef = BitFieldExtractI32(HeadRefVertexMask, 1, PrevBitIndex);
		//const int NumPrevNewVerticesInTriangle = IsPrevStart ? ( 3u - ( bfe_u32( /*SLMask*/ LMask, PrevBitIndex, 1 ) << 1 ) - bfe_u32( /*SMask &*/ WMask, PrevBitIndex, 1 ) ) : /*1u - IsPrevRefVertex*/ 0u;
		const int32 NumPrevNewVerticesInTriangle = IsPrevStart & (3u - ((BitFieldExtractU32( /*SLMask*/ LMask, 1, PrevBitIndex) << 1) | BitFieldExtractU32( /*SMask &*/ WMask, 1, PrevBitIndex)));

		//OutIndices[ 1 ] = IsPrevRefVertex ? ( BaseVertex - ( IndexData & 31u ) + NumPrevNewVerticesInTriangle ) : BaseVertex;	// BaseVertex = ( NumPrevNewVertices - 1 );
		OutIndices[1] = BaseVertex + (IsPrevHeadRef & (NumPrevNewVerticesInTriangle - (IndexData & 31u)));
		//OutIndices[ 2 ] = IsRefVertex ? ( BaseVertex - bfe_u32( IndexData, 5, 5 ) ) : NumPrevNewVertices;
		OutIndices[2] = NumPrevNewVertices + (IsRef & (-1 - BitFieldExtractU32(IndexData, 5, 5)));

		// We have to search for the third vertex. 
		// Left triangles search for previous Right/Start. Right triangles search for previous Left/Start.
		const uint32 SearchMask = SMask | (LMask ^ IsLeft);				// SMask | ( IsRight ? LMask : RMask );
		const uint32 FoundBitIndex = firstbithigh(SearchMask & PrevBitsMask);
		const int32 IsFoundCaseS = BitFieldExtractI32(SMask, 1, FoundBitIndex);		// -1: true, 0: false

		const uint32 FoundPrevBitsMask = (1u << FoundBitIndex) - 1u;
		int32 FoundCurrentDwordNumPrevRefVertices = (countbits(SLMask & FoundPrevBitsMask) << 1) + countbits(WMask & FoundPrevBitsMask);
		int32 FoundCurrentDwordNumPrevNewVertices = (countbits(SMask & FoundPrevBitsMask) << 1) + FoundBitIndex - FoundCurrentDwordNumPrevRefVertices;

		int32 FoundNumPrevNewVertices = NumPrevNewVerticesBeforeDword + FoundCurrentDwordNumPrevNewVertices;
		int32 FoundNumPrevRefVertices = NumPrevRefVerticesBeforeDword + FoundCurrentDwordNumPrevRefVertices;

		const uint32 FoundNumRefVertices = (BitFieldExtractU32(LMask, 1, FoundBitIndex) << 1) + BitFieldExtractU32(WMask, 1, FoundBitIndex);
		const uint32 IsBeforeFoundRefVertex = BitFieldExtractU32(HeadRefVertexMask, 1, FoundBitIndex - 1);

		// ReadOffset: Where is the vertex relative to triangle we searched for?
		const int32 ReadOffset = IsFoundCaseS ? IsLeft : 1;
		const uint32 FoundIndexData = ReadUnalignedDword(StripIndexData, (FoundNumPrevRefVertices - ReadOffset) * 5);
		const uint32 FoundIndex = (FoundNumPrevNewVertices - 1u) - BitFieldExtractU32(FoundIndexData, 5, 0);

		bool bCondition = IsFoundCaseS ? ((int32)FoundNumRefVertices >= 1 - IsLeft) : (IsBeforeFoundRefVertex != 0u);
		int32 FoundNewVertex = FoundNumPrevNewVertices + (IsFoundCaseS ? (IsLeft & (FoundNumRefVertices == 0)) : -1);
		OutIndices[0] = bCondition ? FoundIndex : FoundNewVertex;

		// Would it be better to code New verts instead of Ref verts?
		// HeadRefVertexMask would just be WMask?

		// TODO: could we do better with non-generalized strips?

		/*
		if( IsFoundCaseS )
		{
			if( IsRight )
			{
				OutIndices[ 0 ] = ( FoundNumRefVertices >= 1 ) ? FoundIndex : FoundNumPrevNewVertices;
				// OutIndices[ 0 ] = ( FoundNumRefVertices >= 1 ) ? ( FoundBaseVertex - Cluster.StripIndices[ FoundNumPrevRefVertices ] ) : FoundNumPrevNewVertices;
			}
			else
			{
				OutIndices[ 0 ] = ( FoundNumRefVertices >= 2 ) ? FoundIndex : ( FoundNumPrevNewVertices + ( FoundNumRefVertices == 0 ? 1 : 0 ) );
				// OutIndices[ 0 ] = ( FoundNumRefVertices >= 2 ) ? ( FoundBaseVertex - Cluster.StripIndices[ FoundNumPrevRefVertices + 1 ] ) : ( FoundNumPrevNewVertices + ( FoundNumRefVertices == 0 ? 1 : 0 ) );
			}
		}
		else
		{
			OutIndices[ 0 ] = IsBeforeFoundRefVertex ? FoundIndex : ( FoundNumPrevNewVertices - 1 );
			// OutIndices[ 0 ] = IsBeforeFoundRefVertex ? ( FoundBaseVertex - Cluster.StripIndices[ FoundNumPrevRefVertices - 1 ] ) : ( FoundNumPrevNewVertices - 1 );
		}
		*/

		if (IsLeft)
		{
			// swap
			std::swap(OutIndices[1], OutIndices[2]);
		}
		TI_ASSERT(OutIndices[0] != OutIndices[1] && OutIndices[0] != OutIndices[2] && OutIndices[1] != OutIndices[2]);
	}
}

void FStripifier::BuildTables(const FCluster& Cluster)
{
	struct FEdgeNode
	{
		uint16 Corner;	// (Triangle << 2) | LocalCorner
		uint16 NextNode;
	};

	FEdgeNode EdgeNodes[NANITE_MAX_CLUSTER_INDICES];
	uint16 EdgeNodeHeads[NANITE_MAX_CLUSTER_INDICES * NANITE_MAX_CLUSTER_INDICES];	// Linked list per edge to support more than 2 triangles per edge.
	memset(EdgeNodeHeads, INVALID_NODE_MEMSET, sizeof(EdgeNodeHeads));

	memset(VertexToTriangleMasks, 0, sizeof(VertexToTriangleMasks));

	uint32 NumTriangles = Cluster.NumTris;
	uint32 NumVertices = Cluster.NumVerts;

	// Add triangles to edge lists and update valence
	for (uint32 i = 0; i < NumTriangles; i++)
	{
		uint32 i0 = Cluster.Indexes[i * 3 + 0];
		uint32 i1 = Cluster.Indexes[i * 3 + 1];
		uint32 i2 = Cluster.Indexes[i * 3 + 2];
		TI_ASSERT(i0 != i1 && i1 != i2 && i2 != i0);
		TI_ASSERT(i0 < NumVertices&& i1 < NumVertices&& i2 < NumVertices);

		VertexToTriangleMasks[i0][i >> 5] |= 1 << (i & 31);
		VertexToTriangleMasks[i1][i >> 5] |= 1 << (i & 31);
		VertexToTriangleMasks[i2][i >> 5] |= 1 << (i & 31);

		FFloat3 ScaledCenter = Cluster.GetPosition(i0) + Cluster.GetPosition(i1) + Cluster.GetPosition(i2);
		TrianglePriorities[i] = ScaledCenter.X;	//TODO: Find a good direction to sort by instead of just picking x?

		FEdgeNode& Node0 = EdgeNodes[i * 3 + 0];
		Node0.Corner = SetCorner(i, 0);
		Node0.NextNode = EdgeNodeHeads[i1 * NANITE_MAX_CLUSTER_INDICES + i2];
		EdgeNodeHeads[i1 * NANITE_MAX_CLUSTER_INDICES + i2] = i * 3 + 0;

		FEdgeNode& Node1 = EdgeNodes[i * 3 + 1];
		Node1.Corner = SetCorner(i, 1);
		Node1.NextNode = EdgeNodeHeads[i2 * NANITE_MAX_CLUSTER_INDICES + i0];
		EdgeNodeHeads[i2 * NANITE_MAX_CLUSTER_INDICES + i0] = i * 3 + 1;

		FEdgeNode& Node2 = EdgeNodes[i * 3 + 2];
		Node2.Corner = SetCorner(i, 2);
		Node2.NextNode = EdgeNodeHeads[i0 * NANITE_MAX_CLUSTER_INDICES + i1];
		EdgeNodeHeads[i0 * NANITE_MAX_CLUSTER_INDICES + i1] = i * 3 + 2;
	}

	// Gather adjacency from edge lists	
	for (uint32 i = 0; i < NumTriangles; i++)
	{
		uint32 i0 = Cluster.Indexes[i * 3 + 0];
		uint32 i1 = Cluster.Indexes[i * 3 + 1];
		uint32 i2 = Cluster.Indexes[i * 3 + 2];

		uint16& Node0 = EdgeNodeHeads[i2 * NANITE_MAX_CLUSTER_INDICES + i1];
		uint16& Node1 = EdgeNodeHeads[i0 * NANITE_MAX_CLUSTER_INDICES + i2];
		uint16& Node2 = EdgeNodeHeads[i1 * NANITE_MAX_CLUSTER_INDICES + i0];
		if (Node0 != INVALID_NODE) { OppositeCorner[i * 3 + 0] = EdgeNodes[Node0].Corner; Node0 = EdgeNodes[Node0].NextNode; }
		else { OppositeCorner[i * 3 + 0] = INVALID_CORNER; }
		if (Node1 != INVALID_NODE) { OppositeCorner[i * 3 + 1] = EdgeNodes[Node1].Corner; Node1 = EdgeNodes[Node1].NextNode; }
		else { OppositeCorner[i * 3 + 1] = INVALID_CORNER; }
		if (Node2 != INVALID_NODE) { OppositeCorner[i * 3 + 2] = EdgeNodes[Node2].Corner; Node2 = EdgeNodes[Node2].NextNode; }
		else { OppositeCorner[i * 3 + 2] = INVALID_CORNER; }
	}

	// Generate vertex to triangle masks
	for (uint32 i = 0; i < NumTriangles; i++)
	{
		uint32 i0 = Cluster.Indexes[i * 3 + 0];
		uint32 i1 = Cluster.Indexes[i * 3 + 1];
		uint32 i2 = Cluster.Indexes[i * 3 + 2];
		TI_ASSERT(i0 != i1 && i1 != i2 && i2 != i0);

		VertexToTriangleMasks[i0][i >> 5] |= 1 << (i & 31);
		VertexToTriangleMasks[i1][i >> 5] |= 1 << (i & 31);
		VertexToTriangleMasks[i2][i >> 5] |= 1 << (i & 31);
	}
}

void FStripifier::ConstrainAndStripifyCluster(FCluster& Cluster)
{
	const FStripifyWeights& Weights = DefaultStripifyWeights;
	uint32 NumOldTriangles = Cluster.NumTris;
	uint32 NumOldVertices = Cluster.NumVerts;

	BuildTables(Cluster);

	uint32 NumStrips = 0;

	FContext Context = {};
	memset(Context.OldToNewVertex, -1, sizeof(Context.OldToNewVertex));

	auto NewScoreVertex = [&Weights](const FContext& Context, uint32 OldVertex, bool bStart, bool bHasOpposite, bool bHasLeft, bool bHasRight)
	{
		uint16 NewIndex = Context.OldToNewVertex[OldVertex];

		int32 CacheScore = 0;
		if (NewIndex != INVALID_INDEX)
		{
			uint32 CachePosition = (Context.NumVertices - 1) - NewIndex;
			if (CachePosition < CONSTRAINED_CLUSTER_CACHE_SIZE)
				CacheScore = Weights.Weights[bStart][bHasOpposite][bHasLeft][bHasRight][CachePosition];
		}

		return CacheScore;
	};

	auto NewScoreTriangle = [&Cluster, &NewScoreVertex](const FContext& Context, uint32 TriangleIndex, bool bStart, bool bHasOpposite, bool bHasLeft, bool bHasRight)
	{
		const uint32 OldIndex0 = Cluster.Indexes[TriangleIndex * 3 + 0];
		const uint32 OldIndex1 = Cluster.Indexes[TriangleIndex * 3 + 1];
		const uint32 OldIndex2 = Cluster.Indexes[TriangleIndex * 3 + 2];

		return	NewScoreVertex(Context, OldIndex0, bStart, bHasOpposite, bHasLeft, bHasRight) +
			NewScoreVertex(Context, OldIndex1, bStart, bHasOpposite, bHasLeft, bHasRight) +
			NewScoreVertex(Context, OldIndex2, bStart, bHasOpposite, bHasLeft, bHasRight);
	};

	auto VisitTriangle = [this, &Cluster](FContext& Context, uint32 TriangleCorner, bool bStart, bool bRight)
	{
		const uint32 OldIndex0 = Cluster.Indexes[CornerToIndex(NextCorner(TriangleCorner))];
		const uint32 OldIndex1 = Cluster.Indexes[CornerToIndex(PrevCorner(TriangleCorner))];
		const uint32 OldIndex2 = Cluster.Indexes[CornerToIndex(TriangleCorner)];

		// Mark incident triangles
		for (uint32 i = 0; i < MAX_CLUSTER_TRIANGLES_IN_DWORDS; i++)
		{
			Context.TrianglesTouched[i] |= VertexToTriangleMasks[OldIndex0][i] | VertexToTriangleMasks[OldIndex1][i] | VertexToTriangleMasks[OldIndex2][i];
		}

		uint16& NewIndex0 = Context.OldToNewVertex[OldIndex0];
		uint16& NewIndex1 = Context.OldToNewVertex[OldIndex1];
		uint16& NewIndex2 = Context.OldToNewVertex[OldIndex2];

		uint32 OrgIndex0 = NewIndex0;
		uint32 OrgIndex1 = NewIndex1;
		uint32 OrgIndex2 = NewIndex2;

		uint32 NextVertexIndex = Context.NumVertices + (NewIndex0 == INVALID_INDEX) + (NewIndex1 == INVALID_INDEX) + (NewIndex2 == INVALID_INDEX);
		while (true)
		{
			if (NewIndex0 != INVALID_INDEX && NextVertexIndex - NewIndex0 >= CONSTRAINED_CLUSTER_CACHE_SIZE) { NewIndex0 = INVALID_INDEX; NextVertexIndex++; continue; }
			if (NewIndex1 != INVALID_INDEX && NextVertexIndex - NewIndex1 >= CONSTRAINED_CLUSTER_CACHE_SIZE) { NewIndex1 = INVALID_INDEX; NextVertexIndex++; continue; }
			if (NewIndex2 != INVALID_INDEX && NextVertexIndex - NewIndex2 >= CONSTRAINED_CLUSTER_CACHE_SIZE) { NewIndex2 = INVALID_INDEX; NextVertexIndex++; continue; }
			break;
		}

		uint32 NewTriangleIndex = Context.NumTriangles;
		uint32 NumNewVertices = (NewIndex0 == INVALID_INDEX) + (NewIndex1 == INVALID_INDEX) + (NewIndex2 == INVALID_INDEX);
		if (bStart)
		{
			TI_ASSERT((NewIndex2 == INVALID_INDEX) >= (NewIndex1 == INVALID_INDEX));
			TI_ASSERT((NewIndex1 == INVALID_INDEX) >= (NewIndex0 == INVALID_INDEX));


			uint32 NumWrittenIndices = 3u - NumNewVertices;
			uint32 LowBit = NumWrittenIndices & 1u;
			uint32 HighBit = (NumWrittenIndices >> 1) & 1u;

			Context.StripBitmasks[NewTriangleIndex >> 5][0] |= (1u << (NewTriangleIndex & 31u));
			Context.StripBitmasks[NewTriangleIndex >> 5][1] |= (HighBit << (NewTriangleIndex & 31u));
			Context.StripBitmasks[NewTriangleIndex >> 5][2] |= (LowBit << (NewTriangleIndex & 31u));
		}
		else
		{
			TI_ASSERT(NewIndex0 != INVALID_INDEX);
			TI_ASSERT(NewIndex1 != INVALID_INDEX);
			if (!bRight)
			{
				Context.StripBitmasks[NewTriangleIndex >> 5][1] |= (1 << (NewTriangleIndex & 31u));
			}

			if (NewIndex2 != INVALID_INDEX)
			{
				Context.StripBitmasks[NewTriangleIndex >> 5][2] |= (1 << (NewTriangleIndex & 31u));
			}
		}

		if (NewIndex0 == INVALID_INDEX) { NewIndex0 = Context.NumVertices++; Context.NewToOldVertex[NewIndex0] = OldIndex0; }
		if (NewIndex1 == INVALID_INDEX) { NewIndex1 = Context.NumVertices++; Context.NewToOldVertex[NewIndex1] = OldIndex1; }
		if (NewIndex2 == INVALID_INDEX) { NewIndex2 = Context.NumVertices++; Context.NewToOldVertex[NewIndex2] = OldIndex2; }

		// Output triangle
		Context.NumTriangles++;

		// Disable selected triangle
		const uint32 OldTriangleIndex = CornerToTriangle(TriangleCorner);
		Context.TrianglesEnabled[OldTriangleIndex >> 5] &= ~(1 << (OldTriangleIndex & 31u));
		return NumNewVertices;
	};

	//Cluster.StripIndexData.Empty();
	Cluster.StripIndexData.clear();
	TBitWriter BitWriter(Cluster.StripIndexData);
	FStripDesc& StripDesc = Cluster.StripDesc;
	memset(&StripDesc, 0, sizeof(FStripDesc));
	uint32 NumNewVerticesInDword[4] = {};
	uint32 NumRefVerticesInDword[4] = {};

	uint32 RangeStart = 0;
	for (const FMaterialRange& MaterialRange : Cluster.MaterialRanges)
	{
		TI_ASSERT(RangeStart == MaterialRange.RangeStart);
		uint32 RangeLength = MaterialRange.RangeLength;

		// Enable triangles from current range
		for (uint32 i = 0; i < MAX_CLUSTER_TRIANGLES_IN_DWORDS; i++)
		{
			int32 RangeStartRelativeToDword = (int32)RangeStart - (int32)i * 32;
			int32 BitStart = TMath::Max(RangeStartRelativeToDword, 0);
			int32 BitEnd = TMath::Max(RangeStartRelativeToDword + (int32)RangeLength, 0);
			uint32 StartMask = BitStart < 32 ? ((1u << BitStart) - 1u) : 0xFFFFFFFFu;
			uint32 EndMask = BitEnd < 32 ? ((1u << BitEnd) - 1u) : 0xFFFFFFFFu;
			Context.TrianglesEnabled[i] |= StartMask ^ EndMask;
		}

		// While a strip can be started
		while (true)
		{
			// Pick a start location for the strip
			uint32 StartCorner = INVALID_CORNER;
			int32 BestScore = -1;
			float BestPriority = INT_MIN;
			{
				for (uint32 TriangleDwordIndex = 0; TriangleDwordIndex < MAX_CLUSTER_TRIANGLES_IN_DWORDS; TriangleDwordIndex++)
				{
					uint32 CandidateMask = Context.TrianglesEnabled[TriangleDwordIndex];
					while (CandidateMask)
					{
						uint32 TriangleIndex = (TriangleDwordIndex << 5) + TMath::CountTrailingZeros(CandidateMask);
						CandidateMask &= CandidateMask - 1u;

						for (uint32 Corner = 0; Corner < 3; Corner++)
						{
							uint32 TriangleCorner = SetCorner(TriangleIndex, Corner);

							{
								// Is it viable WRT the constraint that new vertices should always be at the end.
								uint32 OldIndex0 = Cluster.Indexes[CornerToIndex(NextCorner(TriangleCorner))];
								uint32 OldIndex1 = Cluster.Indexes[CornerToIndex(PrevCorner(TriangleCorner))];
								uint32 OldIndex2 = Cluster.Indexes[CornerToIndex(TriangleCorner)];

								uint32 NewIndex0 = Context.OldToNewVertex[OldIndex0];
								uint32 NewIndex1 = Context.OldToNewVertex[OldIndex1];
								uint32 NewIndex2 = Context.OldToNewVertex[OldIndex2];
								uint32 NumVerts = Context.NumVertices + (NewIndex0 == INVALID_INDEX) + (NewIndex1 == INVALID_INDEX) + (NewIndex2 == INVALID_INDEX);
								while (true)
								{
									if (NewIndex0 != INVALID_INDEX && NumVerts - NewIndex0 >= CONSTRAINED_CLUSTER_CACHE_SIZE) { NewIndex0 = INVALID_INDEX; NumVerts++; continue; }
									if (NewIndex1 != INVALID_INDEX && NumVerts - NewIndex1 >= CONSTRAINED_CLUSTER_CACHE_SIZE) { NewIndex1 = INVALID_INDEX; NumVerts++; continue; }
									if (NewIndex2 != INVALID_INDEX && NumVerts - NewIndex2 >= CONSTRAINED_CLUSTER_CACHE_SIZE) { NewIndex2 = INVALID_INDEX; NumVerts++; continue; }
									break;
								}

								uint32 Mask = (NewIndex0 == INVALID_INDEX ? 1u : 0u) | (NewIndex1 == INVALID_INDEX ? 2u : 0u) | (NewIndex2 == INVALID_INDEX ? 4u : 0u);

								if (Mask != 0u && Mask != 4u && Mask != 6u && Mask != 7u)
								{
									continue;
								}
							}


							uint32 Opposite = OppositeCorner[CornerToIndex(TriangleCorner)];
							uint32 LeftCorner = OppositeCorner[CornerToIndex(NextCorner(TriangleCorner))];
							uint32 RightCorner = OppositeCorner[CornerToIndex(PrevCorner(TriangleCorner))];

							bool bHasOpposite = Opposite != INVALID_CORNER && Context.TriangleEnabled(CornerToTriangle(Opposite));
							bool bHasLeft = LeftCorner != INVALID_CORNER && Context.TriangleEnabled(CornerToTriangle(LeftCorner));
							bool bHasRight = RightCorner != INVALID_CORNER && Context.TriangleEnabled(CornerToTriangle(RightCorner));

							int32 Score = NewScoreTriangle(Context, TriangleIndex, true, bHasOpposite, bHasLeft, bHasRight);
							if (Score > BestScore)
							{
								StartCorner = TriangleCorner;
								BestScore = Score;
							}
							else if (Score == BestScore)
							{
								float Priority = TrianglePriorities[TriangleIndex];
								if (Priority > BestPriority)
								{
									StartCorner = TriangleCorner;
									BestScore = Score;
									BestPriority = Priority;
								}
							}
						}
					}
				}

				if (StartCorner == INVALID_CORNER)
					break;
			}

			uint32 StripLength = 1;

			{
				uint32 TriangleDword = Context.NumTriangles >> 5;
				uint32 BaseVertex = Context.NumVertices - 1;
				uint32 NumNewVertices = VisitTriangle(Context, StartCorner, true, false);

				if (NumNewVertices < 3)
				{
					uint32 Index = Context.OldToNewVertex[Cluster.Indexes[CornerToIndex(NextCorner(StartCorner))]];
					BitWriter.PutBits(BaseVertex - Index, 5);
				}
				if (NumNewVertices < 2)
				{
					uint32 Index = Context.OldToNewVertex[Cluster.Indexes[CornerToIndex(PrevCorner(StartCorner))]];
					BitWriter.PutBits(BaseVertex - Index, 5);
				}
				if (NumNewVertices < 1)
				{
					uint32 Index = Context.OldToNewVertex[Cluster.Indexes[CornerToIndex(StartCorner)]];
					BitWriter.PutBits(BaseVertex - Index, 5);
				}
				NumNewVerticesInDword[TriangleDword] += NumNewVertices;
				NumRefVerticesInDword[TriangleDword] += 3u - NumNewVertices;
			}

			// Extend strip as long as we can
			uint32 CurrentCorner = StartCorner;
			while (true)
			{
				if ((Context.NumTriangles & 31u) == 0u)
					break;

				uint32 LeftCorner = OppositeCorner[CornerToIndex(NextCorner(CurrentCorner))];
				uint32 RightCorner = OppositeCorner[CornerToIndex(PrevCorner(CurrentCorner))];
				CurrentCorner = INVALID_CORNER;

				int32 LeftScore = INT_MIN;
				if (LeftCorner != INVALID_CORNER && Context.TriangleEnabled(CornerToTriangle(LeftCorner)))
				{
					uint32 LeftLeftCorner = OppositeCorner[CornerToIndex(NextCorner(LeftCorner))];
					uint32 LeftRightCorner = OppositeCorner[CornerToIndex(PrevCorner(LeftCorner))];
					bool bLeftLeftCorner = LeftLeftCorner != INVALID_CORNER && Context.TriangleEnabled(CornerToTriangle(LeftLeftCorner));
					bool bLeftRightCorner = LeftRightCorner != INVALID_CORNER && Context.TriangleEnabled(CornerToTriangle(LeftRightCorner));

					LeftScore = NewScoreTriangle(Context, CornerToTriangle(LeftCorner), false, true, bLeftLeftCorner, bLeftRightCorner);
					CurrentCorner = LeftCorner;
				}

				bool bIsRight = false;
				if (RightCorner != INVALID_CORNER && Context.TriangleEnabled(CornerToTriangle(RightCorner)))
				{
					uint32 RightLeftCorner = OppositeCorner[CornerToIndex(NextCorner(RightCorner))];
					uint32 RightRightCorner = OppositeCorner[CornerToIndex(PrevCorner(RightCorner))];
					bool bRightLeftCorner = RightLeftCorner != INVALID_CORNER && Context.TriangleEnabled(CornerToTriangle(RightLeftCorner));
					bool bRightRightCorner = RightRightCorner != INVALID_CORNER && Context.TriangleEnabled(CornerToTriangle(RightRightCorner));

					int32 Score = NewScoreTriangle(Context, CornerToTriangle(RightCorner), false, false, bRightLeftCorner, bRightRightCorner);
					if (Score > LeftScore)
					{
						CurrentCorner = RightCorner;
						bIsRight = true;
					}
				}

				if (CurrentCorner == INVALID_CORNER)
					break;

				{
					const uint32 OldIndex0 = Cluster.Indexes[CornerToIndex(NextCorner(CurrentCorner))];
					const uint32 OldIndex1 = Cluster.Indexes[CornerToIndex(PrevCorner(CurrentCorner))];
					const uint32 OldIndex2 = Cluster.Indexes[CornerToIndex(CurrentCorner)];

					const uint32 NewIndex0 = Context.OldToNewVertex[OldIndex0];
					const uint32 NewIndex1 = Context.OldToNewVertex[OldIndex1];
					const uint32 NewIndex2 = Context.OldToNewVertex[OldIndex2];

					TI_ASSERT(NewIndex0 != INVALID_INDEX);
					TI_ASSERT(NewIndex1 != INVALID_INDEX);
					const uint32 NextNumVertices = Context.NumVertices + ((NewIndex2 == INVALID_INDEX || Context.NumVertices - NewIndex2 >= CONSTRAINED_CLUSTER_CACHE_SIZE) ? 1u : 0u);

					if (NextNumVertices - NewIndex0 >= CONSTRAINED_CLUSTER_CACHE_SIZE ||
						NextNumVertices - NewIndex1 >= CONSTRAINED_CLUSTER_CACHE_SIZE)
						break;
				}

				{
					uint32 TriangleDword = Context.NumTriangles >> 5;
					uint32 BaseVertex = Context.NumVertices - 1;
					uint32 NumNewVertices = VisitTriangle(Context, CurrentCorner, false, bIsRight);
					TI_ASSERT(NumNewVertices <= 1u);
					if (NumNewVertices == 0)
					{
						uint32 Index = Context.OldToNewVertex[Cluster.Indexes[CornerToIndex(CurrentCorner)]];
						BitWriter.PutBits(BaseVertex - Index, 5);
					}
					NumNewVerticesInDword[TriangleDword] += NumNewVertices;
					NumRefVerticesInDword[TriangleDword] += 1u - NumNewVertices;
				}

				StripLength++;
			}
		}
		RangeStart += RangeLength;
	}

	BitWriter.Flush(sizeof(uint32));

	// Reorder vertices
	const uint32 NumNewVertices = Context.NumVertices;

	TVector< float > OldVertices;
	OldVertices.swap(Cluster.Verts);
	//Swap(OldVertices, Cluster.Verts);

	uint32 VertStride = Cluster.GetVertSize();
	Cluster.Verts.resize(NumNewVertices * VertStride);
	for (uint32 i = 0; i < NumNewVertices; i++)
	{
		memcpy(&Cluster.GetPosition(i), &OldVertices[Context.NewToOldVertex[i] * VertStride], VertStride * sizeof(float));
	}

	TI_ASSERT(Context.NumTriangles == NumOldTriangles);

	Cluster.NumVerts = Context.NumVertices;

	uint32 NumPrevNewVerticesBeforeDwords1 = NumNewVerticesInDword[0];
	uint32 NumPrevNewVerticesBeforeDwords2 = NumNewVerticesInDword[1] + NumPrevNewVerticesBeforeDwords1;
	uint32 NumPrevNewVerticesBeforeDwords3 = NumNewVerticesInDword[2] + NumPrevNewVerticesBeforeDwords2;
	TI_ASSERT(NumPrevNewVerticesBeforeDwords1 < 1024 && NumPrevNewVerticesBeforeDwords2 < 1024 && NumPrevNewVerticesBeforeDwords3 < 1024);
	StripDesc.NumPrevNewVerticesBeforeDwords = (NumPrevNewVerticesBeforeDwords3 << 20) | (NumPrevNewVerticesBeforeDwords2 << 10) | NumPrevNewVerticesBeforeDwords1;

	uint32 NumPrevRefVerticesBeforeDwords1 = NumRefVerticesInDword[0];
	uint32 NumPrevRefVerticesBeforeDwords2 = NumRefVerticesInDword[1] + NumPrevRefVerticesBeforeDwords1;
	uint32 NumPrevRefVerticesBeforeDwords3 = NumRefVerticesInDword[2] + NumPrevRefVerticesBeforeDwords2;
	TI_ASSERT(NumPrevRefVerticesBeforeDwords1 < 1024 && NumPrevRefVerticesBeforeDwords2 < 1024 && NumPrevRefVerticesBeforeDwords3 < 1024);
	StripDesc.NumPrevRefVerticesBeforeDwords = (NumPrevRefVerticesBeforeDwords3 << 20) | (NumPrevRefVerticesBeforeDwords2 << 10) | NumPrevRefVerticesBeforeDwords1;

	static_assert(sizeof(StripDesc.Bitmasks) == sizeof(Context.StripBitmasks), "");
	memcpy(StripDesc.Bitmasks, Context.StripBitmasks, sizeof(StripDesc.Bitmasks));

	const uint32 PaddedSize = (uint32)Cluster.StripIndexData.size() + 5;
	TVector<uint8> PaddedStripIndexData;
	PaddedStripIndexData.reserve(PaddedSize);

	PaddedStripIndexData.push_back(0);	// TODO: Workaround for empty list and reading from negative offset
	//PaddedStripIndexData.append(Cluster.StripIndexData);
	PaddedStripIndexData.insert(PaddedStripIndexData.end(), Cluster.StripIndexData.begin(), Cluster.StripIndexData.end());

	// UnpackTriangleIndices is 1:1 with the GPU implementation.
	// It can end up over-fetching because it is branchless. The over-fetched data is never actually used.
	// On the GPU index data is followed by other page data, so it is safe.

	// Here we have to pad to make it safe to perform a DWORD read after the end.
	//PaddedStripIndexData.SetNumZeroed(PaddedSize);
	for (int32 i = 0; i < (int32)PaddedSize - (int32)PaddedStripIndexData.size(); i++)
	{
		PaddedStripIndexData.push_back(0);
	}

	// Unpack strip
	for (uint32 i = 0; i < NumOldTriangles; i++)
	{
		UnpackTriangleIndices(StripDesc, (const uint8*)(PaddedStripIndexData.data() + 1), i, &Cluster.Indexes[i * 3]);
	}
}