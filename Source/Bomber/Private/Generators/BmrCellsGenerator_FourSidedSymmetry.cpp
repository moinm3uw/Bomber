// Copyright (c) Yevhenii Selivanov

#include "Generators/BmrCellsGenerator_FourSidedSymmetry.h"
//---
#include "Bomber.h"
#include "UtilityLibraries/CellsUtilsLibrary.h"
//---
#include "Containers/Queue.h"
//---
#include UE_INLINE_GENERATED_CPP_BY_NAME(BmrCellsGenerator_FourSidedSymmetry)

// Is overriden to implement custom level generation logic
TMap<FCell, EActorType> UBmrCellsGenerator_FourSidedSymmetry::GenerateLevel(FBmrGeneratorData&& GeneratorData)
{
	if (!GeneratorData.IsValid())
	{
		return Super::GenerateLevel(MoveTemp(GeneratorData));
	}

	// --- Step 1: Initial Setup and Projection ---
	// Defines the top-left quarter and projects any dragged items/boxes into that quarter
	// so they can be included in the pathfinding logic.
	const FCellsArr QuarterCellsArray = FCell::GetTopLeftQuarterOnGrid(GeneratorData.AllCells, GeneratorData.MapScale);
	const FCells QuarterCellsSet(QuarterCellsArray);

	FCells PriorityTargets, DraggedWalls;
	const int32 MaxIndexX = GeneratorData.MapScale.X - 1;
	const int32 MaxIndexY = GeneratorData.MapScale.Y - 1;

	for (const TTuple<FCell, EActorType>& DraggedPair : GeneratorData.DraggedCells)
	{
		if (DraggedPair.Value == EActorType::Wall)
		{
			DraggedWalls.Add(DraggedPair.Key);
			continue;
		}
		if (DraggedPair.Value == EActorType::Item || DraggedPair.Value == EActorType::Box)
		{
			const FIntPoint OriginalPosition = FCell::GetPositionByCellOnGrid(DraggedPair.Key, GeneratorData.AllCells, GeneratorData.MapScale.X);
			const int32 ProjectedX = FMath::Min(OriginalPosition.X, MaxIndexX - OriginalPosition.X);
			const int32 ProjectedY = FMath::Min(OriginalPosition.Y, MaxIndexY - OriginalPosition.Y);
			const FCell ProjectedCell = FCell::GetCellByPositionOnGrid({ProjectedX, ProjectedY}, GeneratorData.AllCells, GeneratorData.MapScale.X);
			if (ProjectedCell.IsValid())
			{
				PriorityTargets.Add(ProjectedCell);
			}
		}
	}

	// --- Step 2: Generate Core Path Network ---
	// Creates a network of empty cells within the quarter that connects the player start,
	// all priority targets, and the right and bottom borders.
	static constexpr int32 HalfAxisScaleDivider = 2;
	const FIntPoint QuarterScale(GeneratorData.MapScale.X / HalfAxisScaleDivider + 1, GeneratorData.MapScale.Y / HalfAxisScaleDivider + 1);
	const FCell PlayerStartCell = QuarterCellsArray.IsEmpty() ? FCell::InvalidCell : QuarterCellsArray[0];

	const FBmrGeneratorQuarterPathParams PathParams{QuarterCellsSet, PlayerStartCell, QuarterScale, DraggedWalls, PriorityTargets};
	FCells PathCells = GeneratePathsForQuarter(PathParams, GeneratorData); // Made non-const to allow moving

	// --- Step 3: Fill Quarter with Obstacles ---
	// Places indestructible walls and destructible boxes in the remaining empty space,
	// based on the fill percentages defined in the Generation Settings.
	TMap<FCell, EActorType> QuarterActors = FillObstaclesInQuarter({MoveTemp(PathCells), PlayerStartCell}, PathParams, GeneratorData); // Use MoveTemp

	// --- Step 4: Apply Symmetry ---
	// Mirrors the generated quarter to the other three corners of the map to create
	// the final, full level layout.
	TMap<FCell, EActorType> ActorsToSpawn = ApplySymmetry(MoveTemp(QuarterActors), GeneratorData);

	// --- Step 5: Finalize with Dragged Actors ---
	// Merges the designer-placed actors, overwriting any generated actors that
	// occupy the same cells.
	ActorsToSpawn.Append(MoveTemp(GeneratorData.DraggedCells));

	return ActorsToSpawn;
}

