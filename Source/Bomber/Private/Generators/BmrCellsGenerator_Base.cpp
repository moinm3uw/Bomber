// Copyright (c) Yevhenii Selivanov

#include "Generators/BmrCellsGenerator_Base.h"

// Bomber
#include "Bomber.h"
#include "UtilityLibraries/CellsUtilsLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BmrCellsGenerator_Base)

// Main generation function to be overridden by child classes
TMap<FCell, EActorType> UBmrCellsGenerator_Base::GenerateLevel(FBmrGeneratorData&& Params)
{
	/**
	 * This is the fallback implementation for all generators. If a child generator
	 * fails for any reason, it should call this function to ensure a valid, playable,
	 * albeit empty, map is always produced.
	 *
	 * Example Layout (9x9):
	 * P . . . . . . . P
	 * . . . . . . . . .
	 * . . . . . . . . .
	 * . . . . . . . . .
	 * . . . . . . . . .
	 * . . . . . . . . .
	 * . . . . . . . . .
	 * . . . . . . . . .
	 * P . . . . . . . P
	 */
	TMap<FCell, EActorType> ActorsToSpawn;
	const FCells CornerCells = UCellsUtilsLibrary::GetCornerCellsOnGrid(FCells(Params.AllCells));
	for (const FCell& CornerCell : CornerCells)
	{
		ActorsToSpawn.Emplace(CornerCell, EActorType::Player);
	}

	// Always respect user-dragged cells, even in the fallback
	ActorsToSpawn.Append(Params.DraggedCells);

	return ActorsToSpawn;
}