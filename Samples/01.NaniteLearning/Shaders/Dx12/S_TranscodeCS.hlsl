/*
	TiX Engine v3.0 Copyright (C) 2018~2021
	By ZhaoShuai tirax.cn@gmail.com
*/

// NaniteLearning Sample
#include "Common.hlsli"
#include "NaniteDefinitions.h"
#include "NaniteAttributeDecode.h"



#define TRANSCODE_THREADS_PER_GROUP_BITS	7
#define TRANSCODE_THREADS_PER_GROUP			(1 << TRANSCODE_THREADS_PER_GROUP_BITS)
#define TRANSCODE_THREADS_PER_PAGE			(NANITE_MAX_TRANSCODE_GROUPS_PER_PAGE << TRANSCODE_THREADS_PER_GROUP_BITS)

struct FPageInstallInfo
{
	uint SrcPageOffset;
	uint DstPageOffset;
	uint PageDependenciesStart;
	uint PageDependenciesNum;
};

// Params
StructuredBuffer<FPageInstallInfo> InstallInfoBuffer : register(t0);
StructuredBuffer<uint> PageDependenciesBuffer : register(t1);
ByteAddressBuffer SrcPageBuffer : register(t2);

RWByteAddressBuffer DstPageBuffer : register(u0);


struct FPageDiskHeader
{
	uint GpuSize;
	uint NumClusters;
	uint NumClusterInstances;
	uint NumRawFloat4s;
	uint NumTexCoords;
	uint NumVertexRefs;
	uint DecodeInfoOffset;
	uint StripBitmaskOffset;
	uint VertexRefBitmaskOffset;
};
#define SIZEOF_PAGE_DISK_HEADER	(8*4)

FPageDiskHeader GetPageDiskHeader(uint PageBaseOffset)
{
	uint4 Data[2];
	Data[0] = SrcPageBuffer.Load4(PageBaseOffset + 0);
	Data[1] = SrcPageBuffer.Load4(PageBaseOffset + 16);

	FPageDiskHeader DiskHeader;
	DiskHeader.GpuSize					= Data[0].x;
	DiskHeader.NumClusters				= Data[0].y >> 16;
	DiskHeader.NumClusterInstances		= Data[0].y & 0xffff;
	DiskHeader.NumRawFloat4s			= Data[0].z;
	DiskHeader.NumTexCoords				= Data[0].w;
	DiskHeader.NumVertexRefs			= Data[1].x;
	DiskHeader.DecodeInfoOffset			= Data[1].y;
	DiskHeader.StripBitmaskOffset		= Data[1].z;
	DiskHeader.VertexRefBitmaskOffset	= Data[1].w;
	return DiskHeader;
}

struct FClusterDiskHeader
{
	uint IndexDataOffset;
	uint PageClusterMapOffset;
	uint VertexRefDataOffset;
	uint PositionDataOffset;
	uint AttributeDataOffset;
	uint NumVertexRefs;
	uint NumPrevRefVerticesBeforeDwords;
	uint NumPrevNewVerticesBeforeDwords;
};

#define SIZEOF_CLUSTER_DISK_HEADER	(8*4)

FClusterDiskHeader GetClusterDiskHeader(uint PageBaseOffset, uint ClusterIndex)
{
	uint ByteOffset = PageBaseOffset + SIZEOF_PAGE_DISK_HEADER + ClusterIndex * SIZEOF_CLUSTER_DISK_HEADER;
	uint4 Data[2];
	Data[0]	= SrcPageBuffer.Load4(ByteOffset);
	Data[1] = SrcPageBuffer.Load4(ByteOffset + 16);
	
	FClusterDiskHeader Header;
	Header.IndexDataOffset					= Data[0].x;
	Header.PageClusterMapOffset				= Data[0].y;
	Header.VertexRefDataOffset				= Data[0].z;
	Header.PositionDataOffset				= Data[0].w;
	Header.AttributeDataOffset				= Data[1].x;
	Header.NumVertexRefs					= Data[1].y;
	Header.NumPrevRefVerticesBeforeDwords	= Data[1].z;
	Header.NumPrevNewVerticesBeforeDwords	= Data[1].w;
	return Header;
}


uint ReadUnalignedDword(ByteAddressBuffer InputBuffer, uint BaseAddressInBytes, int BitOffset)
{
	uint ByteAddress = BaseAddressInBytes + (BitOffset >> 3);
	uint AlignedByteAddress = ByteAddress & ~3;
	BitOffset = ((ByteAddress - AlignedByteAddress) << 3) | (BitOffset & 7);

	uint2 Data = InputBuffer.Load2(AlignedByteAddress);
	return BitAlignU32(Data.y, Data.x, BitOffset);
}


uint ReadByte(ByteAddressBuffer	InputBuffer, uint Address)
{
	return (InputBuffer.Load(Address & ~3u) >> ((Address & 3u) * 8)) & 0xFFu;	//TODO: use bytealign intrinsic?
}

