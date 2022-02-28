/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	// Structure def from UE4
	/** A vector of spherical harmonic coefficients. */
	template<int32 Order>
	class FSHVector
	{
	public:

		enum { MaxSHOrder = Order };
		enum { MaxSHBasis = MaxSHOrder * MaxSHOrder };
		enum { NumComponentsPerSIMDVector = 4 };
		enum { NumSIMDVectors = (MaxSHBasis + NumComponentsPerSIMDVector - 1) / NumComponentsPerSIMDVector };
		enum { NumTotalFloats = NumSIMDVectors * NumComponentsPerSIMDVector };
		float V[NumTotalFloats];

		/** The integral of the constant SH basis. */
		static constexpr float ConstantBasisIntegral = 3.5449077018110320545963349666823f; // 2 * Sqrt(PI)

		/** Default constructor. */
		FSHVector()
		{
			memset(V, 0, sizeof(V));
		}

		FSHVector(float V0, float V1, float V2, float V3)
		{
			memset(V, 0, sizeof(V));

			V[0] = V0;
			V[1] = V1;
			V[2] = V2;
			V[3] = V3;
		}

		explicit FSHVector(const FFloat4& Vector)
		{
			memset(V, 0, sizeof(V));

			V[0] = Vector.X;
			V[1] = Vector.Y;
			V[2] = Vector.Z;
			V[3] = Vector.W;
		}

		template<int32 OtherOrder>
		explicit FSHVector(const FSHVector<OtherOrder> & Other)
		{
			if (Order <= OtherOrder)
			{
				memcpy(V, Other.V, sizeof(V));
			}
			else
			{
				memset(V, 0, sizeof(V));
				memcpy(V, Other.V, sizeof(V));
			}
		}
	};
	typedef FSHVector<3> FSHVector3;

	/////////////////////////////////////////////////////////////

	/** A vector of colored spherical harmonic coefficients. */
	template<int32 MaxSHOrder>
	class FSHVectorRGB
	{
	public:
		enum { NumTotalFloats = FSHVector<MaxSHOrder>::NumTotalFloats * 3 };

		FSHVector<MaxSHOrder> R;
		FSHVector<MaxSHOrder> G;
		FSHVector<MaxSHOrder> B;

		FSHVectorRGB() {}

		template<int32 OtherOrder>
		explicit FSHVectorRGB(const FSHVectorRGB<OtherOrder>& Other)
		{
			R = (FSHVector<MaxSHOrder>)Other.R;
			G = (FSHVector<MaxSHOrder>)Other.G;
			B = (FSHVector<MaxSHOrder>)Other.B;
		}

		/** Calculates greyscale spherical harmonic coefficients. */
		FSHVector<MaxSHOrder> GetLuminance() const
		{
			return R * 0.3f + G * 0.59f + B * 0.11f;
		}
	};

	typedef FSHVectorRGB<3> FSHVectorRGB3;
}
