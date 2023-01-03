/*
	TiX Engine v3.0 Copyright (C) 2018~2021
	By ZhaoShuai tirax.cn@gmail.com
*/

// NaniteLearning Sample

static const int MaxTesselator = 26;

int CalcTessedPtCountInside(int TessFactorInside)
{
    int t1 = max(0, TessFactorInside - 1);
    int n = t1 / 2;
    return (t1 - n) * n * 3 + (t1 & 1);
}

int CalcTessedPtCount(uint4 TessFactor)
{
    return CalcTessedPtCountInside(int(TessFactor.w)) + TessFactor.x + TessFactor.y + TessFactor.z;
}

int CalcTessedTriCountInside(int TessFactorInside)
{
    int t = max(0, TessFactorInside - 2);
    int t01 = t & 1;
    int n = t / 2;
    return (n * n * 2 + n * 2 * t01) * 3 + t01;
}

int CalcTessedTriCount(uint4 TessFactor)
{
	return CalcTessedTriCountInside(int(TessFactor.w)) + 
		(TessFactor.w - 2) * 3 + TessFactor.x + TessFactor.y + TessFactor.z;
}

static const uint PackedVIndex0[59] = {
        554766608,572662306,858993186,858993459,1145254707,1145324612,1145324612,1431585860,
        1431655765,1431655765,1431655765,1717986645,1717986918,1717986918,1717986918,1986422374,
        2004318071,2004318071,2004318071,2004318071,2004318071,2290649223,2290649224,2290649224,
        2290649224,2290649224,2290649224,2576980376,2576980377,2576980377,2576980377,2576980377,
        2576980377,2845415833,2863311530,2863311530,2863311530,2863311530,2863311530,2863311530,
        2863311530,3149642410,3149642683,3149642683,3149642683,3149642683,3149642683,3149642683,
        3149642683,3435903931,3435973836,3435973836,3435973836,3435973836,3435973836,3435973836,
        3435973836,3435973836,838860,
};
static const uint PackedVIndex1[54] = {
        286330880,572657937,572662306,858993186,858993459,858993459,1145324612,1145324612,
        1145324612,1431655492,1431655765,1431655765,1431655765,1717982549,1717986918,1717986918,
        1717986918,1717986918,2004317798,2004318071,2004318071,2004318071,2004318071,2004318071,
        2290649224,2290649224,2290649224,2290649224,2290649224,2290649224,2576980104,2576980377,
        2576980377,2576980377,2576980377,2576980377,2576980377,2863307161,2863311530,2863311530,
        2863311530,2863311530,2863311530,2863311530,2863311530,3149642410,3149642683,3149642683,
        3149642683,3149642683,3149642683,3149642683,3149642683,3149642683,
};
static const uint PackedTIndex0[108] = {
        571543825,572662306,572662306,858993459,858993459,858993459,1144206131,1145324612,
        1145324612,1145324612,1145324612,1145324612,1431655765,1431655765,1431655765,1431655765,
        1431655765,1431655765,1716868437,1717986918,1717986918,1717986918,1717986918,1717986918,
        1717986918,1717986918,1717986918,2004318071,2004318071,2004318071,2004318071,2004318071,
        2004318071,2004318071,2004318071,2004318071,2289530743,2290649224,2290649224,2290649224,
        2290649224,2290649224,2290649224,2290649224,2290649224,2290649224,2290649224,2290649224,
        2576980377,2576980377,2576980377,2576980377,2576980377,2576980377,2576980377,2576980377,
        2576980377,2576980377,2576980377,2576980377,2862193049,2863311530,2863311530,2863311530,
        2863311530,2863311530,2863311530,2863311530,2863311530,2863311530,2863311530,2863311530,
        2863311530,2863311530,2863311530,3149642683,3149642683,3149642683,3149642683,3149642683,
        3149642683,3149642683,3149642683,3149642683,3149642683,3149642683,3149642683,3149642683,
        3149642683,3149642683,3434855355,3435973836,3435973836,3435973836,3435973836,3435973836,
        3435973836,3435973836,3435973836,3435973836,3435973836,3435973836,3435973836,3435973836,
        3435973836,3435973836,3435973836,3435973836,
};
static const uint PackedTIndex1[118] = {
        286331152,572592401,572662306,572662306,858923554,858993459,858993459,858993459,
        858993459,1145324611,1145324612,1145324612,1145324612,1145324612,1145324612,1431655764,
        1431655765,1431655765,1431655765,1431655765,1431655765,1431655765,1717917013,1717986918,
        1717986918,1717986918,1717986918,1717986918,1717986918,1717986918,1717986918,2004248166,
        2004318071,2004318071,2004318071,2004318071,2004318071,2004318071,2004318071,2004318071,
        2004318071,2004318071,2290649223,2290649224,2290649224,2290649224,2290649224,2290649224,
        2290649224,2290649224,2290649224,2290649224,2290649224,2290649224,2576980376,2576980377,
        2576980377,2576980377,2576980377,2576980377,2576980377,2576980377,2576980377,2576980377,
        2576980377,2576980377,2576980377,2863241625,2863311530,2863311530,2863311530,2863311530,
        2863311530,2863311530,2863311530,2863311530,2863311530,2863311530,2863311530,2863311530,
        2863311530,2863311530,3149572778,3149642683,3149642683,3149642683,3149642683,3149642683,
        3149642683,3149642683,3149642683,3149642683,3149642683,3149642683,3149642683,3149642683,
        3149642683,3149642683,3149642683,3435973835,3435973836,3435973836,3435973836,3435973836,
        3435973836,3435973836,3435973836,3435973836,3435973836,3435973836,3435973836,3435973836,
        3435973836,3435973836,3435973836,3435973836,3435973836,12,
};