uint ReadUnalignedDwordFromAlignedBase(ByteAddressBuffer SrcBuffer, uint AlignedBaseAddress, uint BitOffset)
{
	uint2 Data = SrcBuffer.Load2(AlignedBaseAddress);
	return BitAlignU32(Data.y, Data.x, BitOffset);
}

void CopyBits(RWByteAddressBuffer DstBuffer, uint DstBaseAddress, uint DstBitOffset, ByteAddressBuffer SrcBuffer, uint SrcBaseAddress, uint SrcBitOffset, uint NumBits)
{
	if (NumBits == 0)
		return;

	// TODO: optimize me
	uint DstDword = (DstBaseAddress + (DstBitOffset >> 3)) >> 2;
	DstBitOffset = (((DstBaseAddress & 3u) << 3) + DstBitOffset) & 31u;
	uint SrcDword = (SrcBaseAddress + (SrcBitOffset >> 3)) >> 2;
	SrcBitOffset = (((SrcBaseAddress & 3u) << 3) + SrcBitOffset) & 31u;

	uint DstNumDwords = (DstBitOffset + NumBits + 31u) >> 5;
	uint DstLastBitOffset = (DstBitOffset + NumBits) & 31u;

	const uint FirstMask = 0xFFFFFFFFu << DstBitOffset;
	const uint LastMask = DstLastBitOffset ? BitFieldMaskU32(DstLastBitOffset, 0) : 0xFFFFFFFFu;
	const uint Mask = FirstMask & (DstNumDwords == 1 ? LastMask : 0xFFFFFFFFu);

	{
		uint Data = ReadUnalignedDwordFromAlignedBase(SrcBuffer, SrcDword * 4, SrcBitOffset);
		DstBuffer.InterlockedAnd(DstDword * 4, ~Mask);
		DstBuffer.InterlockedOr(DstDword * 4, (Data << DstBitOffset) & Mask);
		DstDword++;
		DstNumDwords--;
	}

	if (DstNumDwords > 0)
	{
		SrcBitOffset += 32 - DstBitOffset;
		SrcDword += SrcBitOffset >> 5;
		SrcBitOffset &= 31u;

		while (DstNumDwords > 1)
		{
			uint Data = ReadUnalignedDwordFromAlignedBase(SrcBuffer, SrcDword * 4, SrcBitOffset);
			DstBuffer.Store(DstDword * 4, Data);
			DstDword++;
			SrcDword++;
			DstNumDwords--;
		}

		uint Data = ReadUnalignedDwordFromAlignedBase(SrcBuffer, SrcDword * 4, SrcBitOffset);
		DstBuffer.InterlockedAnd(DstDword * 4, ~LastMask);
		DstBuffer.InterlockedOr(DstDword * 4, Data & LastMask);
	}
}

// Debug only. Performance doesn't matter.
void CopyDwords(RWByteAddressBuffer DstBuffer, uint DstAddress, ByteAddressBuffer SrcBuffer, uint SrcAddress, uint NumDwords)
{
	for (uint i = 0; i < NumDwords; i++)
	{
		DstBuffer.Store(DstAddress + i * 4, SrcBuffer.Load(SrcAddress + i * 4));
	}
}

