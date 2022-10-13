/*
	TiX Engine v2.0 Copyright (C) 2018~2021
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once


struct FPackedHierarchyNode
{
	FFloat4 LODBounds[NANITE_MAX_BVH_NODE_FANOUT];

	struct
	{
		FFloat3 BoxBoundsCenter;
		uint32 MinLODError_MaxParentLODError;
	} Misc0[NANITE_MAX_BVH_NODE_FANOUT];

	struct
	{
		FFloat3 BoxBoundsExtent;
		uint32 ChildStartReference;
	} Misc1[NANITE_MAX_BVH_NODE_FANOUT];

	struct
	{
		uint32		ResourcePageIndex_NumPages_GroupPartSize;
	} Misc2[NANITE_MAX_BVH_NODE_FANOUT];
};


FORCEINLINE uint32 GetBits(uint32 Value, uint32 NumBits, uint32 Offset)
{
	uint32 Mask = (1u << NumBits) - 1u;
	return (Value >> Offset) & Mask;
}

FORCEINLINE void SetBits(uint32& Value, uint32 Bits, uint32 NumBits, uint32 Offset)
{
	uint32 Mask = (1u << NumBits) - 1u;
	TI_ASSERT(Bits <= Mask);
	Mask <<= Offset;
	Value = (Value & ~Mask) | (Bits << Offset);
}

// Packed Cluster as it is used by the GPU
struct FPackedCluster
{
	// Members needed for rasterization
	uint32 NumVerts_PositionOffset;					// NumVerts:9, PositionOffset:23
	uint32 NumTris_IndexOffset;						// NumTris:8, IndexOffset: 24
	uint32 ColorMin;
	uint32 ColorBits_GroupIndex;						// R:4, G:4, B:4, A:4. (GroupIndex&0xFFFF) is for debug visualization only.

	FInt3 PosStart;
	uint32 BitsPerIndex_PosPrecision_PosBits;			// BitsPerIndex:4, PosPrecision: 5, PosBits:5.5.5

	// Members needed for culling
	FFloat4 LODBounds;									// LWC_TODO: Was FSphere, but that's now twice as big and won't work on GPU.

	FFloat3 BoxBoundsCenter;
	uint32 LODErrorAndEdgeLength;

	FFloat3 BoxBoundsExtent;
	uint32 Flags;

	// Members needed by materials
	uint32 AttributeOffset_BitsPerAttribute;			// AttributeOffset: 22, BitsPerAttribute: 10
	uint32 DecodeInfoOffset_NumUVs_ColorMode;			// DecodeInfoOffset: 22, NumUVs: 3, ColorMode: 2
	uint32 UV_Prec;									// U0:4, V0:4, U1:4, V1:4, U2:4, V2:4, U3:4, V3:4
	uint32 PackedMaterialInfo;
		   
	uint32 GetNumVerts() const { return GetBits(NumVerts_PositionOffset, 9, 0); }
	uint32 GetPositionOffset() const { return GetBits(NumVerts_PositionOffset, 23, 9); }
		   
	uint32 GetNumTris() const { return GetBits(NumTris_IndexOffset, 8, 0); }
	uint32 GetIndexOffset() const { return GetBits(NumTris_IndexOffset, 24, 8); }
		   
	uint32 GetBitsPerIndex() const { return GetBits(BitsPerIndex_PosPrecision_PosBits, 4, 0); }
	int32 GetPosPrecision() const { return (int32)GetBits(BitsPerIndex_PosPrecision_PosBits, 5, 4) + NANITE_MIN_POSITION_PRECISION; }
	uint32 GetPosBitsX() const { return GetBits(BitsPerIndex_PosPrecision_PosBits, 5, 9); }
	uint32 GetPosBitsY() const { return GetBits(BitsPerIndex_PosPrecision_PosBits, 5, 14); }
	uint32 GetPosBitsZ() const { return GetBits(BitsPerIndex_PosPrecision_PosBits, 5, 19); }
		   
	uint32 GetAttributeOffset() const { return GetBits(AttributeOffset_BitsPerAttribute, 22, 0); }
	uint32 GetBitsPerAttribute() const { return GetBits(AttributeOffset_BitsPerAttribute, 10, 22); }

	void SetNumVerts(uint32 NumVerts) { SetBits(NumVerts_PositionOffset, NumVerts, 9, 0); }
	void SetPositionOffset(uint32 Offset) { SetBits(NumVerts_PositionOffset, Offset, 23, 9); }
		 
	void SetNumTris(uint32 NumTris) { SetBits(NumTris_IndexOffset, NumTris, 8, 0); }
	void SetIndexOffset(uint32 Offset) { SetBits(NumTris_IndexOffset, Offset, 24, 8); }
		 
	void SetBitsPerIndex(uint32 BitsPerIndex) { SetBits(BitsPerIndex_PosPrecision_PosBits, BitsPerIndex, 4, 0); }
	void SetPosPrecision(int32 Precision) { SetBits(BitsPerIndex_PosPrecision_PosBits, uint32(Precision - NANITE_MIN_POSITION_PRECISION), 5, 4); }
	void SetPosBitsX(uint32 NumBits) { SetBits(BitsPerIndex_PosPrecision_PosBits, NumBits, 5, 9); }
	void SetPosBitsY(uint32 NumBits) { SetBits(BitsPerIndex_PosPrecision_PosBits, NumBits, 5, 14); }
	void SetPosBitsZ(uint32 NumBits) { SetBits(BitsPerIndex_PosPrecision_PosBits, NumBits, 5, 19); }
		 
	void SetAttributeOffset(uint32 Offset) { SetBits(AttributeOffset_BitsPerAttribute, Offset, 22, 0); }
	void SetBitsPerAttribute(uint32 Bits) { SetBits(AttributeOffset_BitsPerAttribute, Bits, 10, 22); }
		 
	void SetDecodeInfoOffset(uint32 Offset) { SetBits(DecodeInfoOffset_NumUVs_ColorMode, Offset, 22, 0); }
	void SetNumUVs(uint32 Num) { SetBits(DecodeInfoOffset_NumUVs_ColorMode, Num, 3, 22); }
	void SetColorMode(uint32 Mode) { SetBits(DecodeInfoOffset_NumUVs_ColorMode, Mode, 2, 22 + 3); }
		 
	void SetColorBitsR(uint32 NumBits) { SetBits(ColorBits_GroupIndex, NumBits, 4, 0); }
	void SetColorBitsG(uint32 NumBits) { SetBits(ColorBits_GroupIndex, NumBits, 4, 4); }
	void SetColorBitsB(uint32 NumBits) { SetBits(ColorBits_GroupIndex, NumBits, 4, 8); }
	void SetColorBitsA(uint32 NumBits) { SetBits(ColorBits_GroupIndex, NumBits, 4, 12); }
		 
	void SetGroupIndex(uint32 GroupIndex) { SetBits(ColorBits_GroupIndex, GroupIndex & 0xFFFFu, 16, 16); }
};

class TNaniteMesh
{
public:
	~TNaniteMesh() = default;
	static TNaniteMesh* LoadMesh();

protected:
	TNaniteMesh();
	static bool ConvertNanieMesh();
};