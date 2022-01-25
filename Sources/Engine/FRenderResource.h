/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	enum class ERenderResourceType
	{
		VertexBuffer,
		IndexBuffer,
		InstanceBuffer,
		UniformBuffer,
		Texture,
		Pipeline,
		Shader,
		ShaderBinding,	// RootSignature in dx12
		RenderTarget,
		ResourceTable,
		ArgumentBuffer,
		GpuCommandSignature,
		GpuCommandBuffer,
		EnvLight,
		RtxPipeline,
		AccelerationStructure,
	};

	template<typename T>
	inline IInstrusivePtr<T> ResourceCast(FRenderResourcePtr Resource)
	{
		IInstrusivePtr<T> Ptr = static_cast<T*>(Resource.get());
		return Ptr;
	}

	class TI_API FRenderResource : public IReferenceCounted
	{
	public:
		FRenderResource(ERenderResourceType InResourceType)
			: ResourceType(InResourceType)
		{}
		virtual ~FRenderResource() 
		{}

		virtual void CreateGPUBuffer(TStreamPtr Data) {}
		virtual void CreateGPUTexture(const TVector<TImagePtr>& Data = TVector<TImagePtr>()) {}
		virtual FGPUResourcePtr GetGPUResource()
		{
			return nullptr;
		}

		ERenderResourceType GetResourceType() const
		{
			return ResourceType;
		}

		virtual void SetResourceName(const TString& Name)
		{
#if defined (TIX_DEBUG)
			ResourceName = Name;
#endif
		}
		const TString& GetResourceName() const
		{
#if defined (TIX_DEBUG)
			return ResourceName;
#else
			static const TString DefaultResourceName = "Resource";
			return DefaultResourceName;
#endif
		}

	protected:
		ERenderResourceType ResourceType;

#if defined (TIX_DEBUG)
		TString ResourceName;
#endif
	};
}