uint UnpackVIndex0(uint index)
{
	uint element = index / 8;
	uint bit_offset = (index & 7) * 4;
	return BitFieldExtractU32(PackedVIndex0[element], 4, bit_offset);
}
uint UnpackVIndex1(uint index)
{
	uint element = index / 8;
	uint bit_offset = (index & 7) * 4;
	return BitFieldExtractU32(PackedVIndex1[element], 4, bit_offset);
}
uint UnpackTIndex0(uint index)
{
	uint element = index / 8;
	uint bit_offset = (index & 7) * 4;
	return BitFieldExtractU32(PackedTIndex0[element], 4, bit_offset);
}
uint UnpackTIndex1(uint index)
{
	uint element = index / 8;
	uint bit_offset = (index & 7) * 4;
	return BitFieldExtractU32(PackedTIndex1[element], 4, bit_offset);
}

static const float3 B0 = {0, 1, 0};
static const float3 B1 = {1, 0, 0};
static const float3 B2 = {0, 0, 1};
static const float3 BC = (B0 + B1 + B2) / 3.0;
static const float3 D0 = normalize(B0 - BC);
static const float3 D1 = normalize(B1 - BC);
static const float3 D2 = normalize(B2 - BC);
static const float EdgeLen = length(B0 - B1);

static const float Cos30Inv = 1.0 / cos(0.523599);
static const float3 Pts[3] = {B0, B1, B2};
//static const float3 Dirs[3] = {D0, D1, D2};
static const uint2 SideOrder[3] = {{0, 1}, {1, 2}, {2, 0}};

float3 GetTessedBaryCoord(uint4 TessFactor, int index)
{
	int NumInsidePts = CalcTessedPtCountInside(TessFactor.w);

	float3 Pos;
	[branch]
	if (index < NumInsidePts)
	{
		int T01 = TessFactor.w & 1;
		int Loop = (T01 == 0) ? UnpackVIndex0(index) : UnpackVIndex1(index);
		int Segs = max(1, Loop * 2 + T01);
		int PtStartInLoop = CalcTessedPtCountInside(Segs);
		int PtsInLoop = Segs * 3;
		int Side = (index - PtStartInLoop) / Segs;
		int SideIndex = (index - PtStartInLoop) - Segs * Side;
		float SegLen = EdgeLen / TessFactor.w;
		float R = (Loop + 0.5 * T01) * SegLen * Cos30Inv;
		float3 C0 = BC + D0 * R;
		float3 C1 = BC + D1 * R;
		float3 C2 = BC + D2 * R;
		
		float3 Corners[3] = {C0, C1, C2};

		int S0 = SideOrder[Side].x;
		int S1 = SideOrder[Side].y;
		float3 SideDir = (Corners[S1] - Corners[S0]) / Segs;
		
		Pos = Corners[S0] + SideDir * SideIndex;
	}
	else
	{
		float3 Corners[3] = {B0, B1, B2};
		int PtStartInSide[3] = {NumInsidePts, NumInsidePts + TessFactor.x, NumInsidePts + TessFactor.x + TessFactor.y};
		int SideSegs[3] = {TessFactor.x, TessFactor.y, TessFactor.z};
		int Side;
		if (index < PtStartInSide[1])
			Side = 0;
		else if (index < PtStartInSide[2])
			Side = 1;
		else
			Side = 2;
		int SideIndex = index - PtStartInSide[Side];

		int S0 = SideOrder[Side].x;
		int S1 = SideOrder[Side].y;
    	float3 SideDir = (Corners[S1] - Corners[S0]) / SideSegs[Side];
		
    	Pos = Corners[S0] + SideDir * SideIndex;
	}
	return Pos;
}

