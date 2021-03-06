/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	class TNodeLight;
	class TNodeStaticMesh : public TNode
	{
		DECLARE_NODE_WITH_CONSTRUCTOR(StaticMesh);

	public:
		virtual ~TNodeStaticMesh();

		virtual void UpdateAllTransformation() override;

		virtual void BindLights(TVector<TNode*>& Lights, bool ForceRebind) override;
		void LinkMeshAsset(
			TAssetPtr InMeshAsset, 
			TInstanceBufferPtr InInstanceBuffer, 
			uint32 InInstanceCount, 
			uint32 InInstanceOffset, 
			bool bCastShadow, 
			bool bReceiveShadow);
		void LinkStaticMesh(
			TStaticMeshPtr InStaticMesh, 
			TInstanceBufferPtr InInstanceBuffer,
			uint32 InInstanceCount,
			uint32 InInstanceOffset, 
			bool bCastShadow, 
			bool bReceiveShadow);

	protected:
		virtual void UpdateAbsoluteTransformation() override;

	protected:
		FPrimitivePtr LinkedPrimitive;
		FBox TransformedBBox;
		TVector<TNodeLight*> BindedLights;

		TAssetPtr MeshAsset;
		TInstanceBufferPtr MeshInstance;
		uint32 InstanceCount;
		uint32 InstanceOffset;
	};

} // end namespace tix