uint3 UnpackStripIndices(uint SrcPageBaseOffset, FPageDiskHeader PageDiskHeader, FClusterDiskHeader ClusterDiskHeader, uint LocalClusterIndex, uint TriIndex)
{
	const uint DwordIndex = TriIndex >> 5;
	const uint BitIndex = TriIndex & 31u;

	//Bitmask.x: bIsStart, Bitmask.y: bIsLeft, Bitmask.z: bIsNewVertex
	const uint3 StripBitmasks = SrcPageBuffer.Load3(SrcPageBaseOffset + PageDiskHeader.StripBitmaskOffset + (LocalClusterIndex * (NANITE_MAX_CLUSTER_TRIANGLES / 32) + DwordIndex) * 12);

	const uint SMask = StripBitmasks.x;
	const uint LMask = StripBitmasks.y;
	const uint WMask = StripBitmasks.z;
	const uint SLMask = SMask & LMask;

	//const uint HeadRefVertexMask = ( SMask & LMask & WMask ) | ( ~SMask & WMask );
	const uint HeadRefVertexMask = (SLMask | ~SMask) & WMask;	// 1 if head of triangle is ref. S case with 3 refs or L/R case with 1 ref.

	const uint PrevBitsMask = (1u << BitIndex) - 1u;

	const uint NumPrevRefVerticesBeforeDword = DwordIndex ? BitFieldExtractU32(ClusterDiskHeader.NumPrevRefVerticesBeforeDwords, 10u, DwordIndex * 10u - 10u) : 0u;
	const uint NumPrevNewVerticesBeforeDword = DwordIndex ? BitFieldExtractU32(ClusterDiskHeader.NumPrevNewVerticesBeforeDwords, 10u, DwordIndex * 10u - 10u) : 0u;

	int CurrentDwordNumPrevRefVertices = (countbits(SLMask & PrevBitsMask) << 1) + countbits(WMask & PrevBitsMask);
	int CurrentDwordNumPrevNewVertices = (countbits(SMask & PrevBitsMask) << 1) + BitIndex - CurrentDwordNumPrevRefVertices;

	int NumPrevRefVertices = NumPrevRefVerticesBeforeDword + CurrentDwordNumPrevRefVertices;
	int NumPrevNewVertices = NumPrevNewVerticesBeforeDword + CurrentDwordNumPrevNewVertices;

	const int IsStart = BitFieldExtractI32(SMask, 1, BitIndex);		// -1: true, 0: false
	const int IsLeft = BitFieldExtractI32(LMask, 1, BitIndex);		// -1: true, 0: false
	const int IsRef = BitFieldExtractI32(WMask, 1, BitIndex);		// -1: true, 0: false

	const uint BaseVertex = NumPrevNewVertices - 1u;

	uint3 OutIndices;
	uint ReadBaseAddress = SrcPageBaseOffset + ClusterDiskHeader.IndexDataOffset;
	uint IndexData = ReadUnalignedDword(SrcPageBuffer, ReadBaseAddress, (NumPrevRefVertices + ~IsStart) * 5);	// -1 if not Start

	if (IsStart)
	{
		const int MinusNumRefVertices = (IsLeft << 1) + IsRef;
		uint NextVertex = NumPrevNewVertices;

		if (MinusNumRefVertices <= -1) { OutIndices.x = BaseVertex - (IndexData & 31u); IndexData >>= 5; }
		else { OutIndices[0] = NextVertex++; }
		if (MinusNumRefVertices <= -2) { OutIndices.y = BaseVertex - (IndexData & 31u); IndexData >>= 5; }
		else { OutIndices[1] = NextVertex++; }
		if (MinusNumRefVertices <= -3) { OutIndices.z = BaseVertex - (IndexData & 31u); }
		else { OutIndices[2] = NextVertex++; }
	}
	else
	{
		// Handle two first vertices
		const uint PrevBitIndex = BitIndex - 1u;
		const int IsPrevStart = BitFieldExtractI32(SMask, 1, PrevBitIndex);
		const int IsPrevHeadRef = BitFieldExtractI32(HeadRefVertexMask, 1, PrevBitIndex);
		//const int NumPrevNewVerticesInTriangle = IsPrevStart ? ( 3u - ( bfe_u32( /*SLMask*/ LMask, PrevBitIndex, 1 ) << 1 ) - bfe_u32( /*SMask &*/ WMask, PrevBitIndex, 1 ) ) : /*1u - IsPrevRefVertex*/ 0u;
		const int NumPrevNewVerticesInTriangle = IsPrevStart & (3u - ((BitFieldExtractU32( /*SLMask*/ LMask, 1, PrevBitIndex) << 1) | BitFieldExtractU32( /*SMask &*/ WMask, 1, PrevBitIndex)));

		//OutIndices[ 1 ] = IsPrevRefVertex ? ( BaseVertex - ( IndexData & 31u ) + NumPrevNewVerticesInTriangle ) : BaseVertex;	// BaseVertex = ( NumPrevNewVertices - 1 );
		OutIndices.y = BaseVertex + (IsPrevHeadRef & (NumPrevNewVerticesInTriangle - (IndexData & 31u)));
		//OutIndices[ 2 ] = IsRefVertex ? ( BaseVertex - bfe_u32( IndexData, 5, 5 ) ) : NumPrevNewVertices;
		OutIndices.z = NumPrevNewVertices + (IsRef & (-1 - BitFieldExtractU32(IndexData, 5, 5)));

		// We have to search for the third vertex. 
		// Left triangles search for previous Right/Start. Right triangles search for previous Left/Start.
		const uint SearchMask = SMask | (LMask ^ IsLeft);				// SMask | ( IsRight ? LMask : RMask );
		const uint FoundBitIndex = firstbithigh(SearchMask & PrevBitsMask);
		const int IsFoundCaseS = BitFieldExtractI32(SMask, 1, FoundBitIndex);		// -1: true, 0: false

		const uint FoundPrevBitsMask = (1u << FoundBitIndex) - 1u;
		int FoundCurrentDwordNumPrevRefVertices = (countbits(SLMask & FoundPrevBitsMask) << 1) + countbits(WMask & FoundPrevBitsMask);
		int FoundCurrentDwordNumPrevNewVertices = (countbits(SMask & FoundPrevBitsMask) << 1) + FoundBitIndex - FoundCurrentDwordNumPrevRefVertices;

		int FoundNumPrevNewVertices = NumPrevNewVerticesBeforeDword + FoundCurrentDwordNumPrevNewVertices;
		int FoundNumPrevRefVertices = NumPrevRefVerticesBeforeDword + FoundCurrentDwordNumPrevRefVertices;

		const uint FoundNumRefVertices = (BitFieldExtractU32(LMask, 1, FoundBitIndex) << 1) + BitFieldExtractU32(WMask, 1, FoundBitIndex);
		const uint IsBeforeFoundRefVertex = BitFieldExtractU32(HeadRefVertexMask, 1, FoundBitIndex - 1);

		// ReadOffset: Where is the vertex relative to triangle we searched for?
		const int ReadOffset = IsFoundCaseS ? IsLeft : 1;
		const uint FoundIndexData = ReadUnalignedDword(SrcPageBuffer, ReadBaseAddress, (FoundNumPrevRefVertices - ReadOffset) * 5);
		const uint FoundIndex = (FoundNumPrevNewVertices - 1u) - BitFieldExtractU32(FoundIndexData, 5, 0);

		bool bCondition = IsFoundCaseS ? ((int)FoundNumRefVertices >= 1 - IsLeft) : IsBeforeFoundRefVertex;
		int FoundNewVertex = FoundNumPrevNewVertices + (IsFoundCaseS ? (IsLeft & (FoundNumRefVertices == 0)) : -1);
		OutIndices.x = bCondition ? FoundIndex : FoundNewVertex;

		// Would it be better to code New verts instead of Ref verts?
		// HeadRefVertexMask would just be WMask?

		if (IsLeft)
		{
			OutIndices.yz = OutIndices.zy;
		}
	}

	return OutIndices;
}

