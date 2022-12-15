/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "NaniteMesh.h"
#include "Cluster.h"
#include "NaniteEncode.h"

TNaniteMesh::TNaniteMesh()
{
}

//static const TString MeshFilename = "SM_NaniteMesh";
//const int32 NumLODs = 5;
//static const TString MeshFilename = "SM_NaniteTess";
//const int32 NumLODs = 3;
static const TString MeshFilename = "SM_NaniteTessGrid";
const int32 NumLODs = 2;

struct FNaniteAssetHeader
{
	uint32 RootDataSize;
	uint32 StreamablePagesSize;
	uint32 NumBvHNodes;
	uint32 NumPages;
	uint32 NumPageDependencies;

	uint32 NumRootPages;
	uint32 NumRootPageClusters;
	uint32 PositionPrecision;

	uint32 GetDataSize() const
	{
		return
			RootDataSize +
			StreamablePagesSize +
			NumBvHNodes * sizeof(FPackedHierarchyNode) +
			NumPages * sizeof(FPageStreamingState) +
			NumPageDependencies * sizeof(uint32);
	}
};

TNaniteMesh* TNaniteMesh::LoadMesh()
{
	TNaniteMesh* Mesh = ti_new TNaniteMesh;
	const TString DestFilename = MeshFilename + ".tasset";
	TFile MeshFile;
	if (!MeshFile.Open(DestFilename, EFA_READ))
	{
		bool ConvertResult = ConvertNanieMesh(*Mesh);
		TI_ASSERT(ConvertResult);
	}
	else
	{
		FNaniteAssetHeader Header;
		MeshFile.Read(&Header, sizeof(FNaniteAssetHeader), sizeof(FNaniteAssetHeader));

		Mesh->NumRootPages = Header.NumRootPages;
		Mesh->NumRootPageClusters = Header.NumRootPageClusters;
		Mesh->PositionPrecision = Header.PositionPrecision;

		int32 Aligned = 0;
		Mesh->RootData.resize(Header.RootDataSize);
		MeshFile.Read(Mesh->RootData.data(), Header.RootDataSize, Header.RootDataSize);
		Aligned = TMath::Align4(Header.RootDataSize);
		MeshFile.Seek(Header.RootDataSize - Aligned, true);

		Mesh->StreamablePages.resize(Header.StreamablePagesSize);
		MeshFile.Read(Mesh->StreamablePages.data(), Header.StreamablePagesSize, Header.StreamablePagesSize);
		Aligned = TMath::Align4(Header.StreamablePagesSize);
		MeshFile.Seek(Header.StreamablePagesSize - Aligned, true);

		TI_ASSERT(Header.NumBvHNodes <= TNaniteMesh::MaxHierarchyNodes);
		Mesh->HierarchyNodes.resize(Header.NumBvHNodes);
		MeshFile.Read(Mesh->HierarchyNodes.data(), Header.NumBvHNodes * sizeof(FPackedHierarchyNode), Header.NumBvHNodes * sizeof(FPackedHierarchyNode));

		Mesh->PageStreamingStates.resize(Header.NumPages);
		MeshFile.Read(Mesh->PageStreamingStates.data(), Header.NumPages * sizeof(FPageStreamingState), Header.NumPages * sizeof(FPageStreamingState));

		Mesh->PageDependencies.resize(Header.NumPageDependencies);
		MeshFile.Read(Mesh->PageDependencies.data(), Header.NumPageDependencies * sizeof(uint32), Header.NumPageDependencies * sizeof(uint32));
	}

	const int32 NumPages = (int32)Mesh->PageStreamingStates.size();
	int32 NumClusters = 0;
	for (int32 i = 0; i < NumPages; i++)
	{
		const bool bIsRoot = Mesh->IsRootPage(i);
		const auto& State = Mesh->PageStreamingStates[i];
		const FFixupChunk* Fixup = (const FFixupChunk*)((bIsRoot ? Mesh->RootData.data() : Mesh->StreamablePages.data()) + State.BulkOffset);
		NumClusters += Fixup->Header.NumClusters;
	}

	return Mesh;
}

