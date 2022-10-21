// Copyright Epic Games, Inc. All Rights Reserved.

//#pragma once

#define UlongType uint2

UlongType PackUlongType(uint2 Value)
{
	return Value;
}

uint2 UnpackUlongType(UlongType Value)
{
	return Value;
}

float  CondMask(bool Cond, float  Src0, float  Src1) { return Cond ? Src0 : Src1; }
float2 CondMask(bool Cond, float2 Src0, float2 Src1) { return Cond ? Src0 : Src1; }
float3 CondMask(bool Cond, float3 Src0, float3 Src1) { return Cond ? Src0 : Src1; }
float4 CondMask(bool Cond, float4 Src0, float4 Src1) { return Cond ? Src0 : Src1; }

int  CondMask(bool Cond, int  Src0, int  Src1) { return Cond ? Src0 : Src1; }
int2 CondMask(bool Cond, int2 Src0, int2 Src1) { return Cond ? Src0 : Src1; }
int3 CondMask(bool Cond, int3 Src0, int3 Src1) { return Cond ? Src0 : Src1; }
int4 CondMask(bool Cond, int4 Src0, int4 Src1) { return Cond ? Src0 : Src1; }

uint  CondMask(bool Cond, uint  Src0, uint  Src1) { return Cond ? Src0 : Src1; }
uint2 CondMask(bool Cond, uint2 Src0, uint2 Src1) { return Cond ? Src0 : Src1; }
uint3 CondMask(bool Cond, uint3 Src0, uint3 Src1) { return Cond ? Src0 : Src1; }
uint4 CondMask(bool Cond, uint4 Src0, uint4 Src1) { return Cond ? Src0 : Src1; }
// Software emulation using SM5/GCN semantics.
// Fast as long as shifts, sizes and offsets are compile-time constant.
// TODO: Should we consider weaker semantics to allow for a more efficient implementation in the dynamic case?

uint BitFieldInsertU32(uint Mask, uint Preserve, uint Enable)
{
	return (Preserve & Mask) | (Enable & ~Mask);
}

uint BitFieldExtractU32(uint Data, uint Size, uint Offset)
{
	// Shift amounts are implicitly &31 in HLSL, so they should be optimized away on most platforms
	// In GLSL shift amounts < 0 or >= word_size are undefined, so we better be explicit
	Size &= 31;
	Offset &= 31;
	return (Data >> Offset) & ((1u << Size) - 1u);
}

int BitFieldExtractI32(int Data, uint Size, uint Offset)
{
	Size &= 31u;
	Offset &= 31u;
	const uint Shift = (32u - Size) & 31u;
	const int Value = (Data >> Offset) & int((1u << Size) - 1u);
	return (Value << Shift) >> Shift;
}

uint BitFieldMaskU32(uint MaskWidth, uint MaskLocation)
{
	MaskWidth &= 31u;
	MaskLocation &= 31u;

	return ((1u << MaskWidth) - 1u) << MaskLocation;
}

uint BitAlignU32(uint High, uint Low, uint Shift)
{
	Shift &= 31u;

	uint Result = Low >> Shift;
	Result |= Shift > 0u ? (High << (32u - Shift)) : 0u;
	return Result;
}

uint ByteAlignU32(uint High, uint Low, uint Shift)
{
	return BitAlignU32(High, Low, Shift * 8);
}


uint3 UnpackToUint3(uint Value, int3 NumComponentBits)
{
	return uint3(BitFieldExtractU32(Value, NumComponentBits.x, 0),
				 BitFieldExtractU32(Value, NumComponentBits.y, NumComponentBits.x),
				 BitFieldExtractU32(Value, NumComponentBits.z, NumComponentBits.x + NumComponentBits.y));
}

uint4 UnpackToUint4(uint Value, int4 NumComponentBits)
{
	return uint4(BitFieldExtractU32(Value, NumComponentBits.x, 0),
				 BitFieldExtractU32(Value, NumComponentBits.y, NumComponentBits.x),
				 BitFieldExtractU32(Value, NumComponentBits.z, NumComponentBits.x + NumComponentBits.y),
				 BitFieldExtractU32(Value, NumComponentBits.w, NumComponentBits.x + NumComponentBits.y + NumComponentBits.z));
}