void TranscodeVertexAttributes(FPageDiskHeader PageDiskHeader, FCluster Cluster, uint DstPageBaseOffset, uint LocalClusterIndex, uint VertexIndex,
	FCluster SrcCluster, FClusterDiskHeader SrcClusterDiskHeader, uint SrcPageBaseOffset, uint SrcLocalClusterIndex, uint SrcCodedVertexIndex,
	bool bIsParentRef, uint ParentGPUPageIndex,
	uint CompileTimeNumTexCoords
)
{
	const uint CompileTimeMaxVertexBits = 2 * NANITE_NORMAL_QUANTIZATION_BITS + 4 * NANITE_MAX_COLOR_QUANTIZATION_BITS + CompileTimeNumTexCoords * 2 * NANITE_MAX_TEXCOORD_QUANTIZATION_BITS;

	const uint BaseAddress = GPUPageIndexToGPUOffset(ParentGPUPageIndex);

	const uint BitsPerAttribute = bIsParentRef ? SrcCluster.BitsPerAttribute : ((SrcCluster.BitsPerAttribute + 7) & ~7u);
	const uint Address = bIsParentRef ? (BaseAddress + SrcCluster.AttributeOffset) : (SrcPageBaseOffset + SrcClusterDiskHeader.AttributeDataOffset);

	FBitStreamWriterState OutputStream = BitStreamWriter_Create_Aligned(DstPageBaseOffset + Cluster.AttributeOffset, VertexIndex * Cluster.BitsPerAttribute);
	FBitStreamReaderState InputStream = BitStreamReader_Create(Address, SrcCodedVertexIndex * BitsPerAttribute, CompileTimeMaxVertexBits);

	// Normal
	uint PackedNormal = BitStreamReader_Read_RORW(SrcPageBuffer, DstPageBuffer, bIsParentRef, InputStream, 2 * NANITE_NORMAL_QUANTIZATION_BITS, 2 * NANITE_NORMAL_QUANTIZATION_BITS);
	BitStreamWriter_Writer(DstPageBuffer, OutputStream, PackedNormal, 2 * NANITE_NORMAL_QUANTIZATION_BITS, 2 * NANITE_NORMAL_QUANTIZATION_BITS);

	// Color
	{
		uint4 SrcComponentBits = UnpackToUint4(SrcCluster.ColorBits, 4);
		uint4 SrcColorDelta = BitStreamReader_Read4_RORW(SrcPageBuffer, DstPageBuffer, bIsParentRef, InputStream, SrcComponentBits, 8);

		if (Cluster.ColorMode == NANITE_VERTEX_COLOR_MODE_VARIABLE)
		{
			uint SrcPackedColorDelta = SrcColorDelta.x | (SrcColorDelta.y << 8) | (SrcColorDelta.z << 16) | (SrcColorDelta.w << 24);
			uint PackedColor = SrcCluster.ColorMin + SrcPackedColorDelta;

			uint4 DstComponentBits = UnpackToUint4(Cluster.ColorBits, 4);
			uint DstPackedColorDelta = PackedColor - Cluster.ColorMin;

			uint PackedDeltaColor = BitFieldExtractU32(DstPackedColorDelta, 8, 0) |
				(BitFieldExtractU32(DstPackedColorDelta, 8, 8) << (DstComponentBits.x)) |
				(BitFieldExtractU32(DstPackedColorDelta, 8, 16) << (DstComponentBits.x + DstComponentBits.y)) |
				(BitFieldExtractU32(DstPackedColorDelta, 8, 24) << (DstComponentBits.x + DstComponentBits.y + DstComponentBits.z));

			BitStreamWriter_Writer(DstPageBuffer, OutputStream, PackedDeltaColor, DstComponentBits.x + DstComponentBits.y + DstComponentBits.z + DstComponentBits.w, 4 * NANITE_MAX_COLOR_QUANTIZATION_BITS);
		}
	}

	// UVs
	//UNROLL_N(NANITE_MAX_UVS)
	//for (uint TexCoordIndex = 0; TexCoordIndex < CompileTimeNumTexCoords; TexCoordIndex++)
	// Only work with CompileTimeNumTexCoords = 1;
	uint TexCoordIndex = 0;
	{
		const uint SrcU_NumBits = BitFieldExtractU32(SrcCluster.UV_Prec, 4, TexCoordIndex * 8 + 0);
		const uint SrcV_NumBits = BitFieldExtractU32(SrcCluster.UV_Prec, 4, TexCoordIndex * 8 + 4);

		const uint DstU_NumBits = BitFieldExtractU32(Cluster.UV_Prec, 4, TexCoordIndex * 8 + 0);
		const uint DstV_NumBits = BitFieldExtractU32(Cluster.UV_Prec, 4, TexCoordIndex * 8 + 4);

		const int2 SrcPackedUV = BitStreamReader_Read2_RORW(SrcPageBuffer, DstPageBuffer, bIsParentRef, InputStream, int2(SrcU_NumBits, SrcV_NumBits), NANITE_MAX_TEXCOORD_QUANTIZATION_BITS);

		FUVRange SrcUVRange;
		if (bIsParentRef)
			SrcUVRange = GetUVRange(DstPageBuffer, BaseAddress + SrcCluster.DecodeInfoOffset, TexCoordIndex);
		else
			SrcUVRange = GetUVRange(SrcPageBuffer, SrcPageBaseOffset + PageDiskHeader.DecodeInfoOffset + SrcLocalClusterIndex * CompileTimeNumTexCoords * 32, TexCoordIndex);

		const FUVRange UVRange = GetUVRange(SrcPageBuffer, SrcPageBaseOffset + PageDiskHeader.DecodeInfoOffset + LocalClusterIndex * CompileTimeNumTexCoords * 32, TexCoordIndex);

		const float Scale = asfloat(asint(1.0f) + ((UVRange.Precision - SrcUVRange.Precision) << 23));
		const int2 SrcUV = SrcPackedUV + select(SrcPackedUV > SrcUVRange.GapStart, SrcUVRange.GapLength, 0u) + SrcUVRange.Min;
		int2 DstUV = int2(round(SrcUV * Scale));
		uint2 DstPackedUV = (uint2)max(DstUV - UVRange.Min, 0);

		const uint2 GapMid = UVRange.GapStart + (UVRange.GapLength >> 1);
		const bool2 bOverMid = DstPackedUV > GapMid;

		const uint2 RangeMin = select(bOverMid, UVRange.GapStart + 1u, 0u);
		const uint2 RangeMax = select(bOverMid, uint2(BitFieldMaskU32(DstU_NumBits, 0), BitFieldMaskU32(DstV_NumBits, 0)), UVRange.GapStart);
		DstPackedUV = select(bOverMid, DstPackedUV - UVRange.GapLength, DstPackedUV);
		DstPackedUV = clamp(DstPackedUV, RangeMin, RangeMax);

		BitStreamWriter_Writer(DstPageBuffer, OutputStream, (DstPackedUV.y << DstU_NumBits) | DstPackedUV.x, DstU_NumBits + DstV_NumBits, 2 * NANITE_MAX_TEXCOORD_QUANTIZATION_BITS);
	}

	BitStreamWriter_Flush(DstPageBuffer, OutputStream);
}