struct NaniteRawMeshData	// from json
{
	bool IsInstanced;
	int32 NumVtx;
	int32 NumTriangles;
	int32 NumClusters;
	int32 NumGroups;

	TVector<RawVertex> Vtx;
	TVector<int32> Idx;

	TVector<int32> ClusterIdPerTriangle;
	TVector<int32> GroupIdPerTriangle;
	TVector<int32> ParentIdPerTriangle;

	TVector<int32> MaterialIndexes ;	// always be 0 for now

	FInt2 ClusterInstanceRange;
};


void LoadDataFromJson(
	TVector<NaniteRawMeshData>& RawDatas, 
	TVector<float>& LODErrors, 
	TVector<FClusterInstance>& ClusterInstances,
	TVector<int32>& ClusterInsGroups, 
	TVector<int32>& ClusterInsParents)
{
	RawDatas.resize(NumLODs);

	ClusterInstances.reserve(1024);
	ClusterInsGroups.reserve(1024);
	ClusterInsParents.reserve(1024);

	int32 TotalClusters = 0;
	int32 TotalGroups = 0;

	TMap<uint32, uint32> ClusterToInstanceMap;	// cluster<==>intance pair

	for (int32 lod = 0; lod < NumLODs; lod++)
	{
		int8 NameBuffer[128];
		sprintf(NameBuffer, "%sLOD%d.json", MeshFilename.c_str(), lod);
		TString JsonFilename = NameBuffer;
		TFile JsonFile;
		if (!JsonFile.Open(JsonFilename, EFA_READ))
		{
			RuntimeFail();
		}
		// Load json file
		int8* FileContent = ti_new int8[JsonFile.GetSize() + 1];
		JsonFile.Read(FileContent, JsonFile.GetSize(), JsonFile.GetSize());
		FileContent[JsonFile.GetSize()] = 0;
		JsonFile.Close();

		// Parse json file
		TJSON JsonDoc;
		JsonDoc.Parse(FileContent);

		int32 NumTriangles, NumMeshes, NumClusters, NumGroups;
		JsonDoc["num_triangles"] << NumTriangles;
		JsonDoc["num_meshes"] << NumMeshes;
		JsonDoc["num_clusters"] << NumClusters;
		JsonDoc["num_groups"] << NumGroups;
		bool HasInstance;
		JsonDoc["has_instances"] << HasInstance;
		bool HasUV = false;
		JsonDoc["has_uv"] << HasUV;
		TVector<int32> NumPerInstance;
		JsonDoc["num_per_ins"] << NumPerInstance;

		TVector<float> VtxBuffer;
		TVector<int32> IdxBuffer;
		JsonDoc["vtx_buffer"] << VtxBuffer;
		JsonDoc["idx_buffer"] << IdxBuffer;

		const int32 NumClusterInstanceBegin = (int32)ClusterInstances.size();

		TVector<int32> ClusterInfo, GroupInfo, ParentInfo;
		if (HasInstance)
		{
			JsonDoc["cluster_per_tri"] << ClusterInfo;

			TVector<int32> InstanceIdInfo;
			JsonDoc["instance_id_per_tri"] << InstanceIdInfo;

			TVector<TSet<int32>> ClustersInInstance;
			ClustersInInstance.resize(NumPerInstance.size());
			for (int32 t = 0; t < NumTriangles; t++)
			{
				int32 InstanceId = InstanceIdInfo[t];
				int32 ClusterId = ClusterInfo[t];
				ClustersInInstance[InstanceId].insert(ClusterId);
			}

			TJSONNode JInstances = JsonDoc["instances"];
			for (int32 i = 0; i < JInstances.Size(); i++)
			{
				TJSONNode JIns = JInstances[i];
				int32 Id, Group, Parent;
				FFloat3 Pos, Dir;
				float PScale;
				JIns["id"] << Id;
				JIns["pos"] << Pos;
				JIns["dir"] << Dir;
				JIns["pscale"] << PScale;
				JIns["group"] << Group;
				JIns["parent"] << Parent;

				// Build transform matrix
				FMat4 Transform;
				FQuat Q;
				Q.RotationFromToUE(FFloat3(0, 1, 0), Dir);
				Q.GetMatrix(Transform);
				Transform.SetTranslation(Pos);
				Transform.PostScale(FFloat3(PScale, PScale, PScale));

				// Add instances
				for (const auto& CId : ClustersInInstance[Id])
				{
					ClusterInstances.push_back(FClusterInstance(CId, Transform));
					ClusterInsGroups.push_back(Group);
					ClusterInsParents.push_back(Parent);
				}
			}
		}
		else
		{
			JsonDoc["cluster"] << ClusterInfo;
			JsonDoc["group"] << GroupInfo;
			JsonDoc["parent"] << ParentInfo;

			TI_ASSERT(NumTriangles == ClusterInfo.size() &&
				NumTriangles == GroupInfo.size() && 
				NumTriangles == ParentInfo.size());

			TSet<int32> UniqueClusterIds;
			for (int32 t = 0; t < NumTriangles; t++)
			{
				int32 ClusterId = ClusterInfo[t];
				if (UniqueClusterIds.count(ClusterId) == 0)
				{
					// add this as a cluster instance
					UniqueClusterIds.insert(ClusterId);
					ClusterToInstanceMap[ClusterId] = (uint32)ClusterInstances.size();
					TI_ASSERT(ClusterInsGroups.size() == ClusterInstances.size());
					TI_ASSERT(ClusterInsParents.size() == ClusterInstances.size());
					ClusterInstances.push_back(FClusterInstance(ClusterId));
					ClusterInsGroups.push_back(GroupInfo[t]);
					ClusterInsParents.push_back(ParentInfo[t]);
				}
			}
		}

		const int32 StrideInFloat = HasUV ? 8 : 6;

		const int32 NumVtx = (int32)VtxBuffer.size() / StrideInFloat;
		const int32 NumIdx = (int32)IdxBuffer.size();
		TI_ASSERT(NumIdx / 3 == NumTriangles);

		// Set data to raw data
		NaniteRawMeshData& RawData = RawDatas[lod];
		RawData.IsInstanced = HasInstance;
		RawData.NumVtx = NumVtx;
		RawData.NumTriangles = NumTriangles;
		RawData.NumClusters = NumClusters - TotalClusters;
		RawData.NumGroups = NumGroups - TotalGroups;
		RawData.ClusterInstanceRange.X = NumClusterInstanceBegin;
		RawData.ClusterInstanceRange.Y = (int32)ClusterInstances.size() - 1;

		TotalClusters += RawData.NumClusters;
		TotalGroups += RawData.NumGroups;

		RawData.Vtx.resize(NumVtx);
		for (int32 v = 0; v < NumVtx; v++)
		{
			RawVertex& V = RawData.Vtx[v];
			V.Pos.X = VtxBuffer[v * StrideInFloat + 0];
			V.Pos.Y = VtxBuffer[v * StrideInFloat + 1];
			V.Pos.Z = VtxBuffer[v * StrideInFloat + 2];
			V.Nor.X = VtxBuffer[v * StrideInFloat + 3];
			V.Nor.Y = VtxBuffer[v * StrideInFloat + 4];
			V.Nor.Z = VtxBuffer[v * StrideInFloat + 5];
			if (HasUV)
			{
				V.UV.X = VtxBuffer[v * StrideInFloat + 6];
				V.UV.Y = VtxBuffer[v * StrideInFloat + 7];
			}
		}
		RawData.Idx.swap(IdxBuffer);

		// Materials always be 0 for now
		RawData.MaterialIndexes.resize(NumTriangles);

		RawData.ClusterIdPerTriangle.swap(ClusterInfo);
		RawData.GroupIdPerTriangle.swap(GroupInfo);
		RawData.ParentIdPerTriangle.swap(ParentInfo);

		ti_delete[] FileContent;
	}

	// Translate ClusterInsParents
	for (auto& Parent : ClusterInsParents)
	{
		if (Parent >= 0)
		{
			TI_ASSERT(ClusterToInstanceMap.find(Parent) != ClusterToInstanceMap.end());
			Parent = ClusterToInstanceMap[Parent];
		}
	}

	// LOD Errors
	TFile JsonFile;
	if (!JsonFile.Open(MeshFilename + "Error.json", EFA_READ))
	{
		RuntimeFail();
	}
	// Load json file
	int8* FileContent = ti_new int8[JsonFile.GetSize() + 1];
	JsonFile.Read(FileContent, JsonFile.GetSize(), JsonFile.GetSize());
	FileContent[JsonFile.GetSize()] = 0;
	JsonFile.Close();

	TJSON JsonDoc;
	JsonDoc.Parse(FileContent);
	JsonDoc["lod_errors"] << LODErrors;	
	ti_delete[] FileContent;
}

