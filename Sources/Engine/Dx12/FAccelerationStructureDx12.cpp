/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "FRHIDx12.h"
#include "FRHIDx12Conversion.h"
#include "FAccelerationStructureDx12.h"
#include "FGPUBufferDx12.h"

#if COMPILE_WITH_RHI_DX12

namespace tix
{
	FBottomLevelAccelerationStructureDx12::FBottomLevelAccelerationStructureDx12()
	{}

	FBottomLevelAccelerationStructureDx12::~FBottomLevelAccelerationStructureDx12()
	{}

	void FBottomLevelAccelerationStructureDx12::AddMeshBuffer(FVertexBufferPtr InVB, FIndexBufferPtr InIB)
	{
		// Create Geometry Desc
		D3D12_RAYTRACING_GEOMETRY_DESC GeometryDesc = {};
		GeometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
		GeometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

		FGPUBufferDx12* VBDx12 = static_cast<FGPUBufferDx12*>(InVB->GetGPUResource().get());
		FGPUBufferDx12* IBDx12 = static_cast<FGPUBufferDx12*>(InIB->GetGPUResource().get());

		GeometryDesc.Triangles.IndexFormat = GetDxIndexFormat(InIB->GetDesc().IndexType);
		GeometryDesc.Triangles.IndexBuffer = IBDx12->GetResource()->GetGPUVirtualAddress();
		GeometryDesc.Triangles.IndexCount = InIB->GetDesc().IndexCount;

		GeometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;	// Position always be RGB32F
		GeometryDesc.Triangles.VertexBuffer.StartAddress = VBDx12->GetResource()->GetGPUVirtualAddress();
		GeometryDesc.Triangles.VertexBuffer.StrideInBytes = InVB->GetDesc().Stride;
		GeometryDesc.Triangles.VertexCount = InVB->GetDesc().VertexCount;

		GeometryDesc.Triangles.Transform3x4 = NULL;

		GeometryDescs.push_back(GeometryDesc); 
		MarkDirty();
	}

	bool FBottomLevelAccelerationStructureDx12::AlreadyBuilt()
	{
		return AccelerationStructure != nullptr;
	}