uint FloatToUIntScaled(float Value, float Scale)
{
	return (uint)floor(Value * Scale + 0.5f);
}

uint Pack_Float4_To_R10G10B10A2_UNORM(float4 Unpacked)
{
	const float4 UnpackedClamped = saturate(Unpacked);
	uint Packed = ((FloatToUIntScaled(UnpackedClamped.x, 1023))       |
				   (FloatToUIntScaled(UnpackedClamped.y, 1023) << 10) |
				   (FloatToUIntScaled(UnpackedClamped.z, 1023) << 20) |
				   (FloatToUIntScaled(UnpackedClamped.w,    3) << 30));
	return Packed;
}

float4 Unpack_R10G10B10A2_UNORM_To_Float4(uint Packed)
{
	float4 Unpacked;
	Unpacked.x = (float)(((Packed      ) & 0x000003FF)) / 1023;
	Unpacked.y = (float)(((Packed >> 10) & 0x000003FF)) / 1023;
	Unpacked.z = (float)(((Packed >> 20) & 0x000003FF)) / 1023;
	Unpacked.w = (float)(((Packed >> 30) & 0x00000003)) / 3;
	return Unpacked;
}

// Implement BitStreamReader for ByteAddressBuffer (RO), RWByteAddressBuffer (RW) and dynamic choice (RORW).
struct FBitStreamReaderState
{
	uint	AlignedByteAddress;
	int		BitOffsetFromAddress;

	uint4	BufferBits;
	int		BufferOffset;

	int		CompileTimeMinBufferBits;
	int		CompileTimeMinDwordBits;
	int		CompileTimeMaxRemainingBits;
};

FBitStreamReaderState BitStreamReader_Create_Aligned(uint AlignedByteAddress, uint BitOffset, uint CompileTimeMaxRemainingBits)
{
	FBitStreamReaderState State;

	State.AlignedByteAddress = AlignedByteAddress;
	State.BitOffsetFromAddress = BitOffset;

	State.BufferBits = 0;
	State.BufferOffset = 0;

	State.CompileTimeMinBufferBits = 0;
	State.CompileTimeMinDwordBits = 0;
	State.CompileTimeMaxRemainingBits = CompileTimeMaxRemainingBits;

	return State;
}

FBitStreamReaderState BitStreamReader_Create(uint ByteAddress, uint BitOffset, uint CompileTimeMaxRemainingBits)
{
	uint AlignedByteAddress = ByteAddress & ~3u;
	BitOffset += (ByteAddress & 3u) << 3;
	return BitStreamReader_Create_Aligned(AlignedByteAddress, BitOffset, CompileTimeMaxRemainingBits);
}

#define TYPE_SUFFIX RO
#define RORW_ENABLED 0
#define INPUT_BUFFER_PARAMS ByteAddressBuffer InputBuffer
#define INPUT_BUFFER_ARGS InputBuffer
#include "BitStreamReaderImplementation.ush"
#undef TYPE_SUFFIX
#undef RORW_ENABLED
#undef INPUT_BUFFER_PARAMS
#undef INPUT_BUFFER_ARGS

#define TYPE_SUFFIX RW
#define RORW_ENABLED 0
#define INPUT_BUFFER_PARAMS RWByteAddressBuffer InputBuffer
#define INPUT_BUFFER_ARGS InputBuffer
#include "BitStreamReaderImplementation.ush"
#undef TYPE_SUFFIX
#undef RORW_ENABLED
#undef INPUT_BUFFER_PARAMS
#undef INPUT_BUFFER_ARGS

#define TYPE_SUFFIX RORW
#define RORW_ENABLED 1
#define INPUT_BUFFER_PARAMS ByteAddressBuffer InputBufferRO, RWByteAddressBuffer InputBufferRW, bool bRW
#define INPUT_BUFFER_ARGS InputBufferRO, InputBufferRW, bRW
#include "BitStreamReaderImplementation.ush"
#undef TYPE_SUFFIX
#undef RORW_ENABLED
#undef INPUT_BUFFER_PARAMS
#undef INPUT_BUFFER_ARGS

