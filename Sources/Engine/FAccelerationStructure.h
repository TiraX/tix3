/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	// AccelerationStructure
	// A base class for bottom-level and top-level AS.
	class FAccelerationStructure : public FRenderResource
	{
	public:
		FAccelerationStructure();
		virtual ~FAccelerationStructure();

		virtual void Build(FRHICmdList* RHICmdList) = 0;
		virtual bool AlreadyBuilt() = 0;

		void MarkDirty()
		{
			Dirty = true;
		}

		bool IsDirty() const
		{
			return Dirty;
		}
	protected:

	protected:
		bool Dirty;
	};

	/////////////////////////////////////////////////////////////

	class FBottomLevelAccelerationStructure : public FAccelerationStructure
	{
	public:
		FBottomLevelAccelerationStructure();
		virtual ~FBottomLevelAccelerationStructure();

		virtual void AddMeshBuffer(FVertexBufferPtr InVB, FIndexBufferPtr InIB) = 0;
	protected:
	};

	/////////////////////////////////////////////////////////////

	class FTopLevelAccelerationStructure : public FAccelerationStructure
	{
	public:
		FTopLevelAccelerationStructure();
		virtual ~FTopLevelAccelerationStructure();

		virtual void ClearAllInstances() = 0;
		virtual void ReserveInstanceCount(uint32 Count) = 0;
		virtual void AddBLASInstance(FBottomLevelAccelerationStructurePtr BLAS, const FMat34& Transform) = 0;

	protected:
		FUniformBufferPtr TLASInstanceBuffer;
	};

}