// Copyright (c) Yevhenii Selivanov

#pragma once

#include "UObject/Object.h"

// Bomber
#include "Structures/Cell.h"
#include "Structures/GeneratedMapSettings.h"

#include "BmrCellsGenerator_Base.generated.h"

enum class EActorType : uint8;

/**
 * Holds all necessary runtime data for the level generation process.
 */
struct FBmrGeneratorData
{
	FCellsArr AllCells;
	FIntPoint MapScale = FIntPoint::ZeroValue;
	TMap<FCell, FIntPoint> AllCellPositions;
	FGeneratedMapSettings GenerationSettings;
	TMap<FCell, EActorType> DraggedCells;

	FORCEINLINE bool IsValid() const { return !AllCellPositions.IsEmpty() && MapScale.X > 0 && MapScale.Y > 0; }
};

/**
 * The base implementation of a level generator.
 * Provides fallback that generates an empty level with players in the corners.
 * Some generators expose additional settings.
 */
UCLASS(Abstract, BlueprintType, NotBlueprintable, EditInlineNew)
class BOMBER_API UBmrCellsGenerator_Base : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Main generation function to be overridden by child classes.
	 * When a generation algorithm fails, it should call the Super::GenerateLevel implementation
	 * to ensure a valid, playable (though empty) map is always produced.
	 * @param Params A struct containing all necessary data for the generation.
	 * @return A map of cells to actor types for the entire level.
	 */
	virtual TMap<FCell, EActorType> GenerateLevel(FBmrGeneratorData&& Params);
};