uint3 CreateSideTriangle0(
    int LocalIndex, 
	int SideCurrPtStart, int SidePrevPtStart, 
	int CurrLoopPtStart, int PtsInCurrLoop, 
	int PrevLoopPtStart, int PtsInPrevLoop, 
	int PrevSegs, int CurrSegs
)
{
    uint3 Triangle;
    Triangle.x = SideCurrPtStart + LocalIndex;
    Triangle.y = Triangle.x + 1;
    Triangle.y = Triangle.y >= CurrLoopPtStart + PtsInCurrLoop ? CurrLoopPtStart : Triangle.y;
    float f = float(PrevSegs) / (CurrSegs - 1);
    Triangle.z = SidePrevPtStart + (int)round(f * LocalIndex + 0.01);
    Triangle.z = Triangle.z >= PrevLoopPtStart + PtsInPrevLoop ? PrevLoopPtStart : Triangle.z;
	return Triangle;
}

uint3 CreateSideTriangle1(
    int LocalIndex, 
	int SideCurrPtStart, int SidePrevPtStart, 
	int PrevLoopPtStart, int PtsInPrevLoop, 
	int PrevSegs, int CurrSegs
)
{
    uint3 Triangle;
    Triangle.x = SidePrevPtStart + LocalIndex;
    Triangle.y = Triangle.x + 1;
    Triangle.y = Triangle.y >= PrevLoopPtStart + PtsInPrevLoop ? PrevLoopPtStart : Triangle.y;
    float f = float(CurrSegs - 1) / (PrevSegs);
    int start = floor((0.49f + LocalIndex) * f) + 1;
    Triangle.z = SideCurrPtStart + start;
	return Triangle;
}
uint3 GetTessedTriangle(uint4 TessFactor, uint index)
{
	int NumInsideTris = CalcTessedTriCountInside(TessFactor.w);

	uint3 Triangle;
	[branch]
	if (index < NumInsideTris)
	{
		int T01 = TessFactor.w & 1;
		int Loop = (T01 == 0) ? UnpackTIndex0(index) : UnpackTIndex1(index);
		int CurrSegs = Loop * 2 + T01;
		int PrevSegs = max(0, CurrSegs - 2);
		int PrevPtStart = CalcTessedPtCountInside(PrevSegs);
		PrevPtStart = index == 0 ? 2 * T01 : PrevPtStart;   // special deal with odd index = 0
		int CurrPtStart = CalcTessedPtCountInside(CurrSegs);
		int TriStartInLoop = CalcTessedTriCountInside(CurrSegs);
		int PtsInCurrLoop = CurrSegs * 3;
		int PtsInPrevLoop = PrevSegs * 3;
		int TrisPerSide = CurrSegs + PrevSegs;

		int IndexInLoop = index - TriStartInLoop;
		int Side = IndexInLoop / TrisPerSide;
		int IndexInSide = IndexInLoop - TrisPerSide * Side;
		int SideCurrPtStart = CurrPtStart + CurrSegs * Side;
		int SidePrevPtStart = PrevPtStart + PrevSegs * Side;
		[branch]
		if (IndexInSide < CurrSegs)
		{
			Triangle = CreateSideTriangle0(IndexInSide, SideCurrPtStart, SidePrevPtStart, CurrPtStart, PtsInCurrLoop, PrevPtStart, PtsInPrevLoop, PrevSegs, CurrSegs);
		}
		else 
		{
			Triangle = CreateSideTriangle1(IndexInSide - CurrSegs, SideCurrPtStart, SidePrevPtStart, PrevPtStart, PtsInPrevLoop, PrevSegs, CurrSegs);
		}
	}
	else
	{
		int CurrSegs = TessFactor.w;
		int PrevSegs = CurrSegs - 2;
		int PrevPtStart = CalcTessedPtCountInside(PrevSegs);
		int CurrPtStart = CalcTessedPtCountInside(CurrSegs);
		int PtsInCurrLoop = TessFactor.x + TessFactor.y + TessFactor.z;
		int PtsInPrevLoop = PrevSegs * 3;

		int TriStartInSide[3] = {NumInsideTris, NumInsideTris + PrevSegs + TessFactor.x, NumInsideTris + PrevSegs * 2 + TessFactor.x + TessFactor.y};
		int SideSegs[3] = {TessFactor.x, TessFactor.y, TessFactor.z};
		int SidePtsOffset[3] = {0, TessFactor.x, TessFactor.x + TessFactor.y};

		int Side;
		if (index < TriStartInSide[1])
		{
			Side = 0;
		}
		else if (index < TriStartInSide[2])
		{
			Side = 1;
		}
		else
		{
			Side = 2;
		}
		int IndexInSide = index - TriStartInSide[Side];
		
		int SideCurrPtStart = CurrPtStart + SidePtsOffset[Side];
		int SidePrevPtStart = PrevPtStart + PrevSegs * Side;

		[branch]
		if (IndexInSide < SideSegs[Side])
		{
			Triangle = CreateSideTriangle0(IndexInSide, SideCurrPtStart, SidePrevPtStart, CurrPtStart, PtsInCurrLoop, PrevPtStart, PtsInPrevLoop, PrevSegs, SideSegs[Side]);
		}
		else
		{
			Triangle = CreateSideTriangle1(IndexInSide - SideSegs[Side], SideCurrPtStart, SidePrevPtStart, PrevPtStart, PtsInPrevLoop, PrevSegs, SideSegs[Side]);
		}
	}
	return Triangle;
}