// Creates a network of paths from the player start to priority targets and quarter borders.
FCells UBmrCellsGenerator_FourSidedSymmetry::GeneratePathsForQuarter(const FBmrGeneratorQuarterPathParams& PathParams, const FBmrGeneratorData& GeneratorData)
{
	/**
	 * Example Path Network ('P'rimary, 'R'ight, 'B'ottom):
	 *   A   B   C   D   E
	 * \+---+---+---+---+---+
	 * 1 | S | P | P | . | . | (S = Start Cell)
	 * \+---+---+---+---+---+
	 * 2 | B | B | P | . | . |
	 * \+---+---+---+---+---+
	 * 3 | . | B | P | R | R | <- Path reaches right border
	 * \+---+---+---+---+---+
	 * 4 | . | B | . | . | . |
	 * \+---+---+---+---+---+
	 * 5 | . | B | B | . | . | <- Path reaches bottom border
	 * \+---+---+---+---+---+
	 */
	if (!PathParams.StartCell.IsValid())
	{
		return FCells();
	}

	FCells PathNetwork;
	PathNetwork.Add(PathParams.StartCell);
	FCellsArr PrimaryPath, PathToRight, PathToBottom;

	static const FIntPoint Right(1, 0), Left(-1, 0), Down(0, 1), Up(0, -1);
	static const TArray HorizontalAndUpDirs = {Right, Left, Up};

	for (const FCell& TargetCell : PathParams.PriorityTargets)
	{
		if (PathNetwork.Contains(TargetCell))
			continue;
		const FCellsArr NetworkAsArray = PathNetwork.Array();
		const FCell RandomStart = NetworkAsArray[FMath::RandRange(0, NetworkAsArray.Num() - 1)];
		const FBmrGeneratorPathfindParams FindParams{RandomStart, {TargetCell}, Down, HorizontalAndUpDirs};
		const FCellsArr PathToTarget = FindDirectedPathWithDetours(FindParams, PathParams, GeneratorData);
		PrimaryPath.Append(PathToTarget);
		PathNetwork.Append(PathToTarget);
	}

	const int32 RightBorderIndex = PathParams.QuarterScale.X - 1;
	FCells RightBorderCells;
	for (int32 Index = 0; Index < PathParams.QuarterScale.Y; ++Index) { RightBorderCells.Add(FCell::GetCellByPositionOnGrid({RightBorderIndex, Index}, GeneratorData.AllCells, GeneratorData.MapScale.X)); }
	if (!RightBorderCells.IsEmpty())
	{
		const FCellsArr NetworkAsArray = PathNetwork.Array();
		const FCell RandomStart = NetworkAsArray[FMath::RandRange(0, NetworkAsArray.Num() - 1)];
		const FBmrGeneratorPathfindParams FindParams{RandomStart, RightBorderCells, Right, {Up, Down}};
		PathToRight = FindDirectedPathWithDetours(FindParams, PathParams, GeneratorData);
		PathNetwork.Append(PathToRight);
	}

	const int32 BottomBorderIndex = PathParams.QuarterScale.Y - 1;
	FCells BottomBorderCells;
	for (int32 Index = 0; Index < PathParams.QuarterScale.X; ++Index) { BottomBorderCells.Add(FCell::GetCellByPositionOnGrid({Index, BottomBorderIndex}, GeneratorData.AllCells, GeneratorData.MapScale.X)); }
	if (!BottomBorderCells.IsEmpty())
	{
		const FCellsArr NetworkAsArray = PathNetwork.Array();
		const FCell RandomStart = NetworkAsArray[FMath::RandRange(0, NetworkAsArray.Num() - 1)];
		const FBmrGeneratorPathfindParams FindParams{RandomStart, BottomBorderCells, Down, {Left, Right}};
		PathToBottom = FindDirectedPathWithDetours(FindParams, PathParams, GeneratorData);
		PathNetwork.Append(PathToBottom);
	}

#if !UE_BUILD_SHIPPING
	if (bDisplayPrimaryPath)
	{
		FDisplayCellsParams DisplayParams;
		DisplayParams.bClearPreviousDisplays = true;
		DisplayParams.TextColor = FLinearColor::White;
		UCellsUtilsLibrary::DisplayCells(this, FCells(PrimaryPath), DisplayParams);
	}
	if (bDisplayRightPath)
	{
		FDisplayCellsParams DisplayParams;
		DisplayParams.bClearPreviousDisplays = !bDisplayPrimaryPath;
		DisplayParams.TextColor = FLinearColor::Blue;
		DisplayParams.RenderString = TEXT("->");
		UCellsUtilsLibrary::DisplayCells(this, FCells(PathToRight), DisplayParams);
	}
	if (bDisplayBottomPath)
	{
		FDisplayCellsParams DisplayParams;
		DisplayParams.bClearPreviousDisplays = !bDisplayPrimaryPath && !bDisplayRightPath;
		DisplayParams.TextColor = FLinearColor::Green;
		DisplayParams.RenderString = TEXT("v");
		UCellsUtilsLibrary::DisplayCells(this, FCells(PathToBottom), DisplayParams);
	}
#endif

	return PathNetwork;
}

