// Fill out your copyright notice in the Description page of Project Settings.


#include "TiXExportFunctions.h"

void FTiXExportFunctions::ExportScene(FTiXScene& Scene)
{
	check(0);

	// Export scene desc

	// Export tiles
	for (const auto& TileIter : Scene.SceneTiles)
	{
		FTiXSceneTile* Tile = TileIter.Value;
		Tile->UpdateSceneTileDesc();
	}

	// Export static meshes

	// Export materials

	// Export textures
}