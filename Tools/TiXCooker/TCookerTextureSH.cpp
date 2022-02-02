/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"

#include "ResHelper.h"
#include "TCookerTexture.h"
#include "TImage.h"
#include "TCookerMultiThreadTask.h"
#include "TCookerTextureTaskHelper.h"

// Calc Spherical Harmonic for IBL, from UE5 ReflectionEnvironmentDiffuseIrradiance.cpp ComputeDiffuseIrradiance()
namespace tix
{
	inline FFloat3 GetCubemapVector(const FFloat2& InUV, int32 InCubeFace)
	{
		FFloat3 CubeCoordinates;

		if (InCubeFace == 0)
		{
			CubeCoordinates = FFloat3(1, -InUV.Y, -InUV.X);
		}
		else if (InCubeFace == 1)
		{
			CubeCoordinates = FFloat3(-1, -InUV.Y, InUV.X);
		}
		else if (InCubeFace == 2)
		{
			CubeCoordinates = FFloat3(InUV.X, 1, InUV.Y);
		}
		else if (InCubeFace == 3)
		{
			CubeCoordinates = FFloat3(InUV.X, -1, -InUV.Y);
		}
		else if (InCubeFace == 4)
		{
			CubeCoordinates = FFloat3(InUV.X, -InUV.Y, 1);
		}
		else
		{
			CubeCoordinates = FFloat3(-InUV.X, -InUV.Y, -1);
		}

		return CubeCoordinates;
	}

	inline float GetSHCoefficient(const FFloat3& InVec, int32 CoIndex)
	{
		FFloat3 VecSqr = InVec * InVec;
		switch (CoIndex)
		{
		case 0:
			return 0.282095f;
		case 1:
			return -0.488603f * InVec.Y;
		case 2:
			return 0.488603f * InVec.Z;
		case 3:
			return -0.488603f * InVec.X;

		case 4:
			return 1.092548f * InVec.X * InVec.Y;
		case 5:
			return -1.092548f * InVec.Y * InVec.Z;
		case 6:
			return 0.315392f * (3.0f * VecSqr.Z - 1.0f);
		case 7:
			return -1.092548f * InVec.X * InVec.Z;

		case 8:
			return 0.546274f * (VecSqr.X - VecSqr.Y);
		default:
			// CoIndex should be in range [0, 8]
			RuntimeFail();
			return 0;
		}
	}

	SColorf SampleParentMip(TImage* Image, const FFloat2& InUV, int32 SrcMipIndex)
	{
		const int32 W = Image->GetMipmap(SrcMipIndex).W;
		const int32 H = Image->GetMipmap(SrcMipIndex).H;

		int32 X0 = (int32)floor(InUV.X * W);
		int32 Y0 = (int32)floor(InUV.Y * H);

		X0 = TMath::Clamp(X0, 0, W - 1);
		Y0 = TMath::Clamp(Y0, 0, H - 1);

		return Image->GetPixelFloat(X0, Y0, SrcMipIndex);
	}

