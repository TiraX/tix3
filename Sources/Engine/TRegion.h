/*
	TiX Engine v2.0 Copyright (C) 2018~2021
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	class TRegion
	{
	public:
		TRegion();
		TRegion(int32 InRegionSize, int32 InCellSize);
		~TRegion();

		struct TRegionDesc
		{
			int16 XCount;
			int16 YCount;
		};

		void Reset(int32 InRegionSize, int32 InCellSize);
		TRegionDesc* FindAvailbleRegion(int32 Width, int32 Height, uint32* OutRegionIndex = nullptr);
		TRegionDesc* GetRegionByIndex(uint32 RegionIndex);
		void GetRegionSizeRequirement(int32& Width, int32& Height);

		int32 GetRegionSize() const
		{
			return RegionSize;
		}

		int32 GetCellSize() const
		{
			return CellSize;
		}

	protected:
		void SubDivideRegion(uint32 MatchRegionId, int32 NewXCount, int32 NewYCount);

	private:
		int32 RegionSize;
		int32 CellSize;

		TVector<TRegionDesc> Regions;
		TVector<uint32> AvailableRegions;
	};
} // end namespace tix
