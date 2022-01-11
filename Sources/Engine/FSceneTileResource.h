/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	class FSceneTileResource : public FRenderResource
	{
	public:
		FSceneTileResource();
		FSceneTileResource(const TSceneTileResource& InSceneTileResource);
		virtual ~FSceneTileResource();

		const FInt2& GetTilePosition() const
		{
			return Position;
		}

		const FBox& GetTileBBox() const
		{
			return BBox;
		}

		const TVector<FInt2>& GetInstanceCountAndOffset() const
		{
			return InstanceCountAndOffset;
		}

		FInstanceBufferPtr GetInstanceBuffer()
		{
			return InstanceBuffer;
		}

		uint32 GetTotalStaticMeshes() const
		{
			return TotalStaticMeshes;
		}

		uint32 GetTotalInstances() const
		{
			return TotalSMInstances;
		}

		void AddPrimitive(uint32 Index, FPrimitivePtr Primitive);
		void AppendPrimitive(FPrimitivePtr Primitive);

		const TVector<FPrimitivePtr>& GetPrimitives() const
		{
			return Primitives;
		}

		void CreateBLASAndInstances(FMeshBufferPtr MB, TInstanceBufferPtr InstanceData, const int32 InstanceCount, const int32 InstanceOffset);
		void BuildBLAS();

		THMap<FMeshBufferPtr, FBottomLevelAccelerationStructurePtr>& GetBLASes()
		{
			return SceneTileBLASes;
		}

		THMap<FMeshBufferPtr, TVector<FMat34>>& GetBLASInstances()
		{
			return SceneTileBLASInstances;
		}

	private:
		FInt2 Position;
		FBox BBox;
		uint32 TotalStaticMeshes;
		uint32 TotalSMInstances;
		// X is Count, Y is Offset
		TVector<FInt2> InstanceCountAndOffset;
		FInstanceBufferPtr InstanceBuffer;

		TVector<FPrimitivePtr> Primitives;

		// Scene BLAS, store by tiles. Each meshbuffer in this tile has a BLAS
		THMap<FMeshBufferPtr, FBottomLevelAccelerationStructurePtr> SceneTileBLASes;

		// Scene BLAS instances
		THMap<FMeshBufferPtr, TVector<FMat34> > SceneTileBLASInstances;
	};
}