// Fills the quarter with walls and boxes based on fill percentages
TMap<FCell, EActorType> UBmrCellsGenerator_FourSidedSymmetry::FillObstaclesInQuarter(FBmrGeneratorQuarterFillParams&& FillParams, const FBmrGeneratorQuarterPathParams& PathParams, const FBmrGeneratorData& GeneratorData)
{
	/**
	 * Example Quarter Fill:
	 * +---+---+---+---+
	 * | P |   | █ | □ | (P=Player, █=Wall, □=Box)
	 * +---+---+---+---+
	 * |   |   | □ | █ | (Empty cells are part of the path network)
	 * +---+---+---+---+
	 */
	TMap<FCell, EActorType> QuarterActors;
	if (!FillParams.PlayerStartCell.IsValid())
	{
		return QuarterActors;
	}

	QuarterActors.Emplace(FillParams.PlayerStartCell, EActorType::Player);
	const FIntPoint PlayerPosition = FCell::GetPositionByCellOnGrid(FillParams.PlayerStartCell, GeneratorData.AllCells, GeneratorData.MapScale.X);

	FCellsArr PossibleWallLocations;
	for (const FCell& CurrentCell : PathParams.QuarterCells)
	{
		if (FillParams.PathCells.Contains(CurrentCell))
		{
			continue;
		}
		const FIntPoint CellPosition = FCell::GetPositionByCellOnGrid(CurrentCell, GeneratorData.AllCells, GeneratorData.MapScale.X);
		if (FMath::Abs(CellPosition.X - PlayerPosition.X) + FMath::Abs(CellPosition.Y - PlayerPosition.Y) <= 1)
		{
			continue;
		}
		PossibleWallLocations.Add(CurrentCell);
	}

	static constexpr float PercentageDivider = 100.0f;
	const int32 NumWallsToPlace = FMath::FloorToInt(PossibleWallLocations.Num() * (GeneratorData.GenerationSettings.WallsChance / PercentageDivider));
	FCell::RandShuffle(PossibleWallLocations);

	FCells PlacedWallCells;
	for (int32 Index = 0; Index < NumWallsToPlace; ++Index)
	{
		const FCell& WallCell = PossibleWallLocations[Index];
		QuarterActors.Emplace(WallCell, EActorType::Wall);
		PlacedWallCells.Add(WallCell);
	}

	const FCells WalkableArea = FindWalkableArea(FillParams.PlayerStartCell, PlacedWallCells, PathParams, GeneratorData);
	FCellsArr PossibleBoxLocations;
	for (const FCell& CurrentCell : WalkableArea)
	{
		if (QuarterActors.Contains(CurrentCell))
		{
			continue;
		}
		const FIntPoint CellPosition = FCell::GetPositionByCellOnGrid(CurrentCell, GeneratorData.AllCells, GeneratorData.MapScale.X);
		if (FMath::Abs(CellPosition.X - PlayerPosition.X) + FMath::Abs(CellPosition.Y - PlayerPosition.Y) <= 1)
		{
			continue;
		}
		PossibleBoxLocations.Add(CurrentCell);
	}

	const int32 NumBoxesToPlace = FMath::FloorToInt(PossibleBoxLocations.Num() * (GeneratorData.GenerationSettings.BoxesChance / PercentageDivider));
	FCell::RandShuffle(PossibleBoxLocations);

	for (int32 Index = 0; Index < NumBoxesToPlace; ++Index)
	{
		QuarterActors.Emplace(PossibleBoxLocations[Index], EActorType::Box);
	}
	return QuarterActors;
}