void ClusterTrianglesLOD0(
	const NaniteRawMeshData& RawMeshLOD0,
	TVector<FClusterInstance>& ClusterInstances,
	TVector<FCluster>& ClusterSources
)
{
	ClusterSources.resize(RawMeshLOD0.NumClusters);

	OMP_PARALLEL_FOR
	for (int32 c = 0; c < RawMeshLOD0.NumClusters; c++)
	{
		TVector<int32> ClusterIndexes;
		ClusterIndexes.reserve(NANITE_MAX_CLUSTER_TRIANGLES * 3);
		TVector<int32> ClusterMaterialIndexes;
		ClusterMaterialIndexes.reserve(NANITE_MAX_CLUSTER_TRIANGLES);

		const int32 TargetClusterId = c;
		for (int32 t = 0; t < RawMeshLOD0.NumTriangles; t++)
		{
			if (RawMeshLOD0.ClusterIdPerTriangle[t] == TargetClusterId)
			{
				ClusterIndexes.push_back(RawMeshLOD0.Idx[t * 3 + 0]);
				ClusterIndexes.push_back(RawMeshLOD0.Idx[t * 3 + 1]);
				ClusterIndexes.push_back(RawMeshLOD0.Idx[t * 3 + 2]);
				ClusterMaterialIndexes.push_back(RawMeshLOD0.MaterialIndexes[c]);
			}
		}

		ClusterSources[c].BuildCluster(RawMeshLOD0.Vtx, ClusterIndexes, ClusterMaterialIndexes, 1);
		ClusterSources[c].GenerateGUID(TargetClusterId);

		// Negative notes it's a leaf
		ClusterSources[c].EdgeLength *= -1.0f;
	}

	// Update cluster instance LODBounds and SphereBounds
	const int32 NumLOD0Instances = RawMeshLOD0.ClusterInstanceRange.Y - RawMeshLOD0.ClusterInstanceRange.X + 1;
	for (int32 i = 0; i < NumLOD0Instances; i++)
	{
		FClusterInstance& CI = ClusterInstances[i];
		FCluster& Cluster = ClusterSources[CI.ClusterId];

		if (CI.IsInstanced)
		{
			const FMat4& Trans = CI.Transform;
			FFloat3 Scale = Trans.GetScale();
			float MaxScale = TMath::Max3(Scale.X, Scale.Y, Scale.Z);

			CI.Transform.TransformVect(CI.LODBounds.Center, Cluster.LODBounds.Center);
			CI.LODBounds.W = MaxScale * Cluster.LODBounds.W;
			CI.Transform.TransformVect(CI.SphereBounds.Center, Cluster.SphereBounds.Center);
			CI.SphereBounds.W = MaxScale * Cluster.SphereBounds.W;
			CI.EdgeLength = MaxScale * Cluster.EdgeLength;
		}
		else
		{
			CI.LODBounds = Cluster.LODBounds;
			CI.SphereBounds = Cluster.SphereBounds;
			CI.EdgeLength = Cluster.EdgeLength;
		}
	}
}

