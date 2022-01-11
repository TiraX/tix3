/*
	TiX Engine v2.0 Copyright (C) 2018~2021
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	// TMeshBuffer, hold mesh vertex and index data memory in game thread
	class TI_API TCollisionSet : public TResource
	{
	public:
		struct TSphere
		{
			TSphere()
				: Radius(1.f)
			{}

			FFloat3 Center;
			float Radius;
		};
		struct TBox
		{
			FFloat3 Center;
			quaternion Rotation;
			FFloat3 Edge;
		};
		struct TCapsule
		{
			TCapsule()
				: Radius(1.f)
				, Length(1.f)
			{}

			FFloat3 Center;
			quaternion Rotation;
			float Radius;
			float Length;
		};
		struct TConvex
		{
			TVector<FFloat3> VertexData;
			TVector<uint16> IndexData;
		};

		TCollisionSet();
		~TCollisionSet();

		TMeshBufferPtr ConvertToMesh() const;

	public:
		virtual void InitRenderThreadResource() override;
		virtual void DestroyRenderThreadResource() override;

	protected:

	protected:
		TVector<TCollisionSet::TSphere> Spheres;
		TVector<TCollisionSet::TBox> Boxes;
		TVector<TCollisionSet::TCapsule> Capsules;
		TVector<TCollisionSet::TConvex> Convexes;

		friend class TAssetFile;
	};
}
