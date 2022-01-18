/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "ResHelper.h"
#include "TCookerMesh.h"
#include "TMeshBufferSemantic.h"
#include "TCookerMultiThreadTask.h"


inline int32 GetSegmentElements(E_VERTEX_STREAM_SEGMENT Segment)
{
	if (Segment == EVSSEG_POSITION ||
		Segment == EVSSEG_NORMAL ||
		Segment == EVSSEG_TANGENT)
		return 3;
	if (Segment == EVSSEG_TEXCOORD0 ||
		Segment == EVSSEG_TEXCOORD1)
		return 2;
	if (Segment == EVSSEG_COLOR ||
		Segment == EVSSEG_BLENDINDEX ||
		Segment == EVSSEG_BLENDWEIGHT)
		return 4;
	TI_ASSERT(0);
	return 0;
}

namespace tix
{
	void TResMeshDefine::AddSegment(E_MESH_STREAM_INDEX InStreamType, float* InData, int32 InStrideInByte)
	{
		TI_ASSERT(InStrideInByte % sizeof(float) == 0);
		Segments[InStreamType].Data = InData;
		Segments[InStreamType].StrideInFloat = InStrideInByte / sizeof(float);
	}

	void TResMeshDefine::SetFaces(int32* Indices, int32 Count)
	{
		Faces.Data = Indices;
		Faces.Count = Count;
	}

	/////////////////////////////////////////////////////////////////

	TCookerMesh::TCookerMesh()
	{
	}

	TCookerMesh::~TCookerMesh()
	{
	}