// Applies four-sided symmetry to the generated quarter map to build the full level.
TMap<FCell, EActorType> UBmrCellsGenerator_FourSidedSymmetry::ApplySymmetry(TMap<FCell, EActorType>&& QuarterActors, const FBmrGeneratorData& GeneratorData)
{
	/**
	 * Quarter:       Full Map:
	 * P □ █          P □ █ █ □ P
	 *   + .            + . . + .
	 *                . . . . . .
	 *                . . . . . .
	 *                . + . . + .
	 *                P □ █ █ □ P
	 */
	TMap<FCell, EActorType> FullMapActors;
	const int32 MaxIndexX = GeneratorData.MapScale.X - 1;
	const int32 MaxIndexY = GeneratorData.MapScale.Y - 1;

	for (const TTuple<FCell, EActorType>& ActorEntry : QuarterActors)
	{
		const FCell& QuarterCell = ActorEntry.Key;
		const EActorType ActorType = ActorEntry.Value;
		const FIntPoint OriginalPosition = FCell::GetPositionByCellOnGrid(QuarterCell, GeneratorData.AllCells, GeneratorData.MapScale.X);
		const int32 SymmetricalX = MaxIndexX - OriginalPosition.X;
		const int32 SymmetricalY = MaxIndexY - OriginalPosition.Y;

		const TArray<FIntPoint> PositionsToPlace = {
			{OriginalPosition.X, OriginalPosition.Y}, {SymmetricalX, OriginalPosition.Y},
			{OriginalPosition.X, SymmetricalY}, {SymmetricalX, SymmetricalY}
		};

		for (const FIntPoint& SymPosition : PositionsToPlace)
		{
			const FCell TargetCell = FCell::GetCellByPositionOnGrid(SymPosition, GeneratorData.AllCells, GeneratorData.MapScale.X);
			if (TargetCell.IsValid() && !FullMapActors.Contains(TargetCell))
			{
				FullMapActors.Emplace(TargetCell, ActorType);
			}
		}
	}
	return FullMapActors;
}

