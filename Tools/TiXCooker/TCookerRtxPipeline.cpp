/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#include "stdafx.h"
#include "ResHelper.h"
#include "TCookerRtxPipeline.h"

using namespace rapidjson;

namespace tix
{
	TCookerRtxPipeline::TCookerRtxPipeline()
	{
	}

	TCookerRtxPipeline::~TCookerRtxPipeline()
	{
	}

	bool TCookerRtxPipeline::Load(const TJSON& Doc)
	{
		// shader lib
		Doc["shader_lib"] << ShaderLibName;

		// export names
		Doc["export_names"] << RtxDesc.ExportNames;

		// configs
		Doc["max_attribute_size_in_bytes"] << RtxDesc.MaxAttributeSizeInBytes;
		Doc["max_payload_size_in_bytes"] << RtxDesc.MaxPayloadSizeInBytes;
		Doc["max_trace_recursion_depth"] << RtxDesc.MaxTraceRecursionDepth;

		// hit group
		Doc["hit_group_name"] << RtxDesc.HitGroupName;
		Doc["hit_group"] << RtxDesc.HitGroup;
		TI_ASSERT(RtxDesc.HitGroup.size() == HITGROUP_NUM);

		return true;
	}
	
	void TCookerRtxPipeline::SaveTrunk(TChunkFile& OutChunkFile)
	{
		TStream& OutStream = OutChunkFile.GetChunk(GetCookerType());
		TVector<TString>& OutStrings = OutChunkFile.Strings;

		TResfileChunkHeader ChunkHeader;
		ChunkHeader.ID = TIRES_ID_CHUNK_RTX_PIPELINE;
		ChunkHeader.Version = TIRES_VERSION_CHUNK_RTX_PIPELINE;
		ChunkHeader.ElementCount = 1;

		TStream HeaderStream, DataStream(1024 * 8);
		for (int32 t = 0; t < ChunkHeader.ElementCount; ++t)
		{
			THeaderRtxPipeline Define;
			Define.ShaderLibName = AddStringToList(OutStrings, ShaderLibName);
			Define.NumExportNames = (int32)RtxDesc.ExportNames.size();
			Define.HitGroupName = AddStringToList(OutStrings, RtxDesc.HitGroupName);
			Define.HitGroupAnyHit = 
				RtxDesc.HitGroup[HITGROUP_ANY_HIT] == "" ? -1 : AddStringToList(OutStrings, RtxDesc.HitGroup[HITGROUP_ANY_HIT]);
			Define.HitGroupClosestHit = 
				RtxDesc.HitGroup[HITGROUP_CLOSEST_HIT] == "" ? -1 : AddStringToList(OutStrings, RtxDesc.HitGroup[HITGROUP_CLOSEST_HIT]);
			Define.HitGroupIntersection = 
				RtxDesc.HitGroup[HITGROUP_INTERSECTION] == "" ? -1 : AddStringToList(OutStrings, RtxDesc.HitGroup[HITGROUP_INTERSECTION]);
			Define.MaxAttributeSizeInDepth = RtxDesc.MaxAttributeSizeInBytes;
			Define.MaxPayloadSizeInBytes = RtxDesc.MaxPayloadSizeInBytes;
			Define.MaxTraceRecursionDepth = RtxDesc.MaxTraceRecursionDepth;

			// Save header
			HeaderStream.Put(&Define, sizeof(THeaderRtxPipeline));

			// Write data
			TVector<int32> ExportNameIndices;
			ExportNameIndices.resize(Define.NumExportNames);
			for (int32 i = 0; i < Define.NumExportNames; i++)
			{
				ExportNameIndices[i] = AddStringToList(OutStrings, RtxDesc.ExportNames[i]);
			}
			DataStream.Put(ExportNameIndices.data(), (uint32)(ExportNameIndices.size() * sizeof(int32)));
		}

		ChunkHeader.ChunkSize = HeaderStream.GetLength() + DataStream.GetLength();

		OutStream.Put(&ChunkHeader, sizeof(TResfileChunkHeader));
		OutStream.FillZero4();
		OutStream.Put(HeaderStream.GetBuffer(), HeaderStream.GetLength());
		OutStream.Put(DataStream.GetBuffer(), DataStream.GetLength());
	}
}
