/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

#if COMPILE_WITH_RHI_DX12
#include <dxgi1_6.h>
#include <d3d12.h>
#include "d3dx12.h"
#include "FAccelerationStructure.h"

namespace tix
{
	class FBottomLevelAccelerationStructureDx12 : public FBottomLevelAccelerationStructure
	{
	public:
		FBottomLevelAccelerationStructureDx12();
		virtual ~FBottomLevelAccelerationStructureDx12();

		virtual void AddMeshBuffer(FVertexBufferPtr InVB, FIndexBufferPtr InIB) override;
		virtual void Build() override;
		virtual bool AlreadyBuilt() override;

		ID3D12Resource* GetASResource()
		{
			return AccelerationStructure.Get();
		}
	private:
		ComPtr<ID3D12Resource> AccelerationStructure;
		ComPtr<ID3D12Resource> ScratchResource;

		TVector<D3D12_RAYTRACING_GEOMETRY_DESC> GeometryDescs;
	};

	////////////////////////////////////////////////////////////
	class FTopLevelAccelerationStructureDx12 : public FTopLevelAccelerationStructure
	{
	public:
		FTopLevelAccelerationStructureDx12();
		virtual ~FTopLevelAccelerationStructureDx12();

		virtual void ClearAllInstances() override;
		virtual void ReserveInstanceCount(uint32 Count) override;
		virtual void AddBLASInstance(FBottomLevelAccelerationStructurePtr BLAS, const FMat34& Transform) override;
		virtual void Build() override;
		virtual bool AlreadyBuilt() override;
	private:
		ComPtr<ID3D12Resource> AccelerationStructure;
		ComPtr<ID3D12Resource> ScratchResource;

		TVector<D3D12_RAYTRACING_INSTANCE_DESC> InstanceDescs;
		THMap<FBottomLevelAccelerationStructurePtr, int32> BLASes;

		friend class FRHIDx12;
	};
}
#endif	// COMPILE_WITH_RHI_DX12