void BuildDAGFromLODs(
	const TVector<NaniteRawMeshData>& RawDatas,
	const TVector<int32>& ClusterInsGroups,
	const TVector<int32>& ClusterInsParents,
	const TVector<float>& FakeLODErrors,
	TVector<FClusterInstance>& ClusterInstances,
	TVector<FCluster>& ClusterSources,
	TVector<FClusterGroup>& ClusterGroups
)
{
	const uint32 MeshIndex = 0;
	const int32 NumLODs = (int32)RawDatas.size();

	for (int32 lod = 1; lod < NumLODs; lod++)
	{
		const NaniteRawMeshData& RawData = RawDatas[lod];
		const int32 BaseCluster = (int32)ClusterSources.size();
		const int32 NumLODClusters = RawData.NumClusters;
		ClusterSources.resize(ClusterSources.size() + NumLODClusters);

		// Build LOD n Clusters
		const int32 LODClusterIdStart = BaseCluster;

		const TVector<RawVertex>& Verts = RawData.Vtx;
		const TVector<int32>& Indexes = RawData.Idx;
		const TVector<int32>& ClusterIds = RawData.ClusterIdPerTriangle;
		const TVector<int32>& MaterialIndexes = RawData.MaterialIndexes;

		OMP_PARALLEL_FOR
		for (int32 c = 0; c < NumLODClusters; c++)
		{
			TVector<int32> ClusterIndexes;
			ClusterIndexes.reserve(NANITE_MAX_CLUSTER_TRIANGLES * 3);
			TVector<int32> ClusterMaterialIndexes;
			ClusterMaterialIndexes.reserve(NANITE_MAX_CLUSTER_TRIANGLES);

			int32 TargetClusterId = LODClusterIdStart + c;
			for (int32 i = 0; i < (int32)Indexes.size(); i += 3)
			{
				int32 c = i / 3;
				if (ClusterIds[c] == TargetClusterId)
				{
					ClusterIndexes.push_back(Indexes[i + 0]);
					ClusterIndexes.push_back(Indexes[i + 1]);
					ClusterIndexes.push_back(Indexes[i + 2]);
					ClusterMaterialIndexes.push_back(MaterialIndexes[c]);
				}
			}

			ClusterSources[BaseCluster + c].BuildCluster(Verts, ClusterIndexes, ClusterMaterialIndexes, 1);
			ClusterSources[BaseCluster + c].GenerateGUID(TargetClusterId);
			ClusterSources[BaseCluster + c].MipLevel = lod;
		}

		// Update cluster instance members
		for (int32 i = RawData.ClusterInstanceRange.X; i <= RawData.ClusterInstanceRange.Y; i++)
		{
			FClusterInstance& CI = ClusterInstances[i];
			FCluster& Cluster = ClusterSources[CI.ClusterId];

			if (CI.IsInstanced)
			{
				const FMat4& Trans = CI.Transform;
				FFloat3 Scale = Trans.GetScale();
				float MaxScale = TMath::Max3(Scale.X, Scale.Y, Scale.Z);

				CI.Transform.TransformVect(CI.LODBounds.Center, Cluster.LODBounds.Center);
				CI.LODBounds.W = MaxScale * Cluster.LODBounds.W;
				CI.Transform.TransformVect(CI.SphereBounds.Center, Cluster.SphereBounds.Center);
				CI.SphereBounds.W = MaxScale * Cluster.SphereBounds.W;
				CI.EdgeLength = MaxScale * Cluster.EdgeLength;
			}
			else
			{
				CI.LODBounds = Cluster.LODBounds;
				CI.SphereBounds = Cluster.SphereBounds;
				CI.EdgeLength = Cluster.EdgeLength;
			}
		}

		// Build LOD n-1 Cluster Groups
		const NaniteRawMeshData& LastRawData = RawDatas[lod - 1];

		const int32 NumGroups = LastRawData.NumGroups;
		const int32 BaseGroup = (int32)ClusterGroups.size();
		ClusterGroups.resize(ClusterGroups.size() + NumGroups);

		OMP_PARALLEL_FOR
		for (int32 g = 0; g < NumGroups; g++)
		{
			TVector<uint32> ClusterInstancesInGroup;
			ClusterInstancesInGroup.reserve(NANITE_MAX_CLUSTERS_PER_GROUP_TARGET);

			const int32 TargetGroupId = BaseGroup + g;
			for (int32 ci = LastRawData.ClusterInstanceRange.X; ci <= LastRawData.ClusterInstanceRange.Y; ci++)
			{
				uint32 CurrGroup = ClusterInsGroups[ci];
				if (CurrGroup == TargetGroupId)
				{
					ClusterInstancesInGroup.push_back((uint32)ci);
				}
			}
			int32 GroupIndex = TargetGroupId;

			TVector<FSpheref> Children_LODBounds;
			TVector<FSpheref> Children_SphereBounds;
			Children_LODBounds.reserve(32);
			Children_SphereBounds.reserve(32);

			// Give a fake parent LOD error here.
			// TODO: give some error jitter ?
			float ParentMaxLODError = FakeLODErrors[lod];

			// Force monotonic nesting.
			float ChildMinLODError = TNumLimit<float>::max();
			for (int32 ClusterInstanceIndex : ClusterInstancesInGroup)
			{
				FClusterInstance& CI = ClusterInstances[ClusterInstanceIndex];
				int32 ClusterIndex = CI.ClusterId;
				bool bLeaf = ClusterSources[ClusterIndex].EdgeLength < 0.0f;
				float LODError = CI.LODError;

				// Bounds on cluster instance already transformed by instance_transform
				Children_LODBounds.push_back(CI.LODBounds);
				Children_SphereBounds.push_back(CI.SphereBounds);
				ChildMinLODError = TMath::Min(ChildMinLODError, bLeaf ? -1.0f : LODError);
				ParentMaxLODError = TMath::Max(ParentMaxLODError, LODError);

				CI.GroupIndex = GroupIndex;
			}

			FSpheref ParentLODBounds(Children_LODBounds.data(), (int32)Children_LODBounds.size());
			FSpheref ParentBounds(Children_SphereBounds.data(), (int32)Children_SphereBounds.size());

			// Set cluster's parent's LODError, LODBounds, G eneratingGroup Index
			// In this case, all clusters in this group have the same ONE cluster parent
			int32 ParentClusterInstance = -1;
			for (int32 ClusterInstanceIndex : ClusterInstancesInGroup)
			{
				if (ParentClusterInstance == -1)
				{
					ParentClusterInstance = ClusterInsParents[ClusterInstanceIndex];
				}
				// Make sure all clusters in this group have the SAME parent
				TI_ASSERT(ParentClusterInstance == ClusterInsParents[ClusterInstanceIndex]);
			}
			// Set parent cluster instance's info
			ClusterInstances[ParentClusterInstance].LODBounds = ParentLODBounds;
			ClusterInstances[ParentClusterInstance].LODError = ParentMaxLODError;
			ClusterInstances[ParentClusterInstance].GeneratingGroupIndex = GroupIndex;

			// Set cluster group values
			ClusterGroups[GroupIndex].Bounds = ParentBounds;
			ClusterGroups[GroupIndex].LODBounds = ParentLODBounds;
			ClusterGroups[GroupIndex].MinLODError = ChildMinLODError;
			ClusterGroups[GroupIndex].MaxParentLODError = ParentMaxLODError;
			ClusterGroups[GroupIndex].MipLevel = lod - 1;
			ClusterGroups[GroupIndex].MeshIndex = MeshIndex;
			ClusterGroups[GroupIndex].bTrimmed = false;
			ClusterGroups[GroupIndex].Children = ClusterInstancesInGroup;
			TI_ASSERT(ClusterInstancesInGroup.size() <= NANITE_MAX_CLUSTERS_PER_GROUP_TARGET);
		}
	}

	// Max out root node
	uint32 RootIndex = (uint32)ClusterInstances.size() - 1;
	FClusterGroup RootClusterGroup;
	RootClusterGroup.Children.push_back(RootIndex);
	FCluster& RootCluster = ClusterSources[ClusterInstances[RootIndex].ClusterId];
	FClusterInstance& RootCI = ClusterInstances[RootIndex];
	RootClusterGroup.Bounds = RootCluster.SphereBounds;
	RootClusterGroup.LODBounds = FSpheref();
	RootClusterGroup.MaxParentLODError = 1e10f;
	RootClusterGroup.MinLODError = -1.0f;
	RootClusterGroup.MipLevel = RootCluster.MipLevel + 1;
	RootClusterGroup.MeshIndex = MeshIndex;
	RootClusterGroup.bTrimmed = false;
	RootCluster.GroupIndex = (uint32)ClusterGroups.size();
	RootCI.GroupIndex = (uint32)ClusterGroups.size();
	ClusterGroups.push_back(RootClusterGroup);
}