// Put bits to ByteAddressBuffer at bit offset. NumBits must be <= 31.
void PutBits(RWByteAddressBuffer Output, uint AlignedBaseAddress, uint BitOffset, uint Value, uint NumBits)
{
    uint BitOffsetInDword = (BitOffset & 31u);  // &31 is implicit in shifts
    
    uint Bits = Value << BitOffsetInDword;
    uint Address = AlignedBaseAddress + ((BitOffset >> 5) << 2);
    uint EndBitPos = BitOffsetInDword + NumBits;

    if (EndBitPos >= 32)
    {
        uint Mask = 0xFFFFFFFFu << (EndBitPos & 31u);
        Output.InterlockedAnd(Address + 4, Mask);
        Output.InterlockedOr(Address + 4, Value >> (32 - BitOffsetInDword));
    }

    {
        uint Mask = ~BitFieldMaskU32(NumBits, BitOffset);
        Output.InterlockedAnd(Address, Mask);
        Output.InterlockedOr(Address, Value << BitOffsetInDword);
    }
}

struct FBitStreamWriterState
{
	uint AlignedByteAddress;
   	uint BufferBits;
    uint BufferOffset;
    uint BufferMask;
};

FBitStreamWriterState BitStreamWriter_Create_Aligned(uint AlignedBaseAddressInBytes, uint BitOffset)
{
	FBitStreamWriterState State;

	State.AlignedByteAddress = AlignedBaseAddressInBytes + ((BitOffset >> 5) << 2);
	BitOffset &= 31u;

	State.BufferBits = 0;
	State.BufferOffset = BitOffset;
	State.BufferMask = BitFieldMaskU32(BitOffset, 0);

	return State;
}

void BitStreamWriter_Writer(RWByteAddressBuffer Output, inout FBitStreamWriterState State, uint Value, int NumBits, int CompileTimeMaxBits)
{
    State.BufferBits |= Value << State.BufferOffset;

	// State.BufferOffset <= 31
    uint NextBufferOffset = State.BufferOffset + NumBits;
    
    if (NextBufferOffset >= 32)
    {
        Output.InterlockedAnd(State.AlignedByteAddress, State.BufferMask);
        Output.InterlockedOr(State.AlignedByteAddress, State.BufferBits);
		State.BufferMask = 0;
		
		// Shifts are mod 32, so we need special handling when shift could be >= 32.
		// State.BufferOffset can only be 0 here if NumBits >= 32 and therefore CompileTimeMaxBits >= 32.
		if(CompileTimeMaxBits >= 32)
			State.BufferBits = State.BufferOffset ? (Value >> (32 - State.BufferOffset)) : 0u;
		else
			State.BufferBits = Value >> (32 - State.BufferOffset);
        State.AlignedByteAddress += 4;
    }

	State.BufferOffset = NextBufferOffset & 31;
}

void BitStreamWriter_Flush(RWByteAddressBuffer Output, inout FBitStreamWriterState State)
{
    if (State.BufferOffset > 0)
    {
        uint Mask = State.BufferMask | ~BitFieldMaskU32(State.BufferOffset, 0);
        Output.InterlockedAnd(State.AlignedByteAddress, Mask);
        Output.InterlockedOr(State.AlignedByteAddress, State.BufferBits);
    }
}

// Utility functions for packing bits into uints.
// When Position and NumBits can be determined at compile time this should be just as fast as manual bit packing.
uint ReadBits(uint4 Data, inout uint Position, uint NumBits)
{
	uint DwordIndex = Position >> 5;
	uint BitIndex = Position & 31;

	uint Value = Data[DwordIndex] >> BitIndex;
	if (BitIndex + NumBits > 32)
	{
		Value |= Data[DwordIndex + 1] << (32 - BitIndex);
	}

	Position += NumBits;

	uint Mask = ((1u << NumBits) - 1u);
	return Value & Mask;
}

void WriteBits(inout uint4 Data, inout uint Position, uint Value, uint NumBits)
{
	uint DwordIndex = Position >> 5;
	uint BitIndex = Position & 31;

	Data[DwordIndex] |= Value << BitIndex;
	if (BitIndex + NumBits > 32)
	{
		Data[DwordIndex + 1] |= Value >> (32 - BitIndex);
	}

	Position += NumBits;
}
