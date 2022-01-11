/*
	TiX Engine v2.0 Copyright (C) 2018~2021
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	class TI_API TShape
	{
	public:
		static void CreateICOSphere(
			uint32 Frequency,
			const FFloat3& Center, 
			float Radius, 
			TVector<FFloat3>& OutPositions, 
			TVector<uint32>& OutIndices);

		static void CreateBox(
			const FFloat3& Center, 
			const FFloat3& Edges, 
			const FQuat& Rotation, 
			TVector<FFloat3>& OutPositions, 
			TVector<uint32>& OutIndices);

		static void CreateCapsule(
			uint32 Latitude,
			uint32 Longitude,
			const FFloat3& Center, 
			float Radius, 
			float Length, 
			const FQuat& Rotation, 
			TVector<FFloat3>& OutPositions, 
			TVector<uint32>& OutIndices);

		static void RecalcNormal(
			const TVector<FFloat3>& Positions,
			const TVector<uint32>& Indices,
			TVector<FFloat3>& OutNormals);
	};
}