void SaveToDisk(TNaniteMesh& Mesh)
{
	const TString DestFilename = MeshFilename + ".tasset";

	FNaniteAssetHeader Header;
	Header.RootDataSize = (uint32)Mesh.RootData.size();
	Header.StreamablePagesSize = (uint32)Mesh.StreamablePages.size();
	Header.NumBvHNodes = (uint32)Mesh.HierarchyNodes.size();
	Header.NumPages = (uint32)Mesh.PageStreamingStates.size();
	Header.NumPageDependencies = (uint32)Mesh.PageDependencies.size();
	Header.NumRootPages = Mesh.NumRootPages;
	Header.NumRootPageClusters = Mesh.NumRootPageClusters;
	Header.PositionPrecision = Mesh.PositionPrecision;

	TStream S(sizeof(Header) + Header.GetDataSize() + 16);
	S.Put(&Header, sizeof(Header));
	S.Put(Mesh.RootData.data(), Header.RootDataSize);
	S.FillZeroToAlign(4);
	S.Put(Mesh.StreamablePages.data(), Header.StreamablePagesSize);
	S.FillZeroToAlign(4);
	S.Put(Mesh.HierarchyNodes.data(), Header.NumBvHNodes * sizeof(FPackedHierarchyNode));
	S.Put(Mesh.PageStreamingStates.data(), Header.NumPages * sizeof(FPageStreamingState));
	S.Put(Mesh.PageDependencies.data(), Header.NumPageDependencies * sizeof(uint32));

	TFile F;
	if (F.Open(DestFilename, EFA_CREATEWRITE))
	{
		F.Write(S.GetBuffer(), S.GetBufferSize());
		F.Close();
	}
}

