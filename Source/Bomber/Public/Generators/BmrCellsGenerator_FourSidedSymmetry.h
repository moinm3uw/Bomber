// Copyright (c) Yevhenii Selivanov

#pragma once

#include "BmrCellsGenerator_Base.h"

#include "BmrCellsGenerator_FourSidedSymmetry.generated.h"

/**
 * Contains all input data required to generate the core path network for a quarter of the map.
 */
struct FBmrGeneratorQuarterPathParams
{
	const FCells& QuarterCells;
	const FCell& StartCell;
	const FIntPoint& QuarterScale;
	const FCells& DraggedWalls;
	const FCells& PriorityTargets;
};

/**
 * Contains all input data required for the obstacle-filling phase of generation.
 */
struct FBmrGeneratorQuarterFillParams
{
	FCells&& PathCells;
	const FCell& PlayerStartCell;
};

/**
 * Wraps all parameters for the FindDirectedPathWithDetours function.
 */
struct FBmrGeneratorPathfindParams
{
	const FCell& StartNode;
	const FCells& TargetCells;
	const FIntPoint& PrimaryDirection;
	const TArray<FIntPoint>& SecondaryDirections;
};

/**
 * Creates a random but fair map by generating one-quarter of the level
 * and then mirroring it to create a perfectly symmetrical battlefield.
 * It guarantees that paths exist between all players, boxes and to any dragged items.
 * Example Layout (P=Player, █=Wall, □=Box):
 * P . □ █ █ □ . P
 * . . □ . . □ . .
 * □ □ . . . . □ □
 * █ . . . . . . █
 * █ . . . . . . █
 * □ □ . . . . □ □
 * . . □ . . □ . .
 * P . □ █ █ □ . P
 */
UCLASS(DisplayName = "4-Sided Symmetry")
class BOMBER_API UBmrCellsGenerator_FourSidedSymmetry : public UBmrCellsGenerator_Base
{
	GENERATED_BODY()

protected:
	/** If enabled, displays the calculated core path for connecting dragged items. */
	UPROPERTY(EditDefaultsOnly)
	bool bDisplayPrimaryPath = false;

	/** If enabled, displays the calculated path connecting the network to the right border. */
	UPROPERTY(EditDefaultsOnly)
	bool bDisplayRightPath = false;

	/** If enabled, displays the calculated path connecting the network to the bottom border. */
	UPROPERTY(EditDefaultsOnly)
	bool bDisplayBottomPath = false;

public:
	/** Is overriden to implement custom level generation logic. */
	virtual TMap<FCell, EActorType> GenerateLevel(FBmrGeneratorData&& GeneratorData) override;

private:
	// --- Helper Methods ---

	/** Creates a network of paths from the player start to priority targets and quarter borders. */
	FCells GeneratePathsForQuarter(const FBmrGeneratorQuarterPathParams& PathParams, const FBmrGeneratorData& GeneratorData);

	/** Fills the quarter with walls and boxes based on fill percentages. */
	static TMap<FCell, EActorType> FillObstaclesInQuarter(FBmrGeneratorQuarterFillParams&& FillParams, const FBmrGeneratorQuarterPathParams& PathParams, const FBmrGeneratorData& GeneratorData);

	/** Applies four-sided symmetry to the generated quarter map to build the full level. */
	static TMap<FCell, EActorType> ApplySymmetry(TMap<FCell, EActorType>&& QuarterActors, const FBmrGeneratorData& GeneratorData);

	/** Creates a directed but randomized path. */
	static FCellsArr FindDirectedPathWithDetours(const FBmrGeneratorPathfindParams& Params, const FBmrGeneratorQuarterPathParams& PathParams, const FBmrGeneratorData& GeneratorData);

	/** Finds all reachable cells from a starting point. */
	static FCells FindWalkableArea(const FCell& StartCell, const FCells& WallCells, const FBmrGeneratorQuarterPathParams& PathParams, const FBmrGeneratorData& GeneratorData);
};