	void FBottomLevelAccelerationStructureDx12::Build()
	{
		TI_ASSERT(IsRenderThread());
		if (!Dirty)
			return;

		FRHIDx12* RHIDx12 = static_cast<FRHIDx12*>(FRHI::Get());
		ID3D12Device5* DXRDevice = RHIDx12->GetDXRDevice();
		ID3D12GraphicsCommandList4* DXRCommandList = RHIDx12->GetDXRCommandList();

		// Get the size requirements for the scratch and AS buffers.
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO PrebuildInfo = {};
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC BottomLevelBuildDesc = {};
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& BottomLevelInputs = BottomLevelBuildDesc.Inputs;
		BottomLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
		BottomLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		BottomLevelInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
		BottomLevelInputs.NumDescs = (uint32)GeometryDescs.size();
		BottomLevelInputs.pGeometryDescs = GeometryDescs.data();

		DXRDevice->GetRaytracingAccelerationStructurePrebuildInfo(&BottomLevelInputs, &PrebuildInfo);
		TI_ASSERT(PrebuildInfo.ResultDataMaxSizeInBytes > 0);

		// Allocate resource for BLAS
		TI_ASSERT(AccelerationStructure == nullptr);
		auto UploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		auto ASBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(PrebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
		VALIDATE_HRESULT(DXRDevice->CreateCommittedResource(
			&UploadHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&ASBufferDesc,
			D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
			nullptr,
			IID_PPV_ARGS(&AccelerationStructure)));
		DX_SETNAME(AccelerationStructure.Get(), GetResourceName());

		TI_ASSERT(ScratchResource == nullptr);
		auto ScratchBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(PrebuildInfo.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
		VALIDATE_HRESULT(DXRDevice->CreateCommittedResource(
			&UploadHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&ScratchBufferDesc,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			nullptr,
			IID_PPV_ARGS(&ScratchResource)));		

		// Build bottom layer AS
		BottomLevelBuildDesc.ScratchAccelerationStructureData = ScratchResource->GetGPUVirtualAddress();
		BottomLevelBuildDesc.DestAccelerationStructureData = AccelerationStructure->GetGPUVirtualAddress();

		// https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html
		// Array of vertex indices.If NULL, triangles are non - indexed.Just as with graphics, 
		// the address must be aligned to the size of IndexFormat. The memory pointed to must 
		// be in state D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE.Note that if an app wants 
		// to share index buffer inputs between graphics input assemblerand raytracing 
		// acceleration structure build input, it can always put a resource into a combination 
		// of read states simultaneously, 
		// e.g.D3D12_RESOURCE_STATE_INDEX_BUFFER | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
		ID3D12DescriptorHeap* DescriptorHeap = RHIDx12->GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		DXRCommandList->SetDescriptorHeaps(1, &DescriptorHeap);
		DXRCommandList->BuildRaytracingAccelerationStructure(&BottomLevelBuildDesc, 0, nullptr);

		Dirty = false;
	}

	////////////////////////////////////////////////////////////
	FTopLevelAccelerationStructureDx12::FTopLevelAccelerationStructureDx12()
	{
		InstanceDescData = ti_new TStream();
	}

	FTopLevelAccelerationStructureDx12::~FTopLevelAccelerationStructureDx12()
	{}

	void FTopLevelAccelerationStructureDx12::ClearAllInstances()
	{
		InstanceDescData->Reset();
		MarkDirty();
	}

	void FTopLevelAccelerationStructureDx12::ReserveInstanceCount(uint32 Count)
	{
		InstanceDescData->Reserve(Count * sizeof(D3D12_RAYTRACING_INSTANCE_DESC));
	}

	void FTopLevelAccelerationStructureDx12::AddBLASInstance(FBottomLevelAccelerationStructurePtr BLAS, const FMat34& Transform)
	{
		FBottomLevelAccelerationStructureDx12* BLASDx12 = static_cast<FBottomLevelAccelerationStructureDx12*>(BLAS.get());

		if (BLASDx12->GetASResource() != nullptr)
		{
			D3D12_RAYTRACING_INSTANCE_DESC Desc;
			memcpy(Desc.Transform, Transform.Data(), sizeof(float) * 3 * 4);
			Desc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
			Desc.InstanceID = 0;
			Desc.InstanceMask = 1;
			Desc.InstanceContributionToHitGroupIndex = 0;
			TI_TODO("Find correct InstanceContributionToHitGroupIndex");
			Desc.AccelerationStructure = BLASDx12->GetASResource()->GetGPUVirtualAddress();

			InstanceDescData->Put(&Desc, sizeof(D3D12_RAYTRACING_INSTANCE_DESC));

			if (BLASes.find(BLAS) == BLASes.end())
			{
				BLASes[BLAS] = 1;
			}
			else
			{
				BLASes[BLAS] ++;
			}
			MarkDirty();
		}
	}

	bool FTopLevelAccelerationStructureDx12::AlreadyBuilt()
	{
		return AccelerationStructure.Get() != nullptr;
	}

	void FTopLevelAccelerationStructureDx12::Build()
	{
		TI_ASSERT(IsRenderThread());
		if (!Dirty || InstanceDescData->GetLength() == 0)
			return;

		FRHIDx12* RHIDx12 = static_cast<FRHIDx12*>(FRHI::Get());
		ID3D12Device5* DXRDevice = RHIDx12->GetDXRDevice();
		ID3D12GraphicsCommandList4* DXRCommandList = RHIDx12->GetDXRCommandList();

		// Get the size requirements for the scratch and AS buffers.
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO PrebuildInfo = {};
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC TopLevelBuildDesc = {};
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& TopLevelInputs = TopLevelBuildDesc.Inputs;
		TopLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
		TopLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		TopLevelInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
		TopLevelInputs.NumDescs = (uint32)InstanceDescData->GetLength() / sizeof(D3D12_RAYTRACING_INSTANCE_DESC);

		DXRDevice->GetRaytracingAccelerationStructurePrebuildInfo(&TopLevelInputs, &PrebuildInfo);
		TI_ASSERT(PrebuildInfo.ResultDataMaxSizeInBytes > 0);

		// Allocate resource for TLAS
		bool NeedRecreateASBuffer = AccelerationStructure.Get() == nullptr ? true : false;
		if (!NeedRecreateASBuffer && AccelerationStructure->GetDesc().Width < PrebuildInfo.ResultDataMaxSizeInBytes)
		{
			NeedRecreateASBuffer = true;
		}
		if (NeedRecreateASBuffer)
		{
			auto UploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
			auto ASBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(PrebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

			VALIDATE_HRESULT(DXRDevice->CreateCommittedResource(
				&UploadHeapProperties,
				D3D12_HEAP_FLAG_NONE,
				&ASBufferDesc,
				D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
				nullptr,
				IID_PPV_ARGS(&AccelerationStructure)));
			DX_SETNAME(AccelerationStructure.Get(), GetResourceName());
		}

		bool NeedRecreateScratch = ScratchResource.Get() == nullptr ? true : false;
		if (!NeedRecreateScratch && ScratchResource->GetDesc().Width < PrebuildInfo.ScratchDataSizeInBytes)
		{
			NeedRecreateScratch = true;
		}
		if (NeedRecreateScratch)
		{
			auto UploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
			auto ScratchBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(PrebuildInfo.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
			VALIDATE_HRESULT(DXRDevice->CreateCommittedResource(
				&UploadHeapProperties,
				D3D12_HEAP_FLAG_NONE,
				&ScratchBufferDesc,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				nullptr,
				IID_PPV_ARGS(&ScratchResource)));
		}

		// Create Instance Resource
		const uint32 InstanceBufferSize = InstanceDescData->GetLength();
		TLASInstanceBuffer = ti_new FUniformBuffer(InstanceBufferSize, 1, (uint32)EGPUResourceFlag::Intermediate);
		TLASInstanceBuffer->CreateGPUBuffer(InstanceDescData);

		// Build top layer AS
		TopLevelInputs.InstanceDescs = RHIDx12->GetGPUBufferGPUAddress(TLASInstanceBuffer->GetGPUResource());
		TopLevelBuildDesc.ScratchAccelerationStructureData = ScratchResource->GetGPUVirtualAddress();
		TopLevelBuildDesc.DestAccelerationStructureData = AccelerationStructure.Get()->GetGPUVirtualAddress();

		// Be sure we are ready for and read access which may follow.  We only call this function in init so this
		// should not affect runtime performance.
		RHIDx12->FlushResourceStateChange();
		int32 BarrierCount = 0;
		for (const auto& BLAS : BLASes)
		{
			RHIDx12->UAVBarrier(BLAS.first);
			++BarrierCount;
			if (BarrierCount >= FRHIConfig::MaxResourceBarrierBuffers)
			{
				RHIDx12->FlushResourceStateChange();
				BarrierCount = 0;
			}
		}
		RHIDx12->FlushResourceStateChange();

		// https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html
		// Array of vertex indices.If NULL, triangles are non - indexed.Just as with graphics, 
		// the address must be aligned to the size of IndexFormat. The memory pointed to must 
		// be in state D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE.Note that if an app wants 
		// to share index buffer inputs between graphics input assemblerand raytracing 
		// acceleration structure build input, it can always put a resource into a combination 
		// of read states simultaneously, 
		// e.g.D3D12_RESOURCE_STATE_INDEX_BUFFER | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
		ID3D12DescriptorHeap* DescriptorHeap = RHIDx12->GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		DXRCommandList->SetDescriptorHeaps(1, &DescriptorHeap);
		DXRCommandList->BuildRaytracingAccelerationStructure(&TopLevelBuildDesc, 0, nullptr);
		Dirty = false;
	}
}

#endif	// COMPILE_WITH_RHI_DX12