bool TNaniteMesh::ConvertNanieMesh(TNaniteMesh& Mesh)
{
	TVector<NaniteRawMeshData> RawDatas;
	TVector<float> LODErrors;
	TVector<FClusterInstance> ClusterInstances;
	TVector<int32> ClusterInsGroups, ClusterInsParents;	// each cluster instance's group and parent

	LoadDataFromJson(RawDatas, LODErrors, ClusterInstances, ClusterInsGroups, ClusterInsParents);

	TVector<FCluster> ClusterSources;
	int32 TotalClusters = 0;
	for (const auto& RawData : RawDatas)
	{
		TotalClusters += RawData.NumClusters;
	}
	ClusterSources.reserve(TotalClusters);

	ClusterTrianglesLOD0(RawDatas[0], ClusterInstances, ClusterSources);

	TVector<FClusterGroup> ClusterGroups;
	int32 TotalGroups = 0;
	for (const auto& RawData : RawDatas)
	{
		TotalGroups += RawData.NumGroups;
	}
	ClusterGroups.reserve(TotalGroups);

	BuildDAGFromLODs(RawDatas, ClusterInsGroups, ClusterInsParents, LODErrors, ClusterInstances, ClusterSources, ClusterGroups);

	Encode(Mesh, ClusterGroups, ClusterSources, ClusterInstances);
	
	SaveToDisk(Mesh);

	return true;
}