// Octahedron Normal Vectors
// [Cigolle 2014, "A Survey of Efficient Representations for Independent Unit Vectors"]
//						Mean	Max
// oct		8:8			0.33709 0.94424
// snorm	8:8:8		0.17015 0.38588
// oct		10:10		0.08380 0.23467
// snorm	10:10:10	0.04228 0.09598
// oct		12:12		0.02091 0.05874

float2 UnitVectorToOctahedron( float3 N )
{
	N.xy /= dot( 1, abs(N) );
	if( N.z <= 0 )
	{
		N.xy = ( 1 - abs(N.yx) ) * select( N.xy >= 0, float2(1,1), float2(-1,-1) );
	}
	return N.xy;
}

float3 OctahedronToUnitVector( float2 Oct )
{
	float3 N = float3( Oct, 1 - dot( 1, abs(Oct) ) );
	float t = max( -N.z, 0 );
	N.xy += select(N.xy >= 0, float2(-t, -t), float2(t, t));
	return normalize(N);
}

static const uint QuantizationMaxValue = (1 << 16) - 1;
static const float QuantizationScale = 0.5f * QuantizationMaxValue;
static const float QuantizationBias = 0.5f * QuantizationMaxValue + 0.5f;
static const float InvQuantizationMaxValue = 1.0f / QuantizationMaxValue;
uint EncodeNormalOctahedron(float3 N)
{
	float2 Oct = UnitVectorToOctahedron(N);

	float2 OctQ = clamp((Oct * QuantizationScale + QuantizationBias), float2(0, 0), float2(QuantizationMaxValue.xx));
	uint2 OctU = uint2(OctQ);
	return (OctU.y << 16) | (OctU.x);
}

float3 DecodeNormalOctahedron(uint N)
{
	uint2 U;
	U.x = N & 0xffff;
	U.y = N >> 16;
	float2 F;
	F = float2(U) * 2.f * InvQuantizationMaxValue - 1.f;
	return OctahedronToUnitVector(F);
}

uint EncodeUV(float2 UV)
{
	uint2 uUV = f32tof16(UV);
	return (uUV.y << 16) | (uUV.x);
}

float2 DecodeUV(uint UV)
{
	uint2 uUV;
	uUV.x = UV & 0xffff;
	uUV.y = UV >> 16;
	return f16tof32(uUV);
}