groupshared uint GroupNumRefsInPrevDwords8888[2];	// Packed byte counts: 8:8:8:8
groupshared uint GroupRefToVertex[NANITE_MAX_CLUSTER_VERTICES];
groupshared uint GroupNonRefToVertex[NANITE_MAX_CLUSTER_VERTICES];


#define TranscodeRS \
	"RootConstants(num32BitConstants=10, b0)," \
    "DescriptorTable(SRV(t0, numDescriptors=3), UAV(u0, numDescriptors=1))" 

[RootSignature(TranscodeRS)]
[numthreads(TRANSCODE_THREADS_PER_GROUP, 1, 1)]
void TranscodePageToGPU(uint2 GroupID : SV_GroupID, uint GroupIndex : SV_GroupIndex)
{
	uint	LocalPageIndex			= DecodeInfo.StartPageIndex + GroupID.y;

	FPageInstallInfo InstallInfo	= InstallInfoBuffer[LocalPageIndex];
	uint	SrcPageBaseOffset		= InstallInfo.SrcPageOffset;
	uint	DstPageBaseOffset		= InstallInfo.DstPageOffset;
	
	FPageDiskHeader PageDiskHeader	= GetPageDiskHeader(SrcPageBaseOffset);

	const uint NumTexCoords = PageDiskHeader.NumTexCoords;
	uint SrcPackedClusterOffset = SrcPageBaseOffset + SIZEOF_PAGE_DISK_HEADER + PageDiskHeader.NumClusters * SIZEOF_CLUSTER_DISK_HEADER;
	uint DstPackedClusterOffset = DstPageBaseOffset;

	uint NumRawFloat4s = PageDiskHeader.NumRawFloat4s;
	uint PageThread = (GroupID.x << TRANSCODE_THREADS_PER_GROUP_BITS) | GroupIndex;

	// Raw copy: FPackedClusters, FPackedClusterInstances, Material Dwords and DecodeInfo.
	for(uint i = PageThread; i < NumRawFloat4s; i += TRANSCODE_THREADS_PER_PAGE)
	{
		uint4 Data = SrcPageBuffer.Load4(SrcPackedClusterOffset + i * 16);
		DstPageBuffer.Store4(DstPackedClusterOffset + i * 16, Data);
	}

	for(uint LocalClusterIndex = GroupID.x; LocalClusterIndex < PageDiskHeader.NumClusters; LocalClusterIndex += NANITE_MAX_TRANSCODE_GROUPS_PER_PAGE)
	{
		FClusterDiskHeader ClusterDiskHeader = GetClusterDiskHeader(SrcPageBaseOffset, LocalClusterIndex);
		FCluster Cluster = GetCluster(SrcPageBuffer, SrcPackedClusterOffset, LocalClusterIndex, PageDiskHeader.NumClusters);
		
		// Decode indices
		if (GroupIndex < Cluster.NumTris)
		{
			uint TriangleIndex = GroupIndex;
//#if NANITE_USE_STRIP_INDICES
			uint3 Indices = UnpackStripIndices(SrcPageBaseOffset, PageDiskHeader, ClusterDiskHeader, LocalClusterIndex, TriangleIndex);
//#else
//			FBitStreamReaderState InputStream = BitStreamReader_Create(SrcPageBaseOffset + ClusterDiskHeader.IndexDataOffset, TriangleIndex * 24, 24);
//			uint Indices24 = BitStreamReader_Read_RO(SrcPageBuffer, InputStream, 24, 24);
//			uint3 Indices = uint3(Indices24 & 0xFF, (Indices24 >> 8) & 0xFF, (Indices24 >> 16) & 0xFF);
//#endif
			// Rotate triangle so first vertex has the lowest index
			if (Indices.y < min(Indices.x, Indices.z)) Indices = Indices.yzx;
			else if (Indices.z < min(Indices.x, Indices.y)) Indices = Indices.zxy;

			// Store triangle as one base index and two 5-bit offsets. Cluster constraints guarantee that the offsets are never larger than 5 bits.
			uint BaseIndex = Indices.x;
			uint Delta0 = Indices.y - BaseIndex;
			uint Delta1 = Indices.z - BaseIndex;

			uint PackedIndices = BaseIndex | (Delta0 << Cluster.BitsPerIndex) | (Delta1 << (Cluster.BitsPerIndex + 5));
			const uint BitsPerTriangle = Cluster.BitsPerIndex + 2 * 5;

			PutBits(DstPageBuffer, DstPageBaseOffset + Cluster.IndexOffset, TriangleIndex * BitsPerTriangle, PackedIndices, BitsPerTriangle);
		}

		// Calculate dword-based prefix sum of ref bitmask
		// TODO: optimize me
		const uint AlignedBitmaskOffset = SrcPageBaseOffset + PageDiskHeader.VertexRefBitmaskOffset + LocalClusterIndex * (NANITE_MAX_CLUSTER_VERTICES / 8);
		GroupMemoryBarrierWithGroupSync();
		if (GroupIndex < 2) GroupNumRefsInPrevDwords8888[GroupIndex] = 0;
		GroupMemoryBarrierWithGroupSync();
		if (GroupIndex < 7) {
			const uint Count = countbits(SrcPageBuffer.Load(AlignedBitmaskOffset + GroupIndex * 4));
			const uint Count8888 = Count * 0x01010101u;	// Broadcast count to all bytes
			const uint Index = GroupIndex + 1;
			InterlockedAdd(GroupNumRefsInPrevDwords8888[Index >> 2], Count8888 << ((Index & 3) << 3));	// Add to bytes above
			if (Cluster.NumVerts > 128 && Index < 4)
			{
				// Add low dword byte counts to all bytes in high dword when there are more than 128 vertices.
				InterlockedAdd(GroupNumRefsInPrevDwords8888[1], Count8888); 
			}
		}
		GroupMemoryBarrierWithGroupSync();

		for (uint VertexIndex = GroupIndex; VertexIndex < Cluster.NumVerts; VertexIndex += TRANSCODE_THREADS_PER_GROUP)
		{
			const uint DwordIndex = VertexIndex >> 5;
			const uint BitIndex = VertexIndex & 31u;
			
			const uint Shift = ((DwordIndex & 3) << 3);
			const uint NumRefsInPrevDwords = (GroupNumRefsInPrevDwords8888[DwordIndex >> 2] >> Shift) & 0xFF;

			const uint DwordMask = SrcPageBuffer.Load(AlignedBitmaskOffset + DwordIndex * 4);
			const uint NumPrevRefVertices = countbits(BitFieldExtractU32(DwordMask, BitIndex, 0)) + NumRefsInPrevDwords;

			const bool bIsRef = (DwordMask & (1u << BitIndex)) != 0u;
			if (bIsRef)
			{
				GroupRefToVertex[NumPrevRefVertices] = VertexIndex;
			}
			else
			{
				const uint NumPrevNonRefVertices = VertexIndex - NumPrevRefVertices;
				GroupNonRefToVertex[NumPrevNonRefVertices] = VertexIndex;
			}
		}
		GroupMemoryBarrierWithGroupSync();

		// Non-Ref vertices
		const uint NumNonRefVertices = Cluster.NumVerts - ClusterDiskHeader.NumVertexRefs;
		for (uint NonRefVertexIndex = GroupIndex; NonRefVertexIndex < NumNonRefVertices; NonRefVertexIndex += TRANSCODE_THREADS_PER_GROUP)
		{
			const uint VertexIndex = GroupNonRefToVertex[NonRefVertexIndex];

//#if NANITE_USE_UNCOMPRESSED_VERTEX_DATA
//			// Position
//			uint3 PositionData = SrcPageBuffer.Load3(SrcPageBaseOffset + ClusterDiskHeader.PositionDataOffset + NonRefVertexIndex * 12);
//			DstPageBuffer.Store3(DstPageBaseOffset + Cluster.PositionOffset + VertexIndex * 12, PositionData);
//
//			// Attributes
//			CopyDwords(	DstPageBuffer, DstPageBaseOffset + Cluster.AttributeOffset + VertexIndex * Cluster.BitsPerAttribute / 8,
//						SrcPageBuffer, SrcPageBaseOffset + ClusterDiskHeader.AttributeDataOffset + NonRefVertexIndex * Cluster.BitsPerAttribute / 8, Cluster.BitsPerAttribute / 32);
//#else
			// Position
			const uint PositionBitsPerVertex = Cluster.PosBits.x + Cluster.PosBits.y + Cluster.PosBits.z;
			const uint SrcPositionBitsPerVertex = (PositionBitsPerVertex + 7) & ~7u;
			CopyBits(	DstPageBuffer, DstPageBaseOffset + Cluster.PositionOffset, VertexIndex * PositionBitsPerVertex,
						SrcPageBuffer, SrcPageBaseOffset + ClusterDiskHeader.PositionDataOffset, NonRefVertexIndex * SrcPositionBitsPerVertex, PositionBitsPerVertex);
			
			// Attributes
			const uint SrcBitsPerAttribute = ((Cluster.BitsPerAttribute + 7) & ~7u);
			CopyBits(	DstPageBuffer, DstPageBaseOffset + Cluster.AttributeOffset, VertexIndex* Cluster.BitsPerAttribute,
						SrcPageBuffer, SrcPageBaseOffset + ClusterDiskHeader.AttributeDataOffset, NonRefVertexIndex* SrcBitsPerAttribute, Cluster.BitsPerAttribute);

//#endif
		}
	
		// Ref vertices
		for (uint RefVertexIndex = GroupIndex; RefVertexIndex < ClusterDiskHeader.NumVertexRefs; RefVertexIndex += TRANSCODE_THREADS_PER_GROUP)
		{
			const uint VertexIndex = GroupRefToVertex[RefVertexIndex];
			const uint PageClusterIndex = ReadByte(SrcPageBuffer, SrcPageBaseOffset + ClusterDiskHeader.VertexRefDataOffset + RefVertexIndex);
			const uint PageClusterData = SrcPageBuffer.Load(SrcPageBaseOffset + ClusterDiskHeader.PageClusterMapOffset + PageClusterIndex * 4);

			uint ParentPageIndex = PageClusterData >> NANITE_MAX_CLUSTERS_PER_PAGE_BITS;
			uint SrcLocalClusterIndex = BitFieldExtractU32(PageClusterData, NANITE_MAX_CLUSTERS_PER_PAGE_BITS, 0);
			uint SrcCodedVertexIndex = ReadByte(SrcPageBuffer, SrcPageBaseOffset + ClusterDiskHeader.VertexRefDataOffset + RefVertexIndex + PageDiskHeader.NumVertexRefs);

			FClusterDiskHeader SrcClusterDiskHeader = GetClusterDiskHeader(SrcPageBaseOffset, SrcLocalClusterIndex);
			FCluster SrcCluster;
			uint ParentGPUPageIndex = 0;
			
			const bool bIsParentRef = (ParentPageIndex != 0);
			if (bIsParentRef)
			{
				ParentGPUPageIndex = PageDependenciesBuffer[InstallInfo.PageDependenciesStart + (ParentPageIndex - 1)];
				uint ParentPageAddress = GPUPageIndexToGPUOffset(ParentGPUPageIndex);
				FPageHeader ParentPageHeader = GetPageHeader(DstPageBuffer, ParentPageAddress);
				SrcCluster = GetCluster(DstPageBuffer, ParentPageAddress, SrcLocalClusterIndex, ParentPageHeader.NumClusters);
			}
			else
			{
				SrcCluster = GetCluster(SrcPageBuffer, SrcPackedClusterOffset, SrcLocalClusterIndex, PageDiskHeader.NumClusters);
			}
			
			// Transcode position 
			{
				int3 SrcPosition;
				if (bIsParentRef)
				{
					uint BaseAddress = GPUPageIndexToGPUOffset(ParentGPUPageIndex) + SrcCluster.PositionOffset;
					uint SrcPositionBitsPerVertex = SrcCluster.PosBits.x + SrcCluster.PosBits.y + SrcCluster.PosBits.z;
					FBitStreamReaderState InputStream = BitStreamReader_Create(BaseAddress, SrcCodedVertexIndex * SrcPositionBitsPerVertex, 3 * NANITE_MAX_POSITION_QUANTIZATION_BITS);
					SrcPosition = BitStreamReader_Read3_RW(DstPageBuffer, InputStream, SrcCluster.PosBits, NANITE_MAX_POSITION_QUANTIZATION_BITS);
				}
				else
				{
					uint BaseAddress = SrcPageBaseOffset + SrcClusterDiskHeader.PositionDataOffset;
					uint SrcPositionBitsPerVertex = (SrcCluster.PosBits.x + SrcCluster.PosBits.y + SrcCluster.PosBits.z + 7) & ~7u;
					FBitStreamReaderState InputStream = BitStreamReader_Create(BaseAddress, SrcCodedVertexIndex * SrcPositionBitsPerVertex, 3 * NANITE_MAX_POSITION_QUANTIZATION_BITS);
					SrcPosition = BitStreamReader_Read3_RO(SrcPageBuffer, InputStream, SrcCluster.PosBits, NANITE_MAX_POSITION_QUANTIZATION_BITS);
				}

				int3 DstPosition = SrcPosition + SrcCluster.PosStart - Cluster.PosStart;
				const uint PositionBitsPerVertex = Cluster.PosBits.x + Cluster.PosBits.y + Cluster.PosBits.z;
				
				FBitStreamWriterState OutputStream = BitStreamWriter_Create_Aligned(DstPageBaseOffset + Cluster.PositionOffset, VertexIndex * PositionBitsPerVertex);
				BitStreamWriter_Writer(DstPageBuffer, OutputStream, DstPosition.x, Cluster.PosBits.x, NANITE_MAX_POSITION_QUANTIZATION_BITS);
				BitStreamWriter_Writer(DstPageBuffer, OutputStream, DstPosition.y, Cluster.PosBits.y, NANITE_MAX_POSITION_QUANTIZATION_BITS);
				BitStreamWriter_Writer(DstPageBuffer, OutputStream, DstPosition.z, Cluster.PosBits.z, NANITE_MAX_POSITION_QUANTIZATION_BITS);
				BitStreamWriter_Flush(DstPageBuffer, OutputStream);
			}

			// Specialize vertex transcoding codegen for each of the possible values for NumTexCoords
			//if (NumTexCoords == 0)
			//{
			//	TranscodeVertexAttributes(PageDiskHeader, Cluster, DstPageBaseOffset, LocalClusterIndex, VertexIndex,
			//		SrcCluster, SrcClusterDiskHeader, SrcPageBaseOffset, SrcLocalClusterIndex, SrcCodedVertexIndex, bIsParentRef, ParentGPUPageIndex, 0);
			//}
			//else if(NumTexCoords == 1)
            {
				// Only Work with 1 tex coords
                TranscodeVertexAttributes(PageDiskHeader, Cluster, DstPageBaseOffset, LocalClusterIndex, VertexIndex,
                    SrcCluster, SrcClusterDiskHeader, SrcPageBaseOffset, SrcLocalClusterIndex, SrcCodedVertexIndex, bIsParentRef, ParentGPUPageIndex, 1);
            }
            //else if(NumTexCoords == 2)
            //{
            //    TranscodeVertexAttributes(PageDiskHeader, Cluster, DstPageBaseOffset, LocalClusterIndex, VertexIndex,
            //        SrcCluster, SrcClusterDiskHeader, SrcPageBaseOffset, SrcLocalClusterIndex, SrcCodedVertexIndex, bIsParentRef, ParentGPUPageIndex, 2);
            //}
            //else if(NumTexCoords == 3)
            //{
            //    TranscodeVertexAttributes(PageDiskHeader, Cluster, DstPageBaseOffset, LocalClusterIndex, VertexIndex,
            //        SrcCluster, SrcClusterDiskHeader, SrcPageBaseOffset, SrcLocalClusterIndex, SrcCodedVertexIndex, bIsParentRef, ParentGPUPageIndex, 3);
            //}
            //else if(NumTexCoords == 4)
            //{
            //    TranscodeVertexAttributes(PageDiskHeader, Cluster, DstPageBaseOffset, LocalClusterIndex, VertexIndex,
            //        SrcCluster, SrcClusterDiskHeader, SrcPageBaseOffset, SrcLocalClusterIndex, SrcCodedVertexIndex, bIsParentRef, ParentGPUPageIndex, 4);
            //}
		}
	}
}