	void TCookerTexture::ComputeDiffuseIrradiance(TResTextureDefine* SrcImage, FSHVectorRGB3& OutIrrEnvMap)
	{
		static const int32 TargetSize = 32;
		TI_ASSERT(SrcImage->Desc.Type == ETT_TEXTURE_2D);
		TI_ASSERT(SrcImage->Desc.Width == SrcImage->Desc.Height * 2);
		TI_ASSERT(TMath::IsPowerOf2(SrcImage->Desc.Width));
		TI_ASSERT(SrcImage->Desc.Height >= TargetSize);

		TImage* LongLatImage = SrcImage->ImageSurfaces[0];

		// Downsample to 64x32
		int32 TargetMip = 0;
		if (SrcImage->Desc.Height > TargetSize && LongLatImage->GetMipmapCount() == 1)
		{
			LongLatImage->GenerateMipmaps();
		}
		int32 H = SrcImage->Desc.Height;
		while (H > TargetSize)
		{
			++TargetMip;
			H /= 2;
		}
		TImage* LongLat32 = ti_new TImage(LongLatImage->GetFormat(), TargetSize * 2, TargetSize);
		uint8* Data = LongLat32->Lock();
		memcpy(Data, LongLatImage->GetMipmap(TargetMip).Data.GetBuffer(), LongLatImage->GetMipmap(TargetMip).Data.GetLength());
		LongLat32->Unlock();
		LongLat32->GenerateMipmaps();

		// Allocate cube faces
		TVector<TImage*> TargetCubeFaces;
		TargetCubeFaces.resize(6);
		for (int32 Face = 0; Face < 6; ++Face)
		{
			TargetCubeFaces[Face] = ti_new TImage(LongLat32->GetFormat(), TargetSize, TargetSize);
			TargetCubeFaces[Face]->AllocEmptyMipmaps();
		}

		TVector<SColorf> CoefficientData;
		CoefficientData.resize(FSHVector3::MaxSHBasis);

		const float InvSize = 1.f / TargetSize;
		for (int32 CoefficientIndex = 0; CoefficientIndex < FSHVector3::MaxSHBasis; CoefficientIndex++)
		{
			// Copy the starting mip from the lighting texture, apply texel area weighting and appropriate SH coefficient
			for (int32 Face = 0; Face < 6; ++Face)
			{
				for (int32 Y = 0; Y < TargetSize; Y++)
				{
					for (int32 X = 0; X < TargetSize; X++)
					{
						FFloat2 UV = FFloat2((X + 0.5f) * InvSize * 2.f - 1.f, (Y + 0.5f) * InvSize * 2.f - 1.f);
						FFloat3 CubeCoord = GetCubemapVector(UV, Face);
						CubeCoord.Normalize();

						float SquaredUVs = 1.f + UV.Dot(UV);
						// Dividing by NumSamples here to keep the sum in the range of fp16, once we get down to the 1x1 mip
						float TexelWeight = 4.f / (TMath::Sqrt(SquaredUVs) * SquaredUVs);

						float CurrentSHCoefficient = GetSHCoefficient(CubeCoord, CoefficientIndex);

						SColorf TexelLighting = SampleLongLatLinear(LongLat32, CubeCoord, 0);
						TexelLighting *= CurrentSHCoefficient * TexelWeight;
						TexelLighting.A = TexelWeight;

						TargetCubeFaces[Face]->SetPixel(X, Y, TexelLighting);
					}
				}
			}

			// Accumulate all the texel values through downsampling to 1x1 mip
			const int32 NumMips = 6;
			for (int32 MipIndex = 1; MipIndex < NumMips; MipIndex++)
			{
				const int32 SourceMipIndex = MipIndex - 1;
				const int32 MipSize = 1 << (NumMips - MipIndex - 1);

				const float InvMipSize = 1.f / (MipSize);
				const float InvSrcMipSize = 1.f / (MipSize * 2);
				const float SourceTexelSize = InvSrcMipSize;

				for (int32 Face = 0; Face < 6; ++Face)
				{
					for (int32 Y = 0; Y < MipSize; Y++)
					{
						for (int32 X = 0; X < MipSize; X++)
						{
							FFloat2 UV = FFloat2((X + 0.5f) * InvMipSize, (Y + 0.5f) * InvMipSize);

							SColorf Acc(0, 0, 0, 0);

							Acc += SampleParentMip(TargetCubeFaces[Face], UV + FFloat2(-SourceTexelSize, -SourceTexelSize), SourceMipIndex);
							Acc += SampleParentMip(TargetCubeFaces[Face], UV + FFloat2(SourceTexelSize, -SourceTexelSize), SourceMipIndex);
							Acc += SampleParentMip(TargetCubeFaces[Face], UV + FFloat2(-SourceTexelSize, SourceTexelSize), SourceMipIndex);
							Acc += SampleParentMip(TargetCubeFaces[Face], UV + FFloat2(SourceTexelSize, SourceTexelSize), SourceMipIndex);

							Acc *= 1.f / 4.f;
							TargetCubeFaces[Face]->SetPixel(X, Y, Acc, MipIndex);
						}
					}
				}
			}

			if (!false)
			{
				char name[64];
				sprintf(name, "SH%d", CoefficientIndex);
				ExportCubeMap(TargetCubeFaces, name);

				TVector<TImage*> UEResult;
				UEResult.resize(6);
				for (int32 Face = 0; Face < 6; ++Face)
				{
					UEResult[Face] = ti_new TImage(LongLat32->GetFormat(), TargetSize, TargetSize);
					UEResult[Face]->AllocEmptyMipmaps();
				}

				// Load UE json result, and convert to hdr
				for (int32 MipIndex = 0; MipIndex < 6; MipIndex++)
				{
					for (int32 Face = 0; Face < 6; Face++)
					{
						int8 Name[64];
						sprintf(Name, "C%d_Face%d_Mip%d.json", CoefficientIndex, Face, MipIndex);

						// Load tjs file to buffer
						TFile TjsFile;
						TjsFile.Open(Name, EFA_READ);
						int8* FileContent = ti_new int8[TjsFile.GetSize() + 1];
						TjsFile.Read(FileContent, TjsFile.GetSize(), TjsFile.GetSize());
						FileContent[TjsFile.GetSize()] = 0;
						TjsFile.Close();

						// Parse json file
						TJSON JsonDoc;
						JsonDoc.Parse(FileContent);

						TVector<float> Data;
						JsonDoc["data"] << Data;

						uint8* ImageData = UEResult[Face]->Lock(MipIndex);
						memcpy(ImageData, Data.data(), Data.size() * sizeof(float));
						UEResult[Face]->Unlock();

						ti_delete[] FileContent;
					}
				}
				sprintf(name, "UE%d", CoefficientIndex);
				ExportCubeMap(UEResult, name);
				for (auto Cube : UEResult)
				{
					ti_delete Cube;
				}
			}

			// Gather the cubemap face results and normalize
			SColorf Acc(0, 0, 0, 0);
			for (int32 Face = 0; Face < 6; ++Face)
			{
				Acc += TargetCubeFaces[Face]->GetPixelFloat(0, 0, NumMips - 1);
			}
			float Weight = TMath::Max(Acc.A, 0.00001f);
			CoefficientData[CoefficientIndex] = Acc * (PI * 4.f) / Weight;
			CoefficientData[CoefficientIndex].A = 0.f;
		}

		for (int32 CoefficientIndex = 0; CoefficientIndex < FSHVector3::MaxSHBasis; CoefficientIndex++)
		{
			OutIrrEnvMap.R.V[CoefficientIndex] = CoefficientData[CoefficientIndex].R;
			OutIrrEnvMap.G.V[CoefficientIndex] = CoefficientData[CoefficientIndex].G;
			OutIrrEnvMap.B.V[CoefficientIndex] = CoefficientData[CoefficientIndex].B;
		}

		// Delete allocated images
		for (auto Cube : TargetCubeFaces)
		{
			ti_delete Cube;
		}
		ti_delete LongLat32;
	}
}