// Creates a directed but randomized path from a start cell to one of the target cells
FCellsArr UBmrCellsGenerator_FourSidedSymmetry::FindDirectedPathWithDetours(const FBmrGeneratorPathfindParams& Params, const FBmrGeneratorQuarterPathParams& PathParams, const FBmrGeneratorData& GeneratorData)
{
	/**
	 * This function uses a biased random walk algorithm. For example, to find a path
	 * from Start (S) to End (E), it will primarily move right but can take random (up, down)
	 * detours up or down to create a more natural-looking path.
	 *
	 * Example Path ('+'):
	 * S + . . . .
	 * . + . + + E
	 * . + + + . .
	 * . . . . . .
	 * . . . . . .
	 */
	FCellsArr PathStack;
	if (!Params.StartNode.IsValid())
	{
		return PathStack;
	}
	PathStack.Push(Params.StartNode);

	FCells VisitedCells = PathParams.DraggedWalls;
	VisitedCells.Add(Params.StartNode);

	static constexpr int32 PrimaryMoveBias = 3;
	FCellsArr PossibleNextCells;

	while (!PathStack.IsEmpty())
	{
		const FCell CurrentCell = PathStack.Last();
		if (Params.TargetCells.Contains(CurrentCell))
		{
			return PathStack;
		}

		PossibleNextCells.Reset();
		const FIntPoint CurrentPosition = FCell::GetPositionByCellOnGrid(CurrentCell, GeneratorData.AllCells, GeneratorData.MapScale.X);

		auto CheckNeighbor = [&](const FIntPoint& NeighborPos, bool bIsPrimary)
		{
			// Constrain the path to within the quarter's dimensions
			if (NeighborPos.X < PathParams.QuarterScale.X && NeighborPos.Y < PathParams.QuarterScale.Y && NeighborPos.X >= 0 && NeighborPos.Y >= 0)
			{
				const FCell NeighborCell = FCell::GetCellByPositionOnGrid(NeighborPos, GeneratorData.AllCells, GeneratorData.MapScale.X);
				if (NeighborCell.IsValid() && !VisitedCells.Contains(NeighborCell))
				{
					const int32 NumToAdd = bIsPrimary ? PrimaryMoveBias : 1;
					for (int32 i = 0; i < NumToAdd; ++i)
					{
						PossibleNextCells.Add(NeighborCell);
					}
				}
			}
		};

		CheckNeighbor(CurrentPosition + Params.PrimaryDirection, true);
		for (const FIntPoint& SecondaryDir : Params.SecondaryDirections)
		{
			CheckNeighbor(CurrentPosition + SecondaryDir, false);
		}

		if (!PossibleNextCells.IsEmpty())
		{
			const FCell NextCell = PossibleNextCells[FMath::RandRange(0, PossibleNextCells.Num() - 1)];
			VisitedCells.Add(NextCell);
			PathStack.Push(NextCell);
		}
		else
		{
			PathStack.Pop();
		}
	}
	return {Params.StartNode};
}

// Finds all reachable cells from a starting point using Breadth-First Search
FCells UBmrCellsGenerator_FourSidedSymmetry::FindWalkableArea(const FCell& StartCell, const FCells& WallCells, const FBmrGeneratorQuarterPathParams& PathParams, const FBmrGeneratorData& GeneratorData)
{
	/**
	 * This function performs a Breadth-First Search (a flood-fill) to discover every
	 * cell that can be reached from a given start point without crossing over a set of specified wall cells.
	 */
	FCells WalkableArea;
	if (!StartCell.IsValid())
	{
		return WalkableArea;
	}

	TQueue<FCell> CellsToVisit;
	FCells VisitedCells = WallCells;
	CellsToVisit.Enqueue(StartCell);
	VisitedCells.Add(StartCell);

	while (!CellsToVisit.IsEmpty())
	{
		FCell CurrentCell;
		CellsToVisit.Dequeue(CurrentCell);
		WalkableArea.Add(CurrentCell);

		const FIntPoint CurrentPosition = FCell::GetPositionByCellOnGrid(CurrentCell, GeneratorData.AllCells, GeneratorData.MapScale.X);
		static const TArray<FIntPoint> AllDirections = {{0, 1}, {0, -1}, {1, 0}, {-1, 0}};

		for (const FIntPoint& Direction : AllDirections)
		{
			const FIntPoint NeighborPosition = CurrentPosition + Direction;
			// Constrain the flood-fill to within the quarter's dimensions
			if (NeighborPosition.X < PathParams.QuarterScale.X && NeighborPosition.Y < PathParams.QuarterScale.Y && NeighborPosition.X >= 0 && NeighborPosition.Y >= 0)
			{
				const FCell NeighborCell = FCell::GetCellByPositionOnGrid(NeighborPosition, GeneratorData.AllCells, GeneratorData.MapScale.X);
				if (NeighborCell.IsValid() && !VisitedCells.Contains(NeighborCell))
				{
					VisitedCells.Add(NeighborCell);
					CellsToVisit.Enqueue(NeighborCell);
				}
			}
		}
	}
	return WalkableArea;
}