	bool TCookerMesh::Load(const TJSON& Doc)
	{
		TString Name;
		Doc["name"] << Name;
		//int32 Version = Doc["version"].GetInt();
		int32 TotalVertices, TotalIndices;
		Doc["vertex_count_total"] << TotalVertices;
		Doc["index_count_total"] << TotalIndices;

		// Get ref skeleton if this is a skeletal mesh
		Doc["skeleton"] << Mesh.RefSkeleton;

		// Load mesh data
		{
			TJSONNode JData = Doc["data"];
			TJSONNode JVsFormat = JData["vs_format"];

			TVector<TString> SFormat;
			JVsFormat << SFormat;

			TJSONNode JVertices = JData["vertices"];
			TJSONNode JIndices = JData["indices"];
			int32 VsFormat = 0;
			int32 ElementsStride = 0;
			for (const auto& S : SFormat)
			{
				E_VERTEX_STREAM_SEGMENT Segment = GetVertexSegment(S);
				VsFormat |= Segment;
				ElementsStride += GetSegmentElements(Segment);
			}
			TI_ASSERT(TotalVertices * ElementsStride == JVertices.Size());

			const int32 BytesStride = ElementsStride * sizeof(float);
			Mesh.NumVertices = TotalVertices;
			Mesh.NumTriangles = (int32)JIndices.Size() / 3;
			JVertices << Mesh.Vertices;
			JIndices << Mesh.Indices;

			int32 ElementOffset = 0;
			{
				TI_ASSERT((VsFormat & EVSSEG_POSITION) != 0);
				Mesh.AddSegment(ESSI_POSITION, (float*)&Mesh.Vertices[ElementOffset], BytesStride);
				ElementOffset += 3;
			}
			if ((VsFormat & EVSSEG_NORMAL) != 0)
			{
				Mesh.AddSegment(ESSI_NORMAL, (float*)&Mesh.Vertices[ElementOffset], BytesStride);
				ElementOffset += 3;
			}
			if ((VsFormat & EVSSEG_COLOR) != 0)
			{
				Mesh.AddSegment(ESSI_COLOR, (float*)&Mesh.Vertices[ElementOffset], BytesStride);
				ElementOffset += 4;
			}
			if ((VsFormat & EVSSEG_TEXCOORD0) != 0)
			{
				Mesh.AddSegment(ESSI_TEXCOORD0, (float*)&Mesh.Vertices[ElementOffset], BytesStride);
				ElementOffset += 2;
			}
			if ((VsFormat & EVSSEG_TEXCOORD1) != 0)
			{
				Mesh.AddSegment(ESSI_TEXCOORD1, (float*)&Mesh.Vertices[ElementOffset], BytesStride);
				ElementOffset += 2;
			}
			if ((VsFormat & EVSSEG_TANGENT) != 0)
			{
				Mesh.AddSegment(ESSI_TANGENT, (float*)&Mesh.Vertices[ElementOffset], BytesStride);
				ElementOffset += 3;
			}
			if ((VsFormat & EVSSEG_BLENDINDEX) != 0)
			{
				Mesh.AddSegment(ESSI_BLENDINDEX, (float*)&Mesh.Vertices[ElementOffset], BytesStride);
				ElementOffset += 4;
			}
			if ((VsFormat & EVSSEG_BLENDWEIGHT) != 0)
			{
				Mesh.AddSegment(ESSI_BLENDWEIGHT, (float*)&Mesh.Vertices[ElementOffset], BytesStride);
				ElementOffset += 4;
			}
			Mesh.SetFaces(&Mesh.Indices[0], (int32)Mesh.Indices.size());
		}

		// Load mesh sections
		{
			TJSONNode JSections = Doc["sections"];

			Mesh.Sections.clear();
			Mesh.Sections.resize(JSections.Size());

			for (int32 i = 0; i < JSections.Size(); ++i)
			{
				TJSONNode JSection = JSections[i];
				TResMeshSection& Section = Mesh.Sections[i];

				JSection["name"] << Section.Name;
				JSection["bone_map"] << Section.ActiveBones;
				JSection["material"] << Section.LinkedMaterialInstance;
				JSection["index_start"] << Section.IndexStart;
				JSection["triangles"] << Section.Triangles;
			}

			// Generate mesh cluster for this section
			if (TCookerSettings::Setting.MeshClusterSize > 0)
			{
				TI_ASSERT(0);
			}
		}

		// Load collisions
		{
			TJSONNode Collisions = Doc["collisions"];

			TJSONNode ColSpheres = Collisions["sphere"];
			TJSONNode ColBoxes = Collisions["box"];
			TJSONNode ColCapsules = Collisions["capsule"];
			TJSONNode ColConvex = Collisions["convex"];

			Mesh.ColSpheres.resize(ColSpheres.Size());
			Mesh.ColBoxes.resize(ColBoxes.Size());
			Mesh.ColCapsules.resize(ColCapsules.Size());
			Mesh.ColConvexes.resize(ColConvex.Size());

			// Spheres
			for (int32 i = 0 ; i < ColSpheres.Size(); ++ i)
			{
				TJSONNode JSphere = ColSpheres[i];
				JSphere["center"] << Mesh.ColSpheres[i].Center;
				JSphere["radius"] << Mesh.ColSpheres[i].Radius;
			}

			// Boxes
			for (int32 i = 0 ; i < ColBoxes.Size(); ++ i)
			{
				TJSONNode JBox = ColBoxes[i];

				JBox["center"] << Mesh.ColBoxes[i].Center;
				JBox["quat"] << Mesh.ColBoxes[i].Rotation;
				JBox["x"] << Mesh.ColBoxes[i].Edge.X;
				JBox["y"] << Mesh.ColBoxes[i].Edge.Y;
				JBox["z"] << Mesh.ColBoxes[i].Edge.Z;
			}

			// Capsules
			for (int32 i = 0 ; i < ColCapsules.Size() ; ++ i)
			{
				TJSONNode JCapsule = ColCapsules[i];

				JCapsule["center"] << Mesh.ColCapsules[i].Center;
				JCapsule["quat"] << Mesh.ColCapsules[i].Rotation;
				JCapsule["radius"] << Mesh.ColCapsules[i].Radius;
				JCapsule["length"] << Mesh.ColCapsules[i].Length;
			}

			// Convex
			for (int32 i = 0; i < ColConvex.Size(); ++i)
			{
				TJSONNode JConvex = ColConvex[i];
				TJSONNode JBBox = JConvex["bbox"];
				TJSONNode JCookedVB = JConvex["cooked_mesh_vertex_data"];
				TJSONNode JCookedIB = JConvex["cooked_mesh_index_data"];

				FFloat3 Translation;
				FQuat Rotation;
				FFloat3 Scale;
				JConvex["translation"] << Translation;
				JConvex["rotation"] << Rotation;
				JConvex["scale"] << Scale;

				FMat4 Mat;
				Rotation.GetMatrix(Mat);
				Mat.PostScale(Scale);
				Mat.SetTranslation(Translation);

				TI_ASSERT(JCookedVB.Size() % 3 == 0 && JCookedIB.Size() % 3 == 0);
				TVector<FFloat3> VertexData;
				VertexData.resize(JCookedVB.Size() / 3);
				for (int32 e = 0 ; e < JCookedVB.Size(); e += 3)
				{
					float X, Y, Z;
					JCookedVB[e + 0] << X;
					JCookedVB[e + 1] << Y;
					JCookedVB[e + 2] << Z;
					FFloat3 Position = FFloat3(X, Y, Z);
					Mat.TransformVect(Position);
					VertexData[e/3] = Position;
				}
				TI_ASSERT(JCookedIB.Size() < 65535);
				TVector<uint16> IndexData;
				IndexData.resize(JCookedIB.Size());
				for (int32 e = 0; e < JCookedIB.Size(); ++e)
				{
					int32 Index;
					JCookedIB[e] << Index;
					TI_ASSERT(Index < (int32)VertexData.size());
					IndexData[e] = Index;
				}

				Mesh.ColConvexes[i].VertexData = VertexData;
				Mesh.ColConvexes[i].IndexData = IndexData;
			}
		}

		return true;
	}

	void TCookerMesh::SaveTrunk(TChunkFile& OutChunkFile)
	{
		TStream& OutStream = OutChunkFile.GetChunk(GetCookerType());
		TVector<TString>& OutStrings = OutChunkFile.Strings;

		TResfileChunkHeader ChunkHeader;
		ChunkHeader.ID = TIRES_ID_CHUNK_MESH;
		ChunkHeader.Version = TIRES_VERSION_CHUNK_MESH;
		ChunkHeader.ElementCount = 1;

		TStream HeaderStream, DataStream(1024 * 8);
		// Mesh data
		for (int32 m = 0; m < ChunkHeader.ElementCount; ++m)
		{
			TI_ASSERT(Mesh.Segments[ESSI_POSITION].Data != nullptr);

			int32 IndexType = Mesh.NumVertices > 65535 ? EIT_32BIT : EIT_16BIT;
			if (TCookerSettings::Setting.Force32BitIndex)
			{
				IndexType = EIT_32BIT;
			}

			// init header
			THeaderMesh MeshHeader;
			MeshHeader.VertexFormat = 0;
			for (int32 s = 0; s < ESSI_TOTAL; ++s)
			{
				if (Mesh.Segments[s].Data != nullptr)
				{
					MeshHeader.VertexFormat |= (1 << s);
				}
			}
			MeshHeader.VertexCount = Mesh.NumVertices;
			MeshHeader.PrimitiveCount = Mesh.NumTriangles;
			MeshHeader.IndexType = IndexType;
			MeshHeader.Sections = (int32)Mesh.Sections.size();
			TI_ASSERT(MeshHeader.Sections > 0);
			MeshHeader.Flag = 0;
			FFloat3 FirstPosition(Mesh.Segments[ESSI_POSITION].Data[0], Mesh.Segments[ESSI_POSITION].Data[1], Mesh.Segments[ESSI_POSITION].Data[2]);
			MeshHeader.BBox.Reset(FirstPosition);
			if (Mesh.RefSkeleton != "")
			{
				MeshHeader.RefSkeletonStrIndex = AddStringToList(OutStrings, Mesh.RefSkeleton);
			}

			TVector<THeaderMeshSection> SMSections;
			SMSections.resize(MeshHeader.Sections);
			TVector<uint32> TotalActiveBones;
			for (int32 s = 0; s < MeshHeader.Sections; ++s)
			{
				SMSections[s].StrId_Name = AddStringToList(OutStrings, Mesh.Sections[s].Name);
				SMSections[s].StrMaterialInstance = AddStringToList(OutStrings, Mesh.Sections[s].LinkedMaterialInstance);
				SMSections[s].IndexStart = Mesh.Sections[s].IndexStart;
				SMSections[s].Triangles = Mesh.Sections[s].Triangles;
				SMSections[s].ActiveBones = (uint32)Mesh.Sections[s].ActiveBones.size();
				for (uint32 b = 0; b < SMSections[s].ActiveBones; b++)
				{
					TotalActiveBones.push_back(Mesh.Sections[s].ActiveBones[b]);
				}
			}

			if (TCookerSettings::Setting.MeshClusterSize > 0)
			{
				TI_ASSERT(0);
			}

			// fill data
			// - vertices
			float* DataPos = Mesh.Segments[ESSI_POSITION].Data != nullptr ? Mesh.Segments[ESSI_POSITION].Data : nullptr;
			float* DataNormal = Mesh.Segments[ESSI_NORMAL].Data != nullptr ? Mesh.Segments[ESSI_NORMAL].Data : nullptr;
			float* DataColor = Mesh.Segments[ESSI_COLOR].Data != nullptr ? Mesh.Segments[ESSI_COLOR].Data : nullptr;
			float* DataUv0 = Mesh.Segments[ESSI_TEXCOORD0].Data != nullptr ? Mesh.Segments[ESSI_TEXCOORD0].Data : nullptr;
			float* DataUv1 = Mesh.Segments[ESSI_TEXCOORD1].Data != nullptr ? Mesh.Segments[ESSI_TEXCOORD1].Data : nullptr;
			float* DataTangent = Mesh.Segments[ESSI_TANGENT].Data != nullptr ? Mesh.Segments[ESSI_TANGENT].Data : nullptr;
			float* DataBI = Mesh.Segments[ESSI_BLENDINDEX].Data != nullptr ? Mesh.Segments[ESSI_BLENDINDEX].Data : nullptr;
			float* DataBW = Mesh.Segments[ESSI_BLENDWEIGHT].Data != nullptr ? Mesh.Segments[ESSI_BLENDWEIGHT].Data : nullptr;

			TI_ASSERT(DataPos != nullptr);
			const int32 VertexStart = DataStream.GetLength();
			for (int32 v = 0; v < Mesh.NumVertices; ++v)
			{
				if (DataPos != nullptr)
				{
					FFloat3 pos(DataPos[0], DataPos[1], DataPos[2]);
					MeshHeader.BBox.AddInternalPoint(pos);

					TI_ASSERT(sizeof(FFloat3) == TMeshBuffer::SemanticSize[ESSI_POSITION]);
					DataStream.Put(DataPos, sizeof(FFloat3));
					DataPos += Mesh.Segments[ESSI_POSITION].StrideInFloat;
				}
				if (DataNormal != nullptr)
				{
					uint8 NData[4];
					NData[0] = FloatToUNorm(DataNormal[0]);
					NData[1] = FloatToUNorm(DataNormal[1]);
					NData[2] = FloatToUNorm(DataNormal[2]);
					NData[3] = 255;

					TI_ASSERT(sizeof(NData) == TMeshBuffer::SemanticSize[ESSI_NORMAL]);
					DataStream.Put(NData, sizeof(NData));
					DataNormal += Mesh.Segments[ESSI_NORMAL].StrideInFloat;
				}
				if (DataColor != nullptr)
				{
					uint8 CData[4];
					CData[0] = FloatToColor(DataColor[0]);
					CData[1] = FloatToColor(DataColor[1]);
					CData[2] = FloatToColor(DataColor[2]);
					CData[3] = FloatToColor(DataColor[3]);

					TI_ASSERT(sizeof(CData) == TMeshBuffer::SemanticSize[ESSI_COLOR]);
					DataStream.Put(CData, sizeof(CData));
					DataColor += Mesh.Segments[ESSI_COLOR].StrideInFloat;
				}
				if (DataUv0 != nullptr)
				{
					TI_ASSERT(sizeof(float16) * 2 == TMeshBuffer::SemanticSize[ESSI_TEXCOORD0]);
					float16 UvHalf[2];
					UvHalf[0] = DataUv0[0];
					UvHalf[1] = DataUv0[1];
					DataStream.Put(UvHalf, sizeof(float16) * 2);
					DataUv0 += Mesh.Segments[ESSI_TEXCOORD0].StrideInFloat;
				}
				if (DataUv1 != nullptr)
				{
					TI_ASSERT(sizeof(float16) * 2 == TMeshBuffer::SemanticSize[ESSI_TEXCOORD1]);
					float16 UvHalf[2];
					UvHalf[0] = DataUv1[0];
					UvHalf[1] = DataUv1[1];
					DataStream.Put(UvHalf, sizeof(float16) * 2);
					DataUv1 += Mesh.Segments[ESSI_TEXCOORD1].StrideInFloat;
				}
				if (DataTangent != nullptr)
				{
					uint8 TData[4];
					TData[0] = FloatToUNorm(DataTangent[0]);
					TData[1] = FloatToUNorm(DataTangent[1]);
					TData[2] = FloatToUNorm(DataTangent[2]);
					TData[3] = 255;

					TI_ASSERT(sizeof(TData) == TMeshBuffer::SemanticSize[ESSI_TANGENT]);
					DataStream.Put(TData, sizeof(TData));
					DataTangent += Mesh.Segments[ESSI_TANGENT].StrideInFloat;
				}
				if (DataBI != nullptr)
				{
					TI_ASSERT(sizeof(uint8) * 4 == TMeshBuffer::SemanticSize[ESSI_BLENDINDEX]);
					uint8 BIData[4];
					BIData[0] = (uint8)DataBI[0];
					BIData[1] = (uint8)DataBI[1];
					BIData[2] = (uint8)DataBI[2];
					BIData[3] = (uint8)DataBI[3];
					DataStream.Put(BIData, sizeof(uint8) * 4);
					DataBI += Mesh.Segments[ESSI_BLENDINDEX].StrideInFloat;
				}
				if (DataBW != nullptr)
				{
					uint8 BWData[4];
					BWData[0] = FloatToColor(DataBW[0]);
					BWData[1] = FloatToColor(DataBW[1]);
					BWData[2] = FloatToColor(DataBW[2]);
					BWData[3] = FloatToColor(DataBW[3]);

					TI_ASSERT((int32)sizeof(BWData) == TMeshBuffer::SemanticSize[ESSI_BLENDWEIGHT]);
					DataStream.Put(BWData, sizeof(BWData));
					DataBW += Mesh.Segments[ESSI_BLENDWEIGHT].StrideInFloat;
				}
			}
			const int32 VertexEnd = DataStream.GetLength();

			// 8 bytes align
			TI_ASSERT((VertexEnd - VertexStart) % 4 == 0);
			if ((VertexEnd - VertexStart) % 4 != 0)
			{
				_LOG(ELogLevel::Error, "Not aligned vertices.\n");
			}
			FillZero4(DataStream);

			// - Indices
			if (TCookerSettings::Setting.MeshClusterSize > 0)
			{
				TI_ASSERT(0);
			}
			else
			{
				TI_ASSERT(Mesh.Faces.Count == MeshHeader.PrimitiveCount * 3);
				if (MeshHeader.IndexType == EIT_16BIT)
				{
					for (int32 i = 0; i < Mesh.Faces.Count; ++i)
					{
						uint16 Index = (uint16)Mesh.Faces.Data[i];
						DataStream.Put(&Index, sizeof(uint16));
					}
				}
				else
				{
					for (int32 i = 0; i < Mesh.Faces.Count; ++i)
					{
						uint32 Index = (uint32)Mesh.Faces.Data[i];
						DataStream.Put(&Index, sizeof(uint32));
					}
				}
			}
			FillZero4(DataStream);

			// Export cluster meta data
			if (TCookerSettings::Setting.MeshClusterSize > 0)
			{
				TI_ASSERT(0);
			}

			// Fill header
			HeaderStream.Put(&MeshHeader, sizeof(THeaderMesh));
			FillZero4(HeaderStream);
			HeaderStream.Put(SMSections.data(), (uint32)(sizeof(THeaderMeshSection) * SMSections.size()));
			if (TotalActiveBones.size() > 0)
			{
				HeaderStream.Put(TotalActiveBones.data(), (uint32)TotalActiveBones.size() * sizeof(uint32));
			}
			FillZero4(HeaderStream);
		}

		// Collision data
		{
			// Header
			THeaderCollisionSet HeaderCollision;
			HeaderCollision.NumSpheres = (uint32)Mesh.ColSpheres.size();
			HeaderCollision.NumBoxes = (uint32)Mesh.ColBoxes.size();
			HeaderCollision.NumCapsules = (uint32)Mesh.ColCapsules.size();
			HeaderCollision.NumConvexes = (uint32)Mesh.ColConvexes.size();
			HeaderCollision.SpheresSizeInBytes = sizeof(TCollisionSet::TSphere) * (uint32)Mesh.ColSpheres.size();
			HeaderCollision.BoxesSizeInBytes = sizeof(TCollisionSet::TBox) * (uint32)Mesh.ColBoxes.size();
			HeaderCollision.CapsulesSizeInBytes = sizeof(TCollisionSet::TCapsule) * (uint32)Mesh.ColCapsules.size();
			HeaderCollision.ConvexesSizeInBytes = sizeof(FUInt2) * (uint32)Mesh.ColConvexes.size();
			HeaderStream.Put(&HeaderCollision, sizeof(THeaderCollisionSet));

			// Data
			DataStream.Put(Mesh.ColSpheres.data(), HeaderCollision.SpheresSizeInBytes);
			DataStream.Put(Mesh.ColBoxes.data(), HeaderCollision.BoxesSizeInBytes);
			DataStream.Put(Mesh.ColCapsules.data(), HeaderCollision.CapsulesSizeInBytes);
			TVector<FUInt2> ConvexVertexIndexCount;
			ConvexVertexIndexCount.reserve(Mesh.ColConvexes.size());
			for (const auto& Convex : Mesh.ColConvexes)
			{
				FUInt2 VertexIndexCount;
				VertexIndexCount.X = (uint32)Convex.VertexData.size();
				VertexIndexCount.Y = (uint32)Convex.IndexData.size();
				ConvexVertexIndexCount.push_back(VertexIndexCount);
			}
			DataStream.Put(ConvexVertexIndexCount.data(), HeaderCollision.ConvexesSizeInBytes);

			// Convex vertex data and index data
			for (const auto& Convex : Mesh.ColConvexes)
			{
				DataStream.Put(Convex.VertexData.data(), sizeof(FFloat3) * (uint32)Convex.VertexData.size());
				DataStream.Put(Convex.IndexData.data(), sizeof(uint16) * (uint32)Convex.IndexData.size());
				FillZero4(DataStream);
			}
		}

		ChunkHeader.ChunkSize = HeaderStream.GetLength() + DataStream.GetLength();

		OutStream.Put(&ChunkHeader, sizeof(TResfileChunkHeader));
		FillZero4(OutStream);
		OutStream.Put(HeaderStream.GetBuffer(), HeaderStream.GetLength());
		OutStream.Put(DataStream.GetBuffer(), DataStream.GetLength